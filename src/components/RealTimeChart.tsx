import React from 'react';
import { 
  ResponsiveContainer, 
  AreaChart, 
  Area, 
  XAxis, 
  YAxis, 
  CartesianGrid, 
  Tooltip,
  Legend
} from 'recharts';
import { ChartDataPoint } from '../types';

interface RealTimeChartProps {
  data: ChartDataPoint[];
}

const CustomTooltip = ({ active, payload, label }: any) => {
  if (active && payload && payload.length) {
    return (
      <div className="bg-white border border-slate-200 p-3 rounded-lg shadow-xl outline-none">
        <p className="text-xs text-slate-400 mb-2 font-mono">{label}</p>
        <div className="space-y-1">
          {payload.map((entry: any, index: number) => (
            <div key={index} className="flex items-center justify-between gap-8">
              <span className="text-xs font-bold text-slate-600 uppercase tracking-tighter">
                {entry.name}
              </span>
              <span className="text-xs font-mono font-bold" style={{ color: entry.color }}>
                {typeof entry.value === 'number' ? entry.value.toFixed(1) : '--'}
              </span>
            </div>
          ))}
        </div>
      </div>
    );
  }
  return null;
};

export const RealTimeChart: React.FC<RealTimeChartProps> = ({ data }) => {
  return (
    <div className="w-full h-full min-h-[300px] bg-white rounded-lg p-4 border border-slate-200 shadow-sm">
      <div className="flex justify-between items-center mb-6">
        <div>
          <h3 className="text-xs font-bold uppercase tracking-wider text-slate-400">Environment Trends</h3>
          <p className="text-[10px] text-slate-500 font-medium">Real-time telemetry (last 20 cycles)</p>
        </div>
      </div>

      <div className="h-[250px] w-full">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={data} margin={{ top: 10, right: 10, left: -20, bottom: 0 }}>
            <defs>
              <linearGradient id="colorTemp" x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor="#3b82f6" stopOpacity={0.1}/>
                <stop offset="95%" stopColor="#3b82f6" stopOpacity={0}/>
              </linearGradient>
              <linearGradient id="colorOutdoorTemp" x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor="#10b981" stopOpacity={0.1}/>
                <stop offset="95%" stopColor="#10b981" stopOpacity={0}/>
              </linearGradient>
            </defs>
            <CartesianGrid strokeDasharray="3 3" vertical={false} stroke="#f1f5f9" opacity={1} />
            <XAxis 
              dataKey="time" 
              hide 
            />
            <YAxis 
              stroke="#94a3b8" 
              fontSize={10} 
              tickLine={false} 
              axisLine={false} 
              tickFormatter={(val) => `${val}`}
            />
            <Tooltip content={<CustomTooltip />} />
            <Legend 
              verticalAlign="bottom" 
              height={36} 
              iconType="circle" 
              wrapperStyle={{ fontSize: '10px', textTransform: 'uppercase', letterSpacing: '0.05em', fontWeight: 600, color: '#64748b', paddingTop: '20px' }}
            />
            <Area
              type="monotone"
              dataKey="temp"
              name="Indoor Temperature"
              stroke="#3b82f6"
              strokeWidth={2}
              fillOpacity={1}
              fill="url(#colorTemp)"
              isAnimationActive={false}
            />
            <Area
              type="monotone"
              dataKey="outdoorTemp"
              name="Outdoor Temperature"
              stroke="#10b981"
              strokeWidth={2}
              fillOpacity={1}
              fill="url(#colorOutdoorTemp)"
              isAnimationActive={false}
            />
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
};
