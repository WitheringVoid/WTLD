-- Таблица для хранения результатов анализа логов
CREATE TABLE IF NOT EXISTS log_analytics (
    id SERIAL PRIMARY KEY,
    log_file_id INTEGER REFERENCES logs(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    analysis_type VARCHAR(50) NOT NULL,  -- error_rate, response_time, request_count, anomaly_detection
    severity_level VARCHAR(10) DEFAULT 'info' CHECK (severity_level IN ('low', 'medium', 'high', 'critical')),
    title VARCHAR(255) NOT NULL,
    description TEXT,
    data JSONB,  -- Данные анализа в формате JSON
    detected_patterns TEXT[],  -- Обнаруженные паттерны
    is_anomaly BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Таблица для хранения настроек анализа (паттерны, правила)
CREATE TABLE IF NOT EXISTS analysis_rules (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    rule_name VARCHAR(100) NOT NULL,
    rule_type VARCHAR(50) NOT NULL,  -- regex, keyword, threshold, custom
    pattern TEXT NOT NULL,  -- регулярное выражение или ключевое слово
    severity VARCHAR(10) DEFAULT 'medium' CHECK (severity IN ('low', 'medium', 'high', 'critical')),
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Индексы для эффективного поиска
CREATE INDEX idx_log_analytics_file_id ON log_analytics(log_file_id);
CREATE INDEX idx_log_analytics_user_id ON log_analytics(user_id);
CREATE INDEX idx_log_analytics_type ON log_analytics(analysis_type);
CREATE INDEX idx_log_analytics_anomaly ON log_analytics(is_anomaly);
CREATE INDEX idx_analysis_rules_user_id ON analysis_rules(user_id);
CREATE INDEX idx_analysis_rules_active ON analysis_rules(is_active);