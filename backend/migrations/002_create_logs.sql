-- Таблица загруженных логов
CREATE TABLE IF NOT EXISTS logs (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    filename VARCHAR(255) NOT NULL,
    original_content TEXT,
    parsed_content JSONB,  -- Парсенные логи в JSON
    file_type VARCHAR(10) CHECK (file_type IN ('json', 'txt')),
    file_size BIGINT,
    upload_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'processing', 'completed', 'failed'))
);

CREATE INDEX idx_logs_user_id ON logs(user_id);
CREATE INDEX idx_logs_upload_date ON logs(upload_date);