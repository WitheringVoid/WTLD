-- Таблица аналитики
CREATE TABLE IF NOT EXISTS analytics (
    id SERIAL PRIMARY KEY,
    log_id INTEGER REFERENCES logs(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    analysis_type VARCHAR(50),  -- error_rate, response_time, request_count, etc.
    result JSONB NOT NULL,      -- Результаты анализа в JSON
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    metadata JSONB              -- Дополнительная информация об анализе
);

CREATE INDEX idx_analytics_log_id ON analytics(log_id);
CREATE INDEX idx_analytics_user_id ON analytics(user_id);