import { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { twoFAApi } from '@/api';
import toast from 'react-hot-toast';
import { QRCodeSVG } from 'qrcode.react';

export default function TwoFASetup() {
  const navigate = useNavigate();
  const [step, setStep] = useState<'setup' | 'verify' | 'backup'>('setup');
  const [secret, setSecret] = useState('');
  const [qrUri, setQrUri] = useState('');
  const [backupCodes, setBackupCodes] = useState<string[]>([]);
  const [verificationCode, setVerificationCode] = useState('');
  const [isLoading, setIsLoading] = useState(false);

  useEffect(() => {
    load2FAStatus();
  }, []);

  const load2FAStatus = async () => {
    try {
      const response = await twoFAApi.getStatus();
      if (response.data.data.is_enabled) {
        toast.error('2FA уже включен');
        navigate('/settings');
      }
    } catch (error) {
      console.error('Failed to load 2FA status:', error);
    }
  };

  const handleSetup = async () => {
    setIsLoading(true);
    try {
      const response = await twoFAApi.setup();
      setSecret(response.data.data.secret);
      setQrUri(response.data.data.qr_uri);
      setBackupCodes(response.data.data.backup_codes);
      setStep('verify');
      toast.success('2FA настроен. Отсканируйте QR-код и введите код для подтверждения.');
    } catch (error) {
      toast.error('Не удалось настроить 2FA');
    } finally {
      setIsLoading(false);
    }
  };

  const handleVerify = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsLoading(true);

    try {
      const response = await twoFAApi.verify(verificationCode);
      if (response.data.valid) {
        await twoFAApi.enable();
        setStep('backup');
        toast.success('2FA успешно включен!');
      } else {
        toast.error('Неверный код');
      }
    } catch (error) {
      toast.error('Ошибка проверки кода');
    } finally {
      setIsLoading(false);
    }
  };

  const handleFinish = () => {
    toast.success('Сохраните резервные коды в безопасном месте!');
    navigate('/settings');
  };

  const handleDownloadBackupCodes = () => {
    const content = `WTLD Backup Codes\n=================\n\nСохраните эти коды в безопасном месте. Каждый код можно использовать только один раз.\n\n${backupCodes.join('\n')}`;
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'wtld-backup-codes.txt';
    a.click();
    URL.revokeObjectURL(url);
  };

  const handleCopyCodes = () => {
    navigator.clipboard.writeText(backupCodes.join('\n'));
    toast.success('Коды скопированы в буфер обмена');
  };

  return (
    <div className="min-h-screen bg-dark-900 flex items-center justify-center p-4">
      <div className="w-full max-w-2xl">
        <div className="bg-dark-800 rounded-lg shadow-xl p-8 border border-dark-700">
          <div className="text-center mb-8">
            <h1 className="text-3xl font-bold text-white">Настройка 2FA</h1>
            <p className="text-gray-400 mt-2">
              {step === 'setup' && 'Отсканируйте QR-код приложением аутентификации'}
              {step === 'verify' && 'Введите код из приложения для подтверждения'}
              {step === 'backup' && 'Сохраните резервные коды'}
            </p>
          </div>

          {step === 'setup' && (
            <div className="space-y-6">
              <div className="flex justify-center">
                <div className="bg-white p-6 rounded-lg">
                  {qrUri ? (
                    <QRCodeSVG value={qrUri} size={200} />
                  ) : (
                    <div className="w-[200px] h-[200px] bg-gray-200 flex items-center justify-center text-gray-400">
                      QR-код будет здесь
                    </div>
                  )}
                </div>
              </div>

              <div className="text-center">
                <p className="text-gray-300 mb-4">
                  Секретный ключ (для ручного ввода):
                </p>
                <code className="px-4 py-2 bg-dark-900 rounded-lg text-primary-400 font-mono text-lg">
                  {secret || 'Загрузка...'}
                </code>
              </div>

              <div className="space-y-4">
                <ol className="text-gray-300 space-y-2 text-sm">
                  <li>1. Откройте приложение аутентификации (Google Authenticator, Authy)</li>
                  <li>2. Отсканируйте QR-код или введите секретный ключ вручную</li>
                  <li>3. Нажмите "Продолжить" для подтверждения</li>
                </ol>

                <button
                  onClick={handleSetup}
                  disabled={isLoading || !!secret}
                  className="w-full py-3 px-4 bg-primary-600 hover:bg-primary-700 text-white font-medium rounded-lg transition-colors disabled:opacity-50"
                >
                  {secret ? 'QR-код сгенерирован' : isLoading ? 'Генерация...' : 'Сгенерировать QR-код'}
                </button>

                {secret && (
                  <button
                    onClick={() => setStep('verify')}
                    className="w-full py-3 px-4 bg-dark-700 hover:bg-dark-600 text-white font-medium rounded-lg transition-colors"
                  >
                    Продолжить
                  </button>
                )}
              </div>
            </div>
          )}

          {step === 'verify' && (
            <form onSubmit={handleVerify} className="space-y-6">
              <div className="text-center">
                <p className="text-gray-300 mb-4">
                  Введите 6-значный код из вашего приложения
                </p>
              </div>

              <div>
                <input
                  type="text"
                  value={verificationCode}
                  onChange={(e) => setVerificationCode(e.target.value.replace(/\D/g, '').slice(0, 6))}
                  className="w-full px-4 py-4 bg-dark-900 border border-dark-700 rounded-lg text-white text-center text-2xl tracking-widest focus:outline-none focus:ring-2 focus:ring-primary-500"
                  placeholder="000000"
                  maxLength={6}
                  required
                />
              </div>

              <div className="space-y-3">
                <button
                  type="submit"
                  disabled={isLoading || verificationCode.length !== 6}
                  className="w-full py-3 px-4 bg-primary-600 hover:bg-primary-700 text-white font-medium rounded-lg transition-colors disabled:opacity-50"
                >
                  {isLoading ? 'Проверка...' : 'Подтвердить'}
                </button>

                <button
                  type="button"
                  onClick={() => setStep('setup')}
                  className="w-full py-3 px-4 bg-dark-700 hover:bg-dark-600 text-white font-medium rounded-lg transition-colors"
                >
                  Назад
                </button>
              </div>
            </form>
          )}

          {step === 'backup' && (
            <div className="space-y-6">
              <div className="bg-green-500/10 border border-green-500/20 rounded-lg p-4">
                <p className="text-green-400 text-center font-medium">
                  ✅ 2FA успешно настроен!
                </p>
              </div>

              <div>
                <p className="text-gray-300 mb-4">
                  Сохраните эти резервные коды в безопасном месте. Каждый код можно использовать
                  только один раз для восстановления доступа.
                </p>

                <div className="grid grid-cols-2 gap-3 mb-4">
                  {backupCodes.map((code, index) => (
                    <div
                      key={index}
                      className="px-4 py-3 bg-dark-900 rounded-lg text-center font-mono text-white border border-dark-700"
                    >
                      {code}
                    </div>
                  ))}
                </div>

                <div className="flex space-x-3">
                  <button
                    onClick={handleDownloadBackupCodes}
                    className="flex-1 py-3 px-4 bg-dark-700 hover:bg-dark-600 text-white font-medium rounded-lg transition-colors"
                  >
                    Скачать
                  </button>
                  <button
                    onClick={handleCopyCodes}
                    className="flex-1 py-3 px-4 bg-dark-700 hover:bg-dark-600 text-white font-medium rounded-lg transition-colors"
                  >
                    Копировать
                  </button>
                </div>
              </div>

              <button
                onClick={handleFinish}
                className="w-full py-3 px-4 bg-primary-600 hover:bg-primary-700 text-white font-medium rounded-lg transition-colors"
              >
                Готово
              </button>
            </div>
          )}

          <button
            onClick={() => navigate('/settings')}
            className="w-full mt-6 py-2 text-gray-400 hover:text-white transition-colors"
          >
            ← Вернуться к настройкам
          </button>
        </div>
      </div>
    </div>
  );
}
