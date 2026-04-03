import { useState, useEffect } from 'react';
import { useParams, Link } from 'react-router-dom';
import { logsApi, analyticsApi } from '@/api';
import type { LogFile, AnalysisResult } from '@/types/api';
import toast from 'react-hot-toast';

export default function LogDetail() {
  const { id } = useParams<{ id: string }>();
  const [log, setLog] = useState<LogFile | null>(null);
  const [analytics, setAnalytics] = useState<AnalysisResult[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [activeTab, setActiveTab] = useState<'entries' | 'analytics'>('entries');

  useEffect(() => {
    if (id) {
      loadLogDetails(id);
    }
  }, [id]);

  const loadLogDetails = async (logId: string) => {
    try {
      const [logResponse, analyticsResponse] = await Promise.all([
        logsApi.getLogById(parseInt(logId)),
        analyticsApi.getAnalytics(parseInt(logId)),
      ]);
      setLog(logResponse.data.data);
      setAnalytics(analyticsResponse.data.data);
    } catch (error) {
      console.error('Failed to load log details:', error);
      toast.error('Не удалось загрузить детали лога');
    } finally {
      setIsLoading(false);
    }
  };

  if (isLoading) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="text-gray-400">Загрузка...</div>
      </div>
    );
  }

  if (!log) {
    return (
      <div className="text-center py-12">
        <h2 className="text-xl font-semibold text-white mb-4">Лог не найден</h2>
        <Link to="/logs" className="text-primary-400 hover:text-primary-300">
          ← Вернуться к списку
        </Link>
      </div>
    );
  }

  const getLevelColor = (level: string) => {
    const colors: Record<string, string> = {
      DEBUG: 'text-gray-400',
      INFO: 'text-green-400',
      WARNING: 'text-yellow-400',
      ERROR: 'text-red-400',
      CRITICAL: 'text-purple-400',
      FATAL: 'text-red-500',
    };
    return colors[level] || 'text-gray-300';
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <Link to="/logs" className="text-gray-400 hover:text-white text-sm">
            ← Назад к списку
          </Link>
          <h1 className="text-2xl font-bold text-white mt-2">{log.filename}</h1>
        </div>
        <div className="flex items-center space-x-4">
          <span className="px-3 py-1 bg-dark-900 rounded text-sm text-gray-300">
            {log.file_type.toUpperCase()}
          </span>
          <span className="text-gray-400 text-sm">
            {new Date(log.upload_date).toLocaleString('ru-RU')}
          </span>
        </div>
      </div>

      {/* Вкладки */}
      <div className="flex space-x-4 border-b border-dark-700">
        <button
          onClick={() => setActiveTab('entries')}
          className={`px-4 py-2 font-medium transition-colors ${activeTab === 'entries'
              ? 'text-primary-400 border-b-2 border-primary-400'
              : 'text-gray-400 hover:text-white'
            }`}
        >
          Записи ({log.entries?.length || 0})
        </button>
        <button
          onClick={() => setActiveTab('analytics')}
          className={`px-4 py-2 font-medium transition-colors ${activeTab === 'analytics'
              ? 'text-primary-400 border-b-2 border-primary-400'
              : 'text-gray-400 hover:text-white'
            }`}
        >
          Аналитика ({analytics.length})
        </button>
      </div>

      {activeTab === 'entries' && log.entries && (
        <div className="bg-dark-800 rounded-lg border border-dark-700 overflow-hidden">
          <div className="max-h-[600px] overflow-y-auto">
            {log.entries.map((entry, index) => (
              <div
                key={index}
                className="p-4 border-b border-dark-700 hover:bg-dark-750 transition-colors"
              >
                <div className="flex items-start space-x-4">
                  <span className={`font-mono text-sm ${getLevelColor(entry.level)}`}>
                    [{entry.level}]
                  </span>
                  <span className="text-gray-500 text-sm font-mono">{entry.timestamp}</span>
                  {entry.source && (
                    <span className="text-blue-400 text-sm">{entry.source}</span>
                  )}
                </div>
                <p className="text-gray-300 mt-2 ml-24">{entry.message}</p>
                {entry.metadata && Object.keys(entry.metadata).length > 0 && (
                  <pre className="mt-2 ml-24 text-xs text-gray-500 bg-dark-900 p-2 rounded">
                    {JSON.stringify(entry.metadata, null, 2)}
                  </pre>
                )}
              </div>
            ))}
          </div>
        </div>
      )}

      {activeTab === 'analytics' && (
        <div className="space-y-4">
          {analytics.length === 0 ? (
            <div className="bg-dark-800 rounded-lg p-12 border border-dark-700 text-center">
              <span className="text-6xl mb-4 block">📊</span>
              <h3 className="text-xl font-semibold text-white mb-2">Нет данных аналитики</h3>
              <p className="text-gray-400">Анализ еще не завершен или не обнаружено значимых паттернов</p>
            </div>
          ) : (
            analytics.map((result) => (
              <AnalyticsCard key={result.id} result={result} />
            ))
          )}
        </div>
      )}
    </div>
  );
}

function AnalyticsCard({ result }: { result: AnalysisResult }) {
  const severityColors: Record<string, string> = {
    critical: 'border-red-500 bg-red-500/5',
    high: 'border-orange-500 bg-orange-500/5',
    medium: 'border-yellow-500 bg-yellow-500/5',
    low: 'border-green-500 bg-green-500/5',
  };

  const badgeColors: Record<string, string> = {
    critical: 'bg-red-500/10 text-red-400',
    high: 'bg-orange-500/10 text-orange-400',
    medium: 'bg-yellow-500/10 text-yellow-400',
    low: 'bg-green-500/10 text-green-400',
  };

  return (
    <div
      className={`p-6 rounded-lg border-l-4 ${severityColors[result.severity] || severityColors.low}`}
    >
      <div className="flex items-start justify-between">
        <div>
          <div className="flex items-center space-x-3">
            <h3 className="text-lg font-semibold text-white">{result.title}</h3>
            <span
              className={`px-2 py-1 rounded text-xs font-medium ${badgeColors[result.severity] || badgeColors.low
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
                🚨 Аномалия
              </span>
            )}
          </div>
          <p className="text-gray-400 mt-2">{result.description}</p>
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
        <span className="text-gray-500 text-sm">
          {new Date(result.created_at).toLocaleString('ru-RU')}
        </span>
      </div>
    </div>
  );
}
