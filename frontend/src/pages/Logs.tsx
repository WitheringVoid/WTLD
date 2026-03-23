import { useState, useEffect, useRef } from 'react';
import { logsApi } from '@/api';
import type { LogFile } from '@/types/api';
import { Link } from 'react-router-dom';
import toast from 'react-hot-toast';

export default function Logs() {
  const [logs, setLogs] = useState<LogFile[]>([]);
  const [isLoading, setIsLoading] = useState(true);
  const [isUploading, setIsUploading] = useState(false);
  const fileInputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    loadLogs();
  }, []);

  const loadLogs = async () => {
    try {
      const response = await logsApi.getLogs({ limit: 50 });
      setLogs(response.data.data);
    } catch (error) {
      console.error('Failed to load logs:', error);
      toast.error('Не удалось загрузить логи');
    } finally {
      setIsLoading(false);
    }
  };

  const handleFileUpload = async (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    // Проверка типа файла
    const validTypes = ['.txt', '.json', '.log'];
    const fileExt = '.' + file.name.split('.').pop()?.toLowerCase();
    if (!validTypes.includes(fileExt)) {
      toast.error('Неподдерживаемый формат файла. Используйте .txt, .json или .log');
      return;
    }

    setIsUploading(true);
    try {
      await logsApi.upload(file);
      toast.success('Файл загружен и обрабатывается');
      loadLogs();
    } catch (error: unknown) {
      const err = error as { response?: { data?: { message?: string } } };
      toast.error(err.response?.data?.message || 'Ошибка загрузки файла');
    } finally {
      setIsUploading(false);
      if (fileInputRef.current) {
        fileInputRef.current.value = '';
      }
    }
  };

  const handleDelete = async (id: number) => {
    if (!confirm('Вы уверены, что хотите удалить этот лог?')) return;

    try {
      await logsApi.deleteLog(id);
      toast.success('Лог удален');
      loadLogs();
    } catch (error) {
      toast.error('Не удалось удалить лог');
    }
  };

  const getStatusColor = (status: string) => {
    const colors: Record<string, string> = {
      pending: 'bg-yellow-500/10 text-yellow-400 border-yellow-500/20',
      processing: 'bg-blue-500/10 text-blue-400 border-blue-500/20',
      completed: 'bg-green-500/10 text-green-400 border-green-500/20',
      failed: 'bg-red-500/10 text-red-400 border-red-500/20',
    };
    return colors[status] || colors.pending;
  };

  const formatFileSize = (bytes: number) => {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h1 className="text-2xl font-bold text-white">Логи</h1>
        <button
          onClick={() => fileInputRef.current?.click()}
          disabled={isUploading}
          className="px-4 py-2 bg-primary-600 hover:bg-primary-700 text-white font-medium rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed flex items-center space-x-2"
        >
          <span>📤</span>
          <span>{isUploading ? 'Загрузка...' : 'Загрузить лог'}</span>
        </button>
        <input
          ref={fileInputRef}
          type="file"
          onChange={handleFileUpload}
          className="hidden"
          accept=".txt,.json,.log"
        />
      </div>

      {isLoading ? (
        <div className="flex items-center justify-center h-64">
          <div className="text-gray-400">Загрузка...</div>
        </div>
      ) : logs.length === 0 ? (
        <div className="bg-dark-800 rounded-lg p-12 border border-dark-700 text-center">
          <span className="text-6xl mb-4 block">📋</span>
          <h3 className="text-xl font-semibold text-white mb-2">Нет загруженных логов</h3>
          <p className="text-gray-400 mb-6">Загрузите первый файл с логами для начала анализа</p>
          <button
            onClick={() => fileInputRef.current?.click()}
            className="px-6 py-3 bg-primary-600 hover:bg-primary-700 text-white font-medium rounded-lg transition-colors"
          >
            Загрузить лог
          </button>
        </div>
      ) : (
        <div className="bg-dark-800 rounded-lg border border-dark-700 overflow-hidden">
          <table className="w-full">
            <thead className="bg-dark-900">
              <tr>
                <th className="px-6 py-4 text-left text-sm font-medium text-gray-400">Файл</th>
                <th className="px-6 py-4 text-left text-sm font-medium text-gray-400">Тип</th>
                <th className="px-6 py-4 text-left text-sm font-medium text-gray-400">Размер</th>
                <th className="px-6 py-4 text-left text-sm font-medium text-gray-400">Статус</th>
                <th className="px-6 py-4 text-left text-sm font-medium text-gray-400">Дата</th>
                <th className="px-6 py-4 text-right text-sm font-medium text-gray-400">Действия</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-dark-700">
              {logs.map((log) => (
                <tr key={log.id} className="hover:bg-dark-750 transition-colors">
                  <td className="px-6 py-4">
                    <div className="flex items-center space-x-3">
                      <span className="text-2xl">{log.file_type === 'json' ? '📄' : '📝'}</span>
                      <span className="font-medium text-white">{log.filename}</span>
                    </div>
                  </td>
                  <td className="px-6 py-4">
                    <span className="px-2 py-1 bg-dark-900 rounded text-sm text-gray-300 uppercase">
                      {log.file_type}
                    </span>
                  </td>
                  <td className="px-6 py-4 text-gray-300">{formatFileSize(log.file_size)}</td>
                  <td className="px-6 py-4">
                    <span
                      className={`px-3 py-1 rounded-full text-xs font-medium border ${getStatusColor(
                        log.status
                      )}`}
                    >
                      {log.status === 'pending'
                        ? 'Ожидание'
                        : log.status === 'processing'
                        ? 'Обработка'
                        : log.status === 'completed'
                        ? 'Готово'
                        : 'Ошибка'}
                    </span>
                  </td>
                  <td className="px-6 py-4 text-gray-400">
                    {new Date(log.upload_date).toLocaleString('ru-RU')}
                  </td>
                  <td className="px-6 py-4 text-right">
                    <div className="flex items-center justify-end space-x-2">
                      <Link
                        to={`/logs/${log.id}`}
                        className="px-3 py-1 bg-primary-500/10 text-primary-400 hover:bg-primary-500/20 rounded text-sm transition-colors"
                      >
                        Просмотр
                      </Link>
                      <button
                        onClick={() => handleDelete(log.id)}
                        className="px-3 py-1 bg-red-500/10 text-red-400 hover:bg-red-500/20 rounded text-sm transition-colors"
                      >
                        Удалить
                      </button>
                    </div>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
