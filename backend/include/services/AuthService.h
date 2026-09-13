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
        class AuthService
        {
        public:
            explicit AuthService(const drogon::orm::DbClientPtr &dbClient);

            // --- Новые методы для 2FA при входе ---
            bool isTwoFactorEnabled(int userId);
            std::string generateTwoFactorToken(const models::User &user);
            std::optional<int> validateTwoFactorToken(const std::string &token);

            std::optional<models::User> getUserById(int userId);

            // Регистрация нового пользователя
            std::optional<models::User> registerUser(
                const std::string &username,
                const std::string &email,
                const std::string &password);

            // Аутентификация пользователя
            std::optional<models::User> authenticate(
                const std::string &username,
                const std::string &password);

            // Генерация JWT токена
            std::string generateToken(const models::User &user);

            // Валидация JWT токена
            std::optional<models::User> validateToken(const std::string &token);

            // Проверка 2FA кода
            bool verifyTwoFactorCode(int userId, const std::string &code);

            // Проверка резервного кода 2FA
            bool verifyBackupCode(int userId, const std::string &code);

            // Получить JWT конфигурацию
            static std::string getJwtSecret();
            static std::string getJwtIssuer();
            static int getJwtExpirationHours();

        private:
            drogon::orm::DbClientPtr dbClient_;
            std::string hashPassword(const std::string &password);
            bool verifyPassword(const std::string &password, const std::string &hash);
        };
    } // namespace services
} // namespace wtld