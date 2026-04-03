import { useState, useEffect } from 'react';
import { analyticsApi } from '@/api';
import type { DashboardData } from '@/types/api';
import { Link } from 'react-router-dom';

export default function Dashboard() {
  const [data, setData] = useState<DashboardData | null>(null);
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    loadDashboard();
  }, []);

  const loadDashboard = async () => {
    try {
      const response = await analyticsApi.getDashboard();
      setData(response.data);
    } catch (error) {
      console.error('Failed to load dashboard:', error);
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

  return (
    <div className="space-y-6">
      <h1 className="text-2xl font-bold text-white">Дашборд</h1>

      {/* Статистика */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <StatCard
          title="Всего логов"
          value={data?.statistics.total_logs || 0}
          icon="📁"
          color="primary"
        />
        <StatCard
          title="Обработано"
          value={data?.statistics.completed_logs || 0}
          icon="✅"
          color="green"
        />
        <StatCard
          title="Аномалии"
          value={data?.statistics.total_anomalies || 0}
          icon="⚠️"
          color="red"
        />
        <StatCard
          title="Правила"
          value={data?.statistics.active_rules || 0}
          icon="📋"
          color="blue"
        />
      </div>

      {/* Распределение по серьезности */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        <div className="bg-dark-800 rounded-lg p-6 border border-dark-700">
          <h2 className="text-lg font-semibold text-white mb-4">Распределение по серьезности</h2>
          <div className="space-y-4">
            <SeverityBar
              label="Критический"
              count={data?.severity_distribution.critical || 0}
              color="bg-red-500"
            />
            <SeverityBar
              label="Высокий"
              count={data?.severity_distribution.high || 0}
              color="bg-orange-500"
            />
            <SeverityBar
              label="Средний"
              count={data?.severity_distribution.medium || 0}
              color="bg-yellow-500"
            />
            <SeverityBar
              label="Низкий"
              count={data?.severity_distribution.low || 0}
              color="bg-green-500"
            />
          </div>
        </div>

        {/* Последние аномалии */}
        <div className="bg-dark-800 rounded-lg p-6 border border-dark-700">
          <h2 className="text-lg font-semibold text-white mb-4">Последние аномалии</h2>
          <div className="space-y-3">
            {data?.recent_anomalies.length ? (
              data.recent_anomalies.slice(0, 5).map((anomaly) => (
                <div
                  key={anomaly.id}
                  className="p-3 bg-dark-900 rounded-lg border border-dark-700"
                >
                  <div className="flex items-center justify-between">
                    <span className="font-medium text-white">{anomaly.title}</span>
                    <SeverityBadge severity={anomaly.severity} />
                  </div>
                  <p className="text-sm text-gray-400 mt-1">{anomaly.description}</p>
                  <p className="text-xs text-gray-500 mt-2">
                    {new Date(anomaly.created_at).toLocaleString('ru-RU')}
                  </p>
                </div>
              ))
            ) : (
              <p className="text-gray-400 text-center py-4">Аномалий не обнаружено</p>
            )}
          </div>
        </div>
      </div>

      {/* Быстрые действия */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <Link
          to="/logs"
          className="bg-dark-800 rounded-lg p-6 border border-dark-700 hover:border-primary-500 transition-colors group"
        >
          <div className="flex items-center space-x-4">
            <span className="text-3xl">📤</span>
            <div>
              <h3 className="font-semibold text-white group-hover:text-primary-400 transition-colors">
                Загрузить логи
              </h3>
              <p className="text-sm text-gray-400">Добавить новые файлы логов</p>
            </div>
          </div>
        </Link>

        <Link
          to="/analytics"
          className="bg-dark-800 rounded-lg p-6 border border-dark-700 hover:border-primary-500 transition-colors group"
        >
          <div className="flex items-center space-x-4">
            <span className="text-3xl">📊</span>
            <div>
              <h3 className="font-semibold text-white group-hover:text-primary-400 transition-colors">
                Аналитика
              </h3>
              <p className="text-sm text-gray-400">Просмотр аналитики</p>
            </div>
          </div>
        </Link>

        <Link
          to="/settings"
          className="bg-dark-800 rounded-lg p-6 border border-dark-700 hover:border-primary-500 transition-colors group"
        >
          <div className="flex items-center space-x-4">
            <span className="text-3xl">⚙️</span>
            <div>
              <h3 className="font-semibold text-white group-hover:text-primary-400 transition-colors">
                Настройки
              </h3>
              <p className="text-sm text-gray-400">Управление правилами</p>
            </div>
          </div>
        </Link>
      </div>
    </div>
  );
}

function StatCard({
  title,
  value,
  icon,
  color,
}: {
  title: string;
  value: number;
  icon: string;
  color: 'primary' | 'green' | 'red' | 'blue';
}) {
  const colorClasses = {
    primary: 'bg-primary-500/10 text-primary-400',
    green: 'bg-green-500/10 text-green-400',
    red: 'bg-red-500/10 text-red-400',
    blue: 'bg-blue-500/10 text-blue-400',
  };

  return (
    <div className="bg-dark-800 rounded-lg p-6 border border-dark-700">
      <div className="flex items-center justify-between">
        <div>
          <p className="text-gray-400 text-sm">{title}</p>
          <p className="text-3xl font-bold text-white mt-2">{value}</p>
        </div>
        <div className={`w-12 h-12 rounded-lg flex items-center justify-center ${colorClasses[color]}`}>
          <span className="text-2xl">{icon}</span>
        </div>
      </div>
    </div>
  );
}

function SeverityBar({
  label,
  count,
  color,
}: {
  label: string;
  count: number;
  color: string;
}) {
  return (
    <div>
      <div className="flex items-center justify-between mb-1">
        <span className="text-sm text-gray-300">{label}</span>
        <span className="text-sm font-medium text-white">{count}</span>
      </div>
      <div className="h-2 bg-dark-900 rounded-full overflow-hidden">
        <div
          className={`h-full ${color} transition-all duration-500`}
          style={{ width: `${count}%` }}
        />
      </div>
    </div>
  );
}

function SeverityBadge({ severity }: { severity: string }) {
  const colors: Record<string, string> = {
    critical: 'bg-red-500/10 text-red-400 border-red-500/20',
    high: 'bg-orange-500/10 text-orange-400 border-orange-500/20',
    medium: 'bg-yellow-500/10 text-yellow-400 border-yellow-500/20',
    low: 'bg-green-500/10 text-green-400 border-green-500/20',
  };

  return (
    <span
      className={`px-2 py-1 rounded text-xs font-medium border ${colors[severity] || colors.low}`}
    >
      {severity === 'critical' ? 'Крит.' : severity === 'high' ? 'Высок.' : severity === 'medium' ? 'Сред.' : 'Низк.'}
    </span>
  );
}
