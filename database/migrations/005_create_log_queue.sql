-- Таблица для очереди обработки логов (асинхронная обработка)
CREATE TABLE IF NOT EXISTS log_processing_queue (
    id SERIAL PRIMARY KEY,
    log_file_id INTEGER REFERENCES logs(id) ON DELETE CASCADE,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'processing', 'completed', 'failed')),
    priority INTEGER DEFAULT 0,  -- 0 = low, 1 = normal, 2 = high
    attempts INTEGER DEFAULT 0,
    max_attempts INTEGER DEFAULT 3,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    started_at TIMESTAMP,
    completed_at TIMESTAMP,
    error_message TEXT
);

-- Индексы для эффективной работы очереди
CREATE INDEX idx_log_queue_status_priority ON log_processing_queue(status, priority DESC);
CREATE INDEX idx_log_queue_user_id ON log_processing_queue(user_id);
CREATE INDEX idx_log_queue_created_at ON log_processing_queue(created_at);