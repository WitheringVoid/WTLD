// API типы

export interface User {
  id: number;
  username: string;
  email: string;
  created_at: number;
  last_login: number;
  is_active: boolean;
}

export interface AuthResponse {
  status: string;
  token?: string;
  user?: User;
  message?: string;
}

export interface LogEntry {
  timestamp: string;
  level: string;
  message: string;
  source: string;
  metadata: Record<string, unknown>;
}

export interface LogFile {
  id: number;
  filename: string;
  file_type: 'json' | 'txt';
  file_size: number;
  upload_date: string;
  status: 'pending' | 'processing' | 'completed' | 'failed';
  entries?: LogEntry[];
}

export interface AnalysisResult {
  id: number;
  type: string;
  severity: 'low' | 'medium' | 'high' | 'critical';
  title: string;
  description: string;
  data: Record<string, unknown>;
  detected_patterns?: string[];
  is_anomaly: boolean;
  created_at: string;
}

export interface DashboardData {
  statistics: {
    total_logs: number;
    completed_logs: number;
    total_anomalies: number;
    active_rules: number;
  };
  recent_anomalies: AnalysisResult[];
  severity_distribution: {
    critical: number;
    high: number;
    medium: number;
    low: number;
  };
}

export interface AnalysisRule {
  id: number;
  name: string;
  type: 'regex' | 'keyword' | 'threshold' | 'custom';
  pattern: string;
  severity: 'low' | 'medium' | 'high' | 'critical';
  is_active: boolean;
}

export interface TwoFASetupResponse {
  status: string;
  data: {
    secret: string;
    qr_uri: string;
    backup_codes: string[];
  };
}

export interface TwoFAStatusResponse {
  status: string;
  data: {
    is_enabled: boolean;
    is_configured: boolean;
  };
}

export interface WebSocketEvent {
  type: 'ANOMALY_DETECTED' | 'LOG_UPLOADED' | 'ANALYSIS_COMPLETE' | 'REALTIME_LOG' | 'SYSTEM_NOTIFICATION';
  title: string;
  message: string;
  data: Record<string, unknown>;
  timestamp: string;
}
