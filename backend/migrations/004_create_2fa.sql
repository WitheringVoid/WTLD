-- Таблица для хранения информации о 2FA
CREATE TABLE IF NOT EXISTS two_factor_auth (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE CASCADE,
    secret_key VARCHAR(32) NOT NULL,  -- Base32 encoded secret
    backup_codes TEXT[],              -- Массив резервных кодов
    is_enabled BOOLEAN DEFAULT FALSE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Индекс для быстрого поиска по user_id
CREATE INDEX idx_2fa_user_id ON two_factor_auth(user_id);
