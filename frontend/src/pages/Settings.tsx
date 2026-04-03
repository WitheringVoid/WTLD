import { useState, useEffect } from 'react';
import { analyticsApi, twoFAApi } from '@/api';
import type { AnalysisRule } from '@/types/api';
import toast from 'react-hot-toast';
import { useNavigate } from 'react-router-dom';

export default function Settings() {
  const [activeTab, setActiveTab] = useState<'rules' | '2fa'>('rules');
  const [rules, setRules] = useState<AnalysisRule[]>([]);
  const [is2FAEnabled, setIs2FAEnabled] = useState(false);
  const [is2FAConfigured, setIs2FAConfigured] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const [isCreatingRule, setIsCreatingRule] = useState(false);
  const [newRule, setNewRule] = useState({
    name: '',
    type: 'regex' as 'regex' | 'keyword' | 'threshold' | 'custom',
    pattern: '',
    severity: 'medium' as 'low' | 'medium' | 'high' | 'critical',
    is_active: true,
  });

  const navigate = useNavigate();

  useEffect(() => {
    loadSettings();
  }, []);

  const loadSettings = async () => {
    try {
      const [rulesResponse, twoFAResponse] = await Promise.all([
        analyticsApi.getRules(),
        twoFAApi.getStatus(),
      ]);
      setRules(rulesResponse.data.data);
      setIs2FAEnabled(twoFAResponse.data.data.is_enabled);
      setIs2FAConfigured(twoFAResponse.data.data.is_configured);
    } catch (error) {
      console.error('Failed to load settings:', error);
      toast.error('Не удалось загрузить настройки');
    } finally {
      setIsLoading(false);
    }
  };

  const handleCreateRule = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsCreatingRule(true);

    try {
      await analyticsApi.createRule(newRule);
      toast.success('Правило создано');
      setNewRule({ name: '', type: 'regex', pattern: '', severity: 'medium', is_active: true });
      loadSettings();
    } catch (error) {
      toast.error('Не удалось создать правило');
    } finally {
      setIsCreatingRule(false);
    }
  };

  const handleDeleteRule = async (id: number) => {
    if (!confirm('Вы уверены, что хотите удалить это правило?')) return;

    try {
      await analyticsApi.deleteRule(id);
      toast.success('Правило удалено');
      loadSettings();
    } catch (error) {
      toast.error('Не удалось удалить правило');
    }
  };

  const handle2FASetup = () => {
    navigate('/2fa/setup');
  };

  const handle2FADisable = async () => {
    if (!confirm('Вы уверены, что хотите отключить 2FA?')) return;

    try {
      await twoFAApi.disable();
      toast.success('2FA отключен');
      loadSettings();
    } catch (error) {
      toast.error('Не удалось отключить 2FA');
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
      <h1 className="text-2xl font-bold text-white">Настройки</h1>

      {/* Вкладки */}
      <div className="flex space-x-4 border-b border-dark-700">
        <button
          onClick={() => setActiveTab('rules')}
          className={`px-4 py-2 font-medium transition-colors ${activeTab === 'rules'
            ? 'text-primary-400 border-b-2 border-primary-400'
            : 'text-gray-400 hover:text-white'
            }`}
        >
          Правила анализа
        </button>
        <button
          onClick={() => setActiveTab('2fa')}
          className={`px-4 py-2 font-medium transition-colors ${activeTab === '2fa'
            ? 'text-primary-400 border-b-2 border-primary-400'
            : 'text-gray-400 hover:text-white'
            }`}
        >
          Двухфакторная аутентификация
        </button>
      </div>

      {activeTab === 'rules' && (
        <div className="space-y-6">
          {/* Форма создания правила */}
          <div className="bg-dark-800 rounded-lg p-6 border border-dark-700">
            <h2 className="text-lg font-semibold text-white mb-4">Создать правило</h2>
            <form onSubmit={handleCreateRule} className="space-y-4">
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div>
                  <label className="block text-sm font-medium text-gray-300 mb-2">
                    Название
                  </label>
                  <input
                    type="text"
                    value={newRule.name}
                    onChange={(e) => setNewRule({ ...newRule, name: e.target.value })}
                    className="w-full px-4 py-2 bg-dark-900 border border-dark-700 rounded-lg text-white focus:outline-none focus:ring-2 focus:ring-primary-500"
                    placeholder="Например: Ошибки БД"
                    required
                  />
                </div>

                <div>
                  <label className="block text-sm font-medium text-gray-300 mb-2">
                    Тип
                  </label>
                  <select
                    value={newRule.type}
                    onChange={(e) =>
                      setNewRule({ ...newRule, type: e.target.value as typeof newRule.type })
                    }
                    className="w-full px-4 py-2 bg-dark-900 border border-dark-700 rounded-lg text-white focus:outline-none focus:ring-2 focus:ring-primary-500"
                  >
                    <option value="regex">Regex</option>
                    <option value="keyword">Ключевое слово</option>
                    <option value="threshold">Пороговое значение</option>
                    <option value="custom">Пользовательский</option>
                  </select>
                </div>

                <div className="md:col-span-2">
                  <label className="block text-sm font-medium text-gray-300 mb-2">
                    Паттерн
                  </label>
                  <input
                    type="text"
                    value={newRule.pattern}
                    onChange={(e) => setNewRule({ ...newRule, pattern: e.target.value })}
                    className="w-full px-4 py-2 bg-dark-900 border border-dark-700 rounded-lg text-white focus:outline-none focus:ring-2 focus:ring-primary-500"
                    placeholder={newRule.type === 'regex' ? '.*error.*' : 'ключевое слово'}
                    required
                  />
                </div>

                <div>
                  <label className="block text-sm font-medium text-gray-300 mb-2">
                    Серьезность
                  </label>
                  <select
                    value={newRule.severity}
                    onChange={(e) =>
                      setNewRule({ ...newRule, severity: e.target.value as typeof newRule.severity })
                    }
                    className="w-full px-4 py-2 bg-dark-900 border border-dark-700 rounded-lg text-white focus:outline-none focus:ring-2 focus:ring-primary-500"
                  >
                    <option value="low">Низкий</option>
                    <option value="medium">Средний</option>
                    <option value="high">Высокий</option>
                    <option value="critical">Критический</option>
                  </select>
                </div>
              </div>

              <button
                type="submit"
                disabled={isCreatingRule}
                className="px-6 py-2 bg-primary-600 hover:bg-primary-700 text-white font-medium rounded-lg transition-colors disabled:opacity-50"
              >
                {isCreatingRule ? 'Создание...' : 'Создать правило'}
              </button>
            </form>
          </div>

          {/* Список правил */}
          <div className="bg-dark-800 rounded-lg border border-dark-700 overflow-hidden">
            <div className="px-6 py-4 border-b border-dark-700">
              <h2 className="text-lg font-semibold text-white">Существующие правила</h2>
            </div>
            {rules.length === 0 ? (
              <div className="p-12 text-center text-gray-400">
                Нет созданных правил
              </div>
            ) : (
              <table className="w-full">
                <thead className="bg-dark-900">
                  <tr>
                    <th className="px-6 py-3 text-left text-sm font-medium text-gray-400">
                      Название
                    </th>
                    <th className="px-6 py-3 text-left text-sm font-medium text-gray-400">
                      Тип
                    </th>
                    <th className="px-6 py-3 text-left text-sm font-medium text-gray-400">
                      Паттерн
                    </th>
                    <th className="px-6 py-3 text-left text-sm font-medium text-gray-400">
                      Серьезность
                    </th>
                    <th className="px-6 py-3 text-left text-sm font-medium text-gray-400">
                      Статус
                    </th>
                    <th className="px-6 py-3 text-right text-sm font-medium text-gray-400">
                      Действия
                    </th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-dark-700">
                  {rules.map((rule) => (
                    <tr key={rule.id}>
                      <td className="px-6 py-4 text-white">{rule.name}</td>
                      <td className="px-6 py-4">
                        <span className="px-2 py-1 bg-dark-900 rounded text-xs text-gray-300">
                          {rule.type}
                        </span>
                      </td>
                      <td className="px-6 py-4 font-mono text-sm text-gray-400">{rule.pattern}</td>
                      <td className="px-6 py-4">
                        <SeverityBadge severity={rule.severity} />
                      </td>
                      <td className="px-6 py-4">
                        <span
                          className={`px-2 py-1 rounded text-xs ${rule.is_active
                            ? 'bg-green-500/10 text-green-400'
                            : 'bg-gray-500/10 text-gray-400'
                            }`}
                        >
                          {rule.is_active ? 'Активно' : 'Неактивно'}
                        </span>
                      </td>
                      <td className="px-6 py-4 text-right">
                        <button
                          onClick={() => handleDeleteRule(rule.id)}
                          className="px-3 py-1 bg-red-500/10 text-red-400 hover:bg-red-500/20 rounded text-sm transition-colors"
                        >
                          Удалить
                        </button>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            )}
          </div>
        </div>
      )}

      {activeTab === '2fa' && (
        <div className="space-y-6">
          <div className="bg-dark-800 rounded-lg p-6 border border-dark-700">
            <h2 className="text-lg font-semibold text-white mb-4">
              Двухфакторная аутентификация
            </h2>

            <div className="space-y-4">
              <div className="flex items-center justify-between py-4 border-b border-dark-700">
                <div>
                  <p className="text-white font-medium">Статус 2FA</p>
                  <p className="text-gray-400 text-sm mt-1">
                    {is2FAConfigured
                      ? is2FAEnabled
                        ? 'Двухфакторная аутентификация включена'
                        : '2FA настроен, но отключен'
                      : '2FA не настроен'}
                  </p>
                </div>
                <div className="flex items-center space-x-3">
                  {is2FAEnabled ? (
                    <>
                      <span className="px-3 py-1 bg-green-500/10 text-green-400 rounded-full text-sm">
                        Включено
                      </span>
                      <button
                        onClick={handle2FADisable}
                        className="px-4 py-2 bg-red-500/10 text-red-400 hover:bg-red-500/20 rounded-lg transition-colors"
                      >
                        Отключить
                      </button>
                    </>
                  ) : is2FAConfigured ? (
                    <>
                      <span className="px-3 py-1 bg-yellow-500/10 text-yellow-400 rounded-full text-sm">
                        Отключено
                      </span>
                    </>
                  ) : (
                    <button
                      onClick={handle2FASetup}
                      className="px-4 py-2 bg-primary-600 hover:bg-primary-700 text-white rounded-lg transition-colors"
                    >
                      Настроить 2FA
                    </button>
                  )}
                </div>
              </div>

              <div className="py-4">
                <h3 className="text-white font-medium mb-2">
                  Как работает двухфакторная аутентификация?
                </h3>
                <ul className="text-gray-400 text-sm space-y-2">
                  <li>• При каждом входе потребуется код из приложения аутентификации</li>
                  <li>• Используйте Google Authenticator, Authy или аналогичное приложение</li>
                  <li>• Сохраните резервные коды для восстановления доступа</li>
                  <li>• 2FA значительно повышает безопасность вашего аккаунта</li>
                </ul>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

function SeverityBadge({ severity }: { severity: string }) {
  const colors: Record<string, string> = {
    critical: 'bg-red-500/10 text-red-400',
    high: 'bg-orange-500/10 text-orange-400',
    medium: 'bg-yellow-500/10 text-yellow-400',
    low: 'bg-green-500/10 text-green-400',
  };

  const labels: Record<string, string> = {
    critical: 'Крит.',
    high: 'Высок.',
    medium: 'Сред.',
    low: 'Низк.',
  };

  return (
    <span className={`px-2 py-1 rounded text-xs font-medium ${colors[severity] || colors.low}`}>
      {labels[severity] || labels.low}
    </span>
  );
}
