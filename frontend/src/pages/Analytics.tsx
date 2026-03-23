import { useState, useEffect } from 'react';
import { analyticsApi } from '@/api';
import type { AnalysisResult } from '@/types/api';
import toast from 'react-hot-toast';

export default function Analytics() {
  const [analytics, setAnalytics] = useState<AnalysisResult[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [filter, setFilter] = useState<'all' | 'anomalies' | 'by_severity'>('all');
  const [severityFilter, setSeverityFilter] = useState<string>('all');

  useEffect(() => {
    loadAnalytics();
  }, []);

  const loadAnalytics = async () => {
    try {
      // Загружаем все логи и их аналитику
      const response = await analyticsApi.getDashboard();
      // В реальном приложении нужно загружать аналитику для каждого лога
      setAnalytics(response.data.recent_anomalies || []);
    } catch (error) {
      console.error('Failed to load analytics:', error);
      toast.error('Не удалось загрузить аналитику');
    } finally {
      setIsLoading(false);
    }
  };

  const filteredAnalytics = analytics.filter((item) => {
    if (filter === 'anomalies' && !item.is_anomaly) return false;
    if (filter === 'by_severity' && severityFilter !== 'all' && item.severity !== severityFilter)
      return false;
    return true;
  });

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-bold text-white">Аналитика</h1>
        <div className="flex items-center space-x-4">
          <select
            value={filter}
            onChange={(e) => setFilter(e.target.value as typeof filter)}
            className="px-4 py-2 bg-dark-800 border border-dark-700 rounded-lg text-gray-300 focus:outline-none focus:ring-2 focus:ring-primary-500"
          >
            <option value="all">Все записи</option>
            <option value="anomalies">Только аномалии</option>
            <option value="by_severity">По серьезности</option>
          </select>

          {filter === 'by_severity' && (
            <select
              value={severityFilter}
              onChange={(e) => setSeverityFilter(e.target.value)}
              className="px-4 py-2 bg-dark-800 border border-dark-700 rounded-lg text-gray-300 focus:outline-none focus:ring-2 focus:ring-primary-500"
            >
              <option value="all">Все уровни</option>
              <option value="critical">Критический</option>
              <option value="high">Высокий</option>
              <option value="medium">Средний</option>
              <option value="low">Низкий</option>
            </select>
          )}
        </div>
      </div>

      {isLoading ? (
        <div className="flex items-center justify-center h-64">
          <div className="text-gray-400">Загрузка...</div>
        </div>
      ) : filteredAnalytics.length === 0 ? (
        <div className="bg-dark-800 rounded-lg p-12 border border-dark-700 text-center">
          <span className="text-6xl mb-4 block">📈</span>
          <h3 className="text-xl font-semibold text-white mb-2">Нет данных аналитики</h3>
          <p className="text-gray-400">
            Загрузите логи для получения аналитики или измените фильтры
          </p>
        </div>
      ) : (
        <div className="space-y-4">
          {filteredAnalytics.map((result) => (
            <AnalyticsCard key={result.id} result={result} />
          ))}
        </div>
      )}
    </div>
  );
}

function AnalyticsCard({ result }: { result: AnalysisResult }) {
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

  return (
    <div
      className={`bg-dark-800 rounded-lg p-6 border border-dark-700 border-l-4 ${
        severityColors[result.severity] || severityColors.low
      }`}
    >
      <div className="flex items-start justify-between">
        <div className="flex-1">
          <div className="flex items-center space-x-3">
            <span className="text-2xl">
              {result.type === 'error_rate'
                ? '📊'
                : result.type === 'anomaly'
                ? '🚨'
                : result.type === 'pattern_match'
                ? '🔍'
                : '📋'}
            </span>
            <h3 className="text-lg font-semibold text-white">{result.title}</h3>
            <span
              className={`px-2 py-1 rounded text-xs font-medium ${
                badgeColors[result.severity] || badgeColors.low
              }`}
            >
              {result.severity === 'critical'
                ? 'Критический'
                : result.severity === 'high'
                ? 'Высокий'
                : result.severity === 'medium'
                ? 'Средний'
                : 'Низкий'}
            </span>
            {result.is_anomaly && (
              <span className="px-2 py-1 rounded text-xs font-medium bg-purple-500/10 text-purple-400">
                Аномалия
              </span>
            )}
          </div>
          <p className="text-gray-400 mt-3">{result.description}</p>

          {result.data && Object.keys(result.data).length > 0 && (
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
          <p className="text-xs text-gray-600 mt-1">Тип: {result.type}</p>
        </div>
      </div>
    </div>
  );
}
