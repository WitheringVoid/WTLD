import axios from 'axios';
import type {
  AuthResponse,
  LogFile,
  DashboardData,
  AnalysisResult,
  AnalysisRule,
  TwoFASetupResponse,
  TwoFAStatusResponse,
  User
} from '@/types/api';

const api = axios.create({
  baseURL: '/api',
  headers: {
    'Content-Type': 'application/json',
  },
});

// Добавление токена к запросам
api.interceptors.request.use((config) => {
  const token = localStorage.getItem('token');
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

// Обработка ошибок
api.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.response?.status === 401) {
      localStorage.removeItem('token');
      window.location.href = '/login';
    }
    return Promise.reject(error);
  }
);

export const authApi = {
  register: (username: string, email: string, password: string) =>
    api.post<AuthResponse>('/auth/register', { username, email, password }),

  login: (username: string, password: string) =>
    api.post<AuthResponse>('/auth/login', { username, password }),

  logout: () => api.post('/auth/logout'),

  getProfile: () => api.get<User>('/auth/profile'),
};

export const logsApi = {
  upload: (file: File) => {
    const formData = new FormData();
    formData.append('file', file);
    return api.post<{ status: string; data: LogFile }>('/logs/upload', formData, {
      headers: { 'Content-Type': 'multipart/form-data' },
    });
  },

  getLogs: (params?: { status?: string; fileType?: string; limit?: number }) =>
    api.get<{ status: string; data: LogFile[] }>('/logs', { params }),

  getLogById: (id: number) =>
    api.get<{ status: string; data: LogFile }>(`/logs/${id}`),

  deleteLog: (id: number) =>
    api.delete(`/logs/${id}`),

  getLogStats: (id: number) =>
    api.get<{ status: string; data: { statistics: unknown; analytics: AnalysisResult[] } }>(`/logs/${id}/stats`),
};

export const analyticsApi = {
  getDashboard: () =>
    api.get<DashboardData>('/analytics/dashboard'),

  getAnalytics: (logId: number) =>
    api.get<{ status: string; data: AnalysisResult[] }>(`/analytics/${logId}`),

  getAnomalies: (logId: number) =>
    api.get<{ status: string; data: AnalysisResult[] }>(`/analytics/${logId}/anomalies`),

  getRules: () =>
    api.get<{ status: string; data: AnalysisRule[] }>('/analytics/rules'),

  createRule: (rule: Omit<AnalysisRule, 'id'>) =>
    api.post<{ status: string; data: { id: number } }>('/analytics/rules', rule),

  deleteRule: (id: number) =>
    api.delete(`/analytics/rules/${id}`),
};

export const twoFAApi = {
  setup: () =>
    api.post<TwoFASetupResponse>('/2fa/setup'),

  verify: (code: string) =>
    api.post<{ status: string; valid: boolean }>('/2fa/verify', { code }),

  enable: () =>
    api.post('/2fa/enable'),

  disable: () =>
    api.post('/2fa/disable'),

  getStatus: () =>
    api.get<TwoFAStatusResponse>('/2fa/status'),

  getBackupCodes: () =>
    api.get<{ status: string; data: { backup_codes: string[]; remaining: number } }>('/2fa/backup-codes'),
};

export default api;
