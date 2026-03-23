#pragma once
#include <string>
#include <memory>
#include <optional>
#include <drogon/orm/DbClient.h>
#include "../models/User.h"

namespace wtld
{
    namespace services
    {

        class TwoFactorAuthService
        {
        public:
            explicit TwoFactorAuthService(const drogon::orm::DbClientPtr &dbClient);

            // Генерация секретного ключа для 2FA
            std::string generateSecret();

            // Генерация QR-кода для привязки приложения
            std::string generateQRCodeUri(const std::string &username, const std::string &secret);

            // Генерация резервных кодов
            std::vector<std::string> generateBackupCodes();

            // Сохранение информации о 2FA для пользователя
            bool saveUser2FAInfo(int userId, const std::string &secret, const std::vector<std::string> &backupCodes);

            // Проверка кода 2FA
            bool verifyCode(const std::string &secret, const std::string &code);

            // Проверка резервного кода
            bool verifyBackupCode(int userId, const std::string &code);

            // Активация 2FA для пользователя
            bool enable2FA(int userId);

            // Деактивация 2FA для пользователя
            bool disable2FA(int userId);

            // Получение состояния 2FA для пользователя
            bool is2FAEnabled(int userId);

        private:
            drogon::orm::DbClientPtr dbClient_;

            // Внутренняя функция для получения секрета пользователя
            std::optional<std::string> getUserSecret(int userId);
        };

    } // namespace services
} // namespace wtld