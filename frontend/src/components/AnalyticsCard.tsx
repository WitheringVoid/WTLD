import type { AnalysisResult } from '@/types/api';

const severityColors: Record<string, string> = {
  critical: 'border-red-500',
  high: 'border-orange-500',
  medium: 'border-yellow-500',
  low: 'border-green-500',
};

const badgeColors: Record<string, string> = {
  critical: 'bg-red-500/10 text-red-400',
  high: 'bg-orange-500/10 text-orange-400',
  medium: 'bg-yellow-500/10 text-yellow-400',
  low: 'bg-green-500/10 text-green-400',
};

const severityLabels: Record<string, string> = {
  critical: 'Критический',
  high: 'Высокий',
  medium: 'Средний',
  low: 'Низкий',
};

const typeIcons: Record<string, string> = {
  error_rate: '📊',
  anomaly: '🚨',
  pattern_match: '🔍',
};

interface AnalyticsCardProps {
  result: AnalysisResult;
  variant?: 'default' | 'detailed';
}

export default function AnalyticsCard({ result, variant = 'default' }: AnalyticsCardProps) {
  return (
    <div
      className={`bg-dark-800 rounded-lg p-6 border border-dark-700 border-l-4 ${
        severityColors[result.severity] || severityColors.low
      }`}
    >
      <div className="flex items-start justify-between">
        <div className="flex-1">
          <div className="flex items-center space-x-3">
            {variant === 'detailed' && (
              <span className="text-2xl">
                {typeIcons[result.type] || '📋'}
              </span>
            )}
            <h3 className="text-lg font-semibold text-white">{result.title}</h3>
            <span
              className={`px-2 py-1 rounded text-xs font-medium ${
                badgeColors[result.severity] || badgeColors.low
              }`}
            >
              {severityLabels[result.severity] || severityLabels.low}
            </span>
            {result.is_anomaly && (
              <span className="px-2 py-1 rounded text-xs font-medium bg-purple-500/10 text-purple-400">
                🚨 Аномалия
              </span>
            )}
          </div>
          <p className="text-gray-400 mt-2">{result.description}</p>

          {variant === 'detailed' && result.data && Object.keys(result.data).length > 0 && (
            <div className="mt-4 p-4 bg-dark-900 rounded-lg">
              <h4 className="text-sm font-medium text-gray-400 mb-2">Данные:</h4>
              <pre className="text-xs text-gray-500 overflow-x-auto">
                {JSON.stringify(result.data, null, 2)}
              </pre>
            </div>
          )}

          {result.detected_patterns && result.detected_patterns.length > 0 && (
            <div className="mt-3 flex flex-wrap gap-2">
              {result.detected_patterns.map((pattern, idx) => (
                <span
                  key={idx}
                  className="px-2 py-1 bg-dark-900 rounded text-xs text-gray-400 font-mono"
                >
                  {pattern}
                </span>
              ))}
            </div>
          )}
        </div>

        <div className="text-right">
          <span className="text-gray-500 text-sm">
            {new Date(result.created_at).toLocaleString('ru-RU')}
          </span>
          {variant === 'detailed' && (
            <p className="text-xs text-gray-600 mt-1">Тип: {result.type}</p>
          )}
        </div>
      </div>
    </div>
  );
}
