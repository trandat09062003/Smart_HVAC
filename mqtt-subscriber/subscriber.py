import paho.mqtt.client as mqtt
import json
import psycopg2
import os
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread
from urllib.parse import urlparse

# Config
MQTT_BROKER = os.getenv('MQTT_BROKER', 'mosquitto')
MQTT_PORT = int(os.getenv('MQTT_PORT', '1883'))
DB_HOST = os.getenv('DB_HOST', 'timescaledb')
DB_NAME = os.getenv('DB_NAME', 'iotdb')
DB_USER = os.getenv('DB_USER', 'admin')
DB_PASS = os.getenv('DB_PASSWORD', 'admin123')
API_HOST = os.getenv('API_HOST', '0.0.0.0')
API_PORT = int(os.getenv('API_PORT', '5000'))
CONTROL_TOPIC = os.getenv('CONTROL_TOPIC', 'remote-control')

# ====================== DATABASE ======================
def get_db_connection():
    last_error = None
    for attempt in range(1, 11):
        try:
            return psycopg2.connect(
                host=DB_HOST, database=DB_NAME, user=DB_USER, password=DB_PASS
            )
        except psycopg2.OperationalError as error:
            last_error = error
            print(f"⏳ Waiting for database... attempt {attempt}/10")
            time.sleep(2)

    raise last_error

conn = get_db_connection()
cur = conn.cursor()

# Tạo bảng + hypertable
cur.execute("""
CREATE TABLE IF NOT EXISTS sensor_data (
    time TIMESTAMPTZ NOT NULL,
    device_id TEXT,
    temperature FLOAT,
    outdoor_temperature FLOAT,
    humidity FLOAT,
    co2 INT,
    dust FLOAT,
    PRIMARY KEY (time, device_id)
);

CREATE TABLE IF NOT EXISTS remote_control_state (
    time TIMESTAMPTZ NOT NULL,
    device_id TEXT NOT NULL,
    power BOOLEAN NOT NULL,
    temp FLOAT NOT NULL,
    operation_mode TEXT NOT NULL,
    fan_power TEXT NOT NULL,
    client_id TEXT,
    requested_at TIMESTAMPTZ,
    PRIMARY KEY (time, device_id)
);

ALTER TABLE sensor_data ADD COLUMN IF NOT EXISTS outdoor_temperature FLOAT;
ALTER TABLE remote_control_state ADD COLUMN IF NOT EXISTS client_id TEXT;
ALTER TABLE remote_control_state ADD COLUMN IF NOT EXISTS requested_at TIMESTAMPTZ;
SELECT create_hypertable('sensor_data', 'time', if_not_exists => TRUE);
SELECT create_hypertable('remote_control_state', 'time', if_not_exists => TRUE);
""")
conn.commit()

def is_outdoor_device(device_id):
    normalized = (device_id or '').lower()
    return any(token in normalized for token in ['outdoor', 'outside', 'ngoai', 'ngoài'])

def to_float(value):
    try:
        return None if value is None else float(value)
    except (TypeError, ValueError):
        return None

def to_int(value):
    try:
        return None if value is None else int(value)
    except (TypeError, ValueError):
        return None

def get_outdoor_temperature(payload):
    for key in ['outdoor_temperature', 'outdoorTemp', 'outside_temperature', 'outsideTemp']:
        value = to_float(payload.get(key))
        if value is not None:
            return value
    return None

def fetch_telemetry():
    with get_db_connection() as api_conn:
        with api_conn.cursor() as api_cur:
            api_cur.execute("""
                SELECT time, device_id, temperature, outdoor_temperature, humidity, co2, dust
                FROM sensor_data
                ORDER BY time DESC
                LIMIT 200
            """)
            rows = api_cur.fetchall()

            api_cur.execute("""
                SELECT time, device_id, power, temp, operation_mode, fan_power, client_id, requested_at
                FROM remote_control_state
                ORDER BY time DESC
                LIMIT 1
            """)
            control_row = api_cur.fetchone()

    latest_primary = None
    latest_outdoor = None
    for row in rows:
        point = {
            'time': row[0].isoformat(),
            'device_id': row[1],
            'temperature': row[2],
            'outdoor_temperature': row[3],
            'humidity': row[4],
            'co2': row[5],
            'dust': row[6],
        }

        if latest_primary is None and not is_outdoor_device(point['device_id']):
            latest_primary = point

        if latest_outdoor is None and (
            point['outdoor_temperature'] is not None or is_outdoor_device(point['device_id'])
        ):
            latest_outdoor = point

    if latest_primary is None and rows:
        row = rows[0]
        latest_primary = {
            'time': row[0].isoformat(),
            'device_id': row[1],
            'temperature': row[2],
            'outdoor_temperature': row[3],
            'humidity': row[4],
            'co2': row[5],
            'dust': row[6],
        }

    latest = {
        'temperature': latest_primary.get('temperature') if latest_primary else None,
        'outdoor_temperature': None,
        'humidity': latest_primary.get('humidity') if latest_primary else None,
        'co2': latest_primary.get('co2') if latest_primary else None,
        'dust': latest_primary.get('dust') if latest_primary else None,
        'time': latest_primary.get('time') if latest_primary else None,
    }

    if latest_outdoor:
        latest['outdoor_temperature'] = (
            latest_outdoor.get('outdoor_temperature')
            if latest_outdoor.get('outdoor_temperature') is not None
            else latest_outdoor.get('temperature')
        )

    history = []
    last_indoor_temp = None
    last_outdoor_temp = None
    for row in reversed(rows):
        device_id = row[1]
        indoor_temp = row[2]
        outdoor_temp = row[3]

        if is_outdoor_device(device_id):
            last_outdoor_temp = outdoor_temp if outdoor_temp is not None else indoor_temp
        else:
            last_indoor_temp = indoor_temp
            if outdoor_temp is not None:
                last_outdoor_temp = outdoor_temp

        if last_indoor_temp is None and last_outdoor_temp is None:
            continue

        history.append({
            'time': row[0].strftime('%H:%M:%S'),
            'temp': last_indoor_temp,
            'outdoorTemp': last_outdoor_temp,
            'humidity': row[4],
            'co2': row[5],
            'pm25': row[6],
        })

    control_state = None
    if control_row:
        control_state = {
            'time': control_row[0].isoformat(),
            'device_id': control_row[1],
            'power': control_row[2],
            'temp': control_row[3],
            'operationMode': control_row[4],
            'fanPower': control_row[5],
            'clientId': control_row[6] or 'unknown',
            'requestedAt': control_row[7].isoformat() if control_row[7] else control_row[0].isoformat(),
            'lastModifiedAt': control_row[0].isoformat(),
            'lastModifiedBy': control_row[6] or 'unknown',
        }

    return {
        'latest': latest,
        'history': history[-20:],
        'controlState': control_state,
    }

def save_remote_control_state(command):
    with get_db_connection() as control_conn:
        with control_conn.cursor() as control_cur:
            control_cur.execute("""
                INSERT INTO remote_control_state (time, device_id, power, temp, operation_mode, fan_power, client_id, requested_at)
                VALUES (NOW(), %s, %s, %s, %s, %s, %s, %s)
                RETURNING time, requested_at
            """, (
                command['device_id'],
                command['power'],
                command['temp'],
                command['operationMode'],
                command['fanPower'],
                command['clientId'],
                command['requestedAt'],
            ))
            saved_time, requested_at = control_cur.fetchone()

    return {
        **command,
        'time': saved_time.isoformat(),
        'requestedAt': requested_at.isoformat() if requested_at else saved_time.isoformat(),
        'lastModifiedAt': saved_time.isoformat(),
        'lastModifiedBy': command['clientId'],
    }

class TelemetryRequestHandler(BaseHTTPRequestHandler):
    def do_OPTIONS(self):
        self.send_response(204)
        self.send_common_headers()
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path != '/api/telemetry':
            self.send_response(404)
            self.send_common_headers()
            self.end_headers()
            self.wfile.write(json.dumps({'error': 'Not found'}).encode('utf-8'))
            return

        try:
            body = json.dumps(fetch_telemetry()).encode('utf-8')
            self.send_response(200)
            self.send_common_headers()
            self.end_headers()
            self.wfile.write(body)
        except Exception as e:
            self.send_response(500)
            self.send_common_headers()
            self.end_headers()
            self.wfile.write(json.dumps({'error': str(e)}).encode('utf-8'))

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path != '/api/remote-control':
            self.send_response(404)
            self.send_common_headers()
            self.end_headers()
            self.wfile.write(json.dumps({'error': 'Not found'}).encode('utf-8'))
            return

        try:
            content_length = int(self.headers.get('Content-Length', '0'))
            raw_body = self.rfile.read(content_length).decode('utf-8')
            payload = json.loads(raw_body or '{}')

            command = {
                'device_id': payload.get('device_id', 'hvac-01'),
                'power': bool(payload.get('power')),
                'temp': to_float(payload.get('temp')),
                'operationMode': payload.get('operationMode'),
                'fanPower': payload.get('fanPower'),
                'clientId': payload.get('clientId') or 'unknown',
                'requestedAt': payload.get('requestedAt') or time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
            }

            if command['temp'] is None:
                raise ValueError('temp is required')
            if command['operationMode'] not in ['auto', 'cool', 'heat', 'off', 'fan']:
                raise ValueError('operationMode must be auto, cool, heat, off, or fan')
            if command['fanPower'] not in ['auto', 'on', 'off', 'low', 'medium', 'high']:
                raise ValueError('fanPower must be auto, on, off, low, medium, or high')

            result = client.publish(CONTROL_TOPIC, json.dumps(command), qos=1)
            result.wait_for_publish(timeout=3)

            if result.rc != mqtt.MQTT_ERR_SUCCESS:
                raise RuntimeError(f'MQTT publish failed with code {result.rc}')

            saved_command = save_remote_control_state(command)

            body = json.dumps({
                'ok': True,
                'topic': CONTROL_TOPIC,
                'command': saved_command,
            }).encode('utf-8')
            self.send_response(200)
            self.send_common_headers()
            self.end_headers()
            self.wfile.write(body)
        except Exception as e:
            self.send_response(400)
            self.send_common_headers()
            self.end_headers()
            self.wfile.write(json.dumps({'error': str(e)}).encode('utf-8'))

    def send_common_headers(self):
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')

    def log_message(self, format, *args):
        return

def start_api_server():
    server = ThreadingHTTPServer((API_HOST, API_PORT), TelemetryRequestHandler)
    print(f"🌐 Telemetry API listening on {API_HOST}:{API_PORT}")
    server.serve_forever()

# ====================== MQTT CALLBACK (Phiên bản mới) ======================
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("✅ Connected to MQTT Broker!")
        client.subscribe("sensor/#")        # Thay đổi topic nếu cần
    else:
        print(f"❌ Connect failed, code: {rc}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode('utf-8'))
        device_id = payload.get('device_id', 'unknown')

        cur.execute("""
            INSERT INTO sensor_data (time, device_id, temperature, outdoor_temperature, humidity, co2, dust)
            VALUES (NOW(), %s, %s, %s, %s, %s, %s)
        """, (
            device_id,
            to_float(payload.get('temperature')),
            get_outdoor_temperature(payload),
            to_float(payload.get('humidity')),
            to_int(payload.get('co2')),
            to_float(payload.get('dust'))
        ))
        conn.commit()
        
        print(f"✅ Saved → Device: {device_id} | Temp: {payload.get('temperature')} | CO2: {payload.get('co2')}")
        
    except Exception as e:
        print(f"❌ Error processing message: {e} | Topic: {msg.topic}")

# ====================== MAIN ======================
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)   # ← Sửa ở đây
client.on_connect = on_connect
client.on_message = on_message

print("🚀 MQTT Subscriber đang chạy...")
Thread(target=start_api_server, daemon=True).start()
client.connect(MQTT_BROKER, MQTT_PORT, 60)
client.loop_forever()
