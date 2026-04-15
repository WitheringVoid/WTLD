-- Добавление UNIQUE констрейнта на user_id для поддержки ON CONFLICT
-- Это необходимо для корректной работы upsert в TwoFactorAuthService

-- Сначала удаляем дубликаты (если есть), оставляя последнюю запись
DELETE FROM two_factor_auth a
USING two_factor_auth b
WHERE a.user_id = b.user_id
  AND a.id < b.id;

-- Добавляем UNIQUE констрейнт
ALTER TABLE two_factor_auth ADD CONSTRAINT uq_2fa_user_id UNIQUE (user_id);
