export type Status = 'good' | 'warning' | 'critical' | 'active';

export interface SensorReading {
  id: string;
  name: string;
  value: number;
  unit: string;
  status: Status;
  trend: number; // percentage change
  icon: string;
}

export interface ChartDataPoint {
  time: string;
  temp: number | null;
  outdoorTemp: number | null;
  co2: number | null;
  pm25: number | null;
}

export interface TelemetryResponse {
  latest: {
    temperature: number | null;
    outdoor_temperature: number | null;
    humidity: number | null;
    co2: number | null;
    dust: number | null;
    time: string | null;
  };
  history: ChartDataPoint[];
  controlState: RemoteControlState | null;
}

export interface HVACState {
  power: boolean;
  mode: 'auto' | 'cool' | 'heat' | 'off';
  targetTemp: number;
  fanSpeed: 'auto' | 'on' | 'off';
}

export interface RemoteControlPayload {
  device_id: string;
  power: boolean;
  temp: number;
  operationMode: HVACState['mode'];
  fanPower: HVACState['fanSpeed'];
  clientId: string;
  requestedAt: string;
}

export interface RemoteControlState extends RemoteControlPayload {
  time: string;
  lastModifiedAt: string;
  lastModifiedBy: string;
}

export interface RemoteControlResponse {
  ok: boolean;
  topic: string;
  command: RemoteControlState;
}
