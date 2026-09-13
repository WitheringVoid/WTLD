import { BrowserRouter, Routes, Route, Navigate, useParams } from 'react-router-dom'; // 1. Добавлен useParams
import { Toaster } from 'react-hot-toast';
import { useAuthStore } from '@/store/authStore';
import { useWebSocketStore } from '@/store/websocketStore';
import { useEffect } from 'react';

// Страницы
import Home from '@/pages/Home';
import Login from '@/pages/Login';
import Register from '@/pages/Register';
import Dashboard from '@/pages/Dashboard';
import Logs from '@/pages/Logs';
import LogDetail from '@/pages/LogDetail';
import Analytics from '@/pages/Analytics';
import Settings from '@/pages/Settings';
import TwoFASetup from '@/pages/TwoFASetup';

// Компоненты
import Layout from '@/components/Layout';

// Защищенный маршрут
function ProtectedRoute({ children }: { children: React.ReactNode }) {
  const { isAuthenticated } = useAuthStore();
  return isAuthenticated ? <>{children}</> : <Navigate to="/login" replace />;
}

// Если авторизован — редирект на дашборд
function PublicOnly({ children }: { children: React.ReactNode }) {
  const { isAuthenticated } = useAuthStore();
  return isAuthenticated ? <Navigate to="/app/dashboard" replace /> : <>{children}</>;
}

// Перехватывает старый вид ссылки /logs/:id и меняет на /app/logs/:id
function LegacyLogRedirect() {
  const params = useParams();
  return <Navigate to={`/app/logs/${params.id}`} replace />;
}

function App() {
  const { token, isAuthenticated } = useAuthStore();
  const { connect, disconnect } = useWebSocketStore();

  useEffect(() => {
    if (isAuthenticated && token) {
      connect(token);
    }
    return () => {
      disconnect();
    };
  }, [isAuthenticated, token, connect, disconnect]);

  return (
    <BrowserRouter>
      <Toaster
        position="top-right"
        toastOptions={{
          duration: 4000,
          style: {
            background: '#1e293b',
            color: '#f1f5f9',
          },
          success: {
            iconTheme: {
              primary: '#10b981',
              secondary: '#f1f5f9',
            },
          },
          error: {
            iconTheme: {
              primary: '#ef4444',
              secondary: '#f1f5f9',
            },
          },
        }}
      />
      <Routes>
        {/* Публичные маршруты */}
        <Route path="/" element={<Home />} />
        <Route path="/login" element={<PublicOnly><Login /></PublicOnly>} />
        <Route path="/register" element={<PublicOnly><Register /></PublicOnly>} />

        {/* Страховочные редиректы со старых адресов на новые */}
        <Route path="/dashboard" element={<Navigate to="/app/dashboard" replace />} />
        <Route path="/logs" element={<Navigate to="/app/logs" replace />} />
        <Route path="/logs/:id" element={<LegacyLogRedirect />} />
        <Route path="/analytics" element={<Navigate to="/app/analytics" replace />} />
        <Route path="/settings" element={<Navigate to="/app/settings" replace />} />

        {/* Защищенные маршруты внутри Layout */}
        <Route
          path="/app"
          element={
            <ProtectedRoute>
              <Layout />
            </ProtectedRoute>
          }
        >
          <Route index element={<Navigate to="/app/dashboard" replace />} />
          <Route path="dashboard" element={<Dashboard />} />
          <Route path="logs" element={<Logs />} />
          <Route path="logs/:id" element={<LogDetail />} />
          <Route path="analytics" element={<Analytics />} />
          <Route path="settings" element={<Settings />} />
          <Route path="2fa/setup" element={<TwoFASetup />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}

export default App;