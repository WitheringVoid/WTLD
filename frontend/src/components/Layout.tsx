import { Outlet, Link, useLocation, useNavigate } from 'react-router-dom';
import { useAuthStore } from '@/store/authStore';
import { useWebSocketStore } from '@/store/websocketStore';
import { useEffect } from 'react';
import toast from 'react-hot-toast';

export default function Layout() {
  const location = useLocation();
  const navigate = useNavigate();
  const { user, logout } = useAuthStore();
  const { events, clearEvents } = useWebSocketStore();

  const navItems = [
    { path: '/dashboard', label: 'Дашборд', icon: '📊' },
    { path: '/logs', label: 'Логи', icon: '📋' },
    { path: '/analytics', label: 'Аналитика', icon: '📈' },
    { path: '/settings', label: 'Настройки', icon: '⚙️' },
  ];

  // Обработка WebSocket событий
  useEffect(() => {
    if (events.length > 0) {
      const lastEvent = events[events.length - 1];

      switch (lastEvent.type) {
        case 'ANOMALY_DETECTED':
          toast.error(`🚨 ${lastEvent.title}: ${lastEvent.message}`);
          break;
        case 'ANALYSIS_COMPLETE':
          toast.success(`✅ ${lastEvent.title}: ${lastEvent.message}`);
          break;
        case 'SYSTEM_NOTIFICATION':
          toast(`ℹ️ ${lastEvent.title}`);
          break;
      }

      clearEvents();
    }
  }, [events, clearEvents]);

  const handleLogout = () => {
    logout();
    navigate('/login');
  };

  return (
    <div className="min-h-screen bg-dark-900 text-gray-100">
      {/* Sidebar */}
      <aside className="fixed left-0 top-0 h-full w-64 bg-dark-800 border-r border-dark-700">
        <div className="p-6">
          <h1 className="text-2xl font-bold text-primary-400">WTLD</h1>
          <p className="text-sm text-gray-400 mt-1">Анализ логов</p>
        </div>

        <nav className="mt-6">
          {navItems.map((item) => (
            <Link
              key={item.path}
              to={item.path}
              className={`flex items-center px-6 py-3 text-gray-300 hover:bg-dark-700 hover:text-white transition-colors ${location.pathname === item.path ? 'bg-dark-700 text-white border-l-4 border-primary-500' : ''
                }`}
            >
              <span className="mr-3">{item.icon}</span>
              {item.label}
            </Link>
          ))}
        </nav>

        <div className="absolute bottom-0 left-0 right-0 p-6 border-t border-dark-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="font-medium text-white">{user?.username || 'Пользователь'}</p>
              <p className="text-sm text-gray-400">{user?.email || ''}</p>
            </div>
            <button
              onClick={handleLogout}
              className="text-gray-400 hover:text-red-400 transition-colors"
              title="Выйти"
            >
              🚪
            </button>
          </div>
        </div>
      </aside>

      {/* Main content */}
      <main className="ml-64 min-h-screen">
        <header className="bg-dark-800 border-b border-dark-700 px-8 py-4">
          <div className="flex items-center justify-between">
            <h2 className="text-xl font-semibold text-white">
              {navItems.find((item) => item.path === location.pathname)?.label || 'WTLD'}
            </h2>
            <div className="flex items-center space-x-4">
              <span className="text-sm text-gray-400">
                {new Date().toLocaleDateString('ru-RU', {
                  weekday: 'long',
                  year: 'numeric',
                  month: 'long',
                  day: 'numeric',
                })}
              </span>
            </div>
          </div>
        </header>

        <div className="p-8">
          <Outlet />
        </div>
      </main>
    </div>
  );
}
