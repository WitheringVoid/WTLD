-- WTLD migration 008: TOTP-секреты длиннее 32 символов не помещались в secret_key VARCHAR(32).
-- Применять от владельца схемы (postgres):
ALTER TABLE two_factor_auth ALTER COLUMN secret_key TYPE VARCHAR(256);