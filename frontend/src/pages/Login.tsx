import { useState } from 'react';
import { useNavigate, Link } from 'react-router-dom';
import { useAuthStore } from '@/store/authStore';
import api, { authApi } from '@/api';
import toast from 'react-hot-toast';

export default function Login() {
  const navigate = useNavigate();
  const [formData, setFormData] = useState({
    username: '',
    password: '',
  });
  const [isLoading, setIsLoading] = useState(false);

  const [need2fa, setNeed2fa] = useState(false);
  const [twoFactorToken, setTwoFactorToken] = useState('');
  const [code, setCode] = useState('');

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsLoading(true);
    try {
      let d: any;
      if (need2fa) {
        const res = await api.post('/auth/2fa/verify-login', {
          two_factor_token: twoFactorToken,
          code,
        });
        d = res.data;
      } else {
        const res = await authApi.login(formData.username, formData.password);
        d = res.data;
        if (d.status === 'two_factor_required') {
          setTwoFactorToken(d.two_factor_token);
          setNeed2fa(true);
          return;
        }
      }
      localStorage.setItem('token', d.token);
      useAuthStore.setState({ user: d.user, token: d.token, isAuthenticated: true });
      toast.success('Вход выполнен успешно!');
      navigate('/app/dashboard');
    } catch (error: unknown) {
      const err = error as { response?: { data?: { message?: string } } };
      toast.error(err.response?.data?.message || 'Ошибка входа');
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="min-h-screen bg-dark-900 flex items-center justify-center p-4">
      <Link
        to="/"
        className="absolute top-6 left-6 flex items-center space-x-2 text-gray-400 hover:text-primary-400 transition-colors"
      >
        <span className="text-xl">←</span>
        <span>Вернуться на главную</span>
      </Link>
      <div className="w-full max-w-md">
        <div className="bg-dark-800 rounded-lg shadow-xl p-8 border border-dark-700">
          <div className="text-center mb-8">
            <h1 className="text-4xl font-bold text-primary-400">WTLD</h1>
            <p className="text-gray-400 mt-2">Платформа анализа логов</p>
          </div>

          <form onSubmit={handleSubmit} className="space-y-6">
            <div>
              <label htmlFor="username" className="block text-sm font-medium text-gray-300 mb-2">
                Имя пользователя
              </label>
              <input
                type="text"
                id="username"
                value={formData.username}
                onChange={(e) => setFormData({ ...formData, username: e.target.value })}
                className="w-full px-4 py-3 bg-dark-900 border border-dark-700 rounded-lg text-white placeholder-gray-500 focus:outline-none focus:ring-2 focus:ring-primary-500 focus:border-transparent"
                placeholder="Введите имя пользователя"
                required
              />
            </div>

            <div>
              <label htmlFor="password" className="block text-sm font-medium text-gray-300 mb-2">
                Пароль
              </label>
              <input
                type="password"
                id="password"
                value={formData.password}
                onChange={(e) => setFormData({ ...formData, password: e.target.value })}
                className="w-full px-4 py-3 bg-dark-900 border border-dark-700 rounded-lg text-white placeholder-gray-500 focus:outline-none focus:ring-2 focus:ring-primary-500 focus:border-transparent"
                placeholder="Введите пароль"
                required
              />
            </div>
            {need2fa && (
              <div>
                <label htmlFor="code" className="block text-sm font-medium text-gray-300 mb-2">
                  Код из приложения аутентификации
                </label>
                <input
                  type="text"
                  id="code"
                  value={code}
                  onChange={(e) => setCode(e.target.value.replace(/\D/g, '').slice(0, 6))}
                  className="w-full px-4 py-3 bg-dark-900 border border-dark-700 rounded-lg text-white text-center text-2xl tracking-widest focus:outline-none focus:ring-2 focus:ring-primary-500"
                  placeholder="000000"
                  maxLength={6}
                  required
                />
              </div>
            )}
            <button
              type="submit"
              disabled={isLoading}
              className="w-full py-3 px-4 bg-primary-600 hover:bg-primary-700 text-white font-medium rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {isLoading ? 'Вход...' : need2fa ? 'Подтвердить' : 'Войти'}
            </button>
          </form>

          <p className="mt-6 text-center text-gray-400">
            Нет аккаунта?{' '}
            <Link to="/register" className="text-primary-400 hover:text-primary-300">
              Зарегистрироваться
            </Link>
          </p>
        </div>
      </div>
    </div>
  );
}
