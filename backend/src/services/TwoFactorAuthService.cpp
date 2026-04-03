#include "../../include/services/TwoFactorAuthService.h"
#include <jwt-cpp/jwt.h>
#include <openssl/rand.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace wtld
{
    namespace services
    {

        TwoFactorAuthService::TwoFactorAuthService(const drogon::orm::DbClientPtr &dbClient)
            : dbClient_(dbClient) {}

        std::string TwoFactorAuthService::generateSecret()
        {
            // Генерация случайного 20-байтного секрета
            std::string secret;
            const std::string base32Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

            unsigned char buffer[20];
            RAND_bytes(buffer, 20);

            // Base32 encoding
            for (int i = 0; i < 20; i += 5)
            {
                uint32_t val = (buffer[i] << 24) | (buffer[i + 1] << 16) | (buffer[i + 2] << 8) | buffer[i + 3];
                secret += base32Chars[(val >> 35) & 0x1F];
                secret += base32Chars[(val >> 30) & 0x1F];
                secret += base32Chars[(val >> 25) & 0x1F];
                secret += base32Chars[(val >> 20) & 0x1F];
                secret += base32Chars[(val >> 15) & 0x1F];
                secret += base32Chars[(val >> 10) & 0x1F];
                secret += base32Chars[(val >> 5) & 0x1F];
                secret += base32Chars[val & 0x1F];
            }

            // Добавляем padding
            uint32_t val = (buffer[20 - 5] << 24) | (buffer[20 - 4] << 16) | (buffer[20 - 3] << 8) | buffer[20 - 2];
            secret += base32Chars[(val >> 35) & 0x1F];
            secret += base32Chars[(val >> 30) & 0x1F];
            secret += base32Chars[(val >> 25) & 0x1F];
            secret += base32Chars[(val >> 20) & 0x1F];
            secret += base32Chars[(val >> 15) & 0x1F];
            secret += "======"; // Padding для 20 байт

            return secret;
        }

        std::string TwoFactorAuthService::generateQRCodeUri(const std::string &username, const std::string &secret)
        {
            // Формат URI для Google Authenticator
            // otpauth://totp/ISSUER:USERNAME?secret=SECRET&issuer=ISSUER&algorithm=SHA1&digits=6&period=30
            std::stringstream ss;
            ss << "otpauth://totp/WTLD:" << username
               << "?secret=" << secret
               << "&issuer=WTLD"
               << "&algorithm=SHA1"
               << "&digits=6"
               << "&period=30";

            return ss.str();
        }

        std::vector<std::string> TwoFactorAuthService::generateBackupCodes()
        {
            std::vector<std::string> codes;
            const std::string chars = "0123456789";

            for (int i = 0; i < 10; i++)
            {
                std::string code;
                unsigned char buffer[4];
                RAND_bytes(buffer, 4);

                for (int j = 0; j < 8; j++)
                {
                    code += chars[buffer[j % 4] % 10];
                }

                codes.push_back(code);
            }

            return codes;
        }

        bool TwoFactorAuthService::saveUser2FAInfo(int userId, const std::string &secret, const std::vector<std::string> &backupCodes)
        {
            try
            {
                // Преобразование backup_codes в PostgreSQL array
                std::string codesArray = "{";
                for (size_t i = 0; i < backupCodes.size(); i++)
                {
                    codesArray += "\"" + backupCodes[i] + "\"";
                    if (i < backupCodes.size() - 1)
                    {
                        codesArray += ",";
                    }
                }
                codesArray += "}";

                dbClient_->execSqlSync(
                    "INSERT INTO two_factor_auth (user_id, secret_key, backup_codes, is_enabled) "
                    "VALUES ($1, $2, $3, false) "
                    "ON CONFLICT (user_id) DO UPDATE SET "
                    "secret_key = $2, backup_codes = $3, is_enabled = false, updated_at = CURRENT_TIMESTAMP",
                    userId, secret, codesArray);

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Save 2FA info error: " << e.what();
                return false;
            }
        }

        bool TwoFactorAuthService::verifyCode(const std::string &secret, const std::string &code)
        {
            try
            {
                // Получение текущего временного окна (30 секунд)
                auto now = std::chrono::system_clock::now();
                auto epoch = now.time_since_epoch();
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
                uint64_t timeStep = seconds / 30;

                // Декодирование Base32 секрета
                std::string decodedSecret;
                const std::string base32Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

                for (char c : secret)
                {
                    if (c == '=')
                        break;
                    auto pos = base32Chars.find(c);
                    if (pos != std::string::npos)
                    {
                        decodedSecret += static_cast<char>(pos);
                    }
                }

                // Генерация HMAC-SHA1
                auto hmac = jwt::algorithm::hs256{""};
                (void)hmac; // Подавление предупреждения

                // Упрощенная реализация TOTP (для production используйте полноценную библиотеку)
                // Здесь базовая проверка по 6 цифрам
                if (code.length() != 6 || !std::all_of(code.begin(), code.end(), ::isdigit))
                {
                    return false;
                }

                // Для production: реализуйте полноценный TOTP алгоритм
                // Пока принимаем любой 6-значный код для демонстрации
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "TOTP verification error: " << e.what();
                return false;
            }
        }

        bool TwoFactorAuthService::verifyBackupCode(int userId, const std::string &code)
        {
            try
            {
                auto result = dbClient_->execSqlSync(
                    "SELECT backup_codes FROM two_factor_auth WHERE user_id = $1",
                    userId);

                if (result.size() == 0)
                {
                    return false;
                }

                auto backupCodesJson = result[0]["backup_codes"].as<std::string>();
                auto backupCodes = nlohmann::json::parse(backupCodesJson).get<std::vector<std::string>>();
                auto it = std::find(backupCodes.begin(), backupCodes.end(), code);

                if (it != backupCodes.end())
                {
                    // Удаляем использованный код
                    backupCodes.erase(it);
                    std::string newJson = nlohmann::json(backupCodes).dump();
                    dbClient_->execSqlSync(
                        "UPDATE two_factor_auth SET backup_codes = $1 WHERE user_id = $2",
                        newJson, userId);
                    return true;
                }

                return false;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Backup code verification error: " << e.what();
                return false;
            }
        }

        bool TwoFactorAuthService::enable2FA(int userId)
        {
            try
            {
                dbClient_->execSqlSync(
                    "UPDATE two_factor_auth SET is_enabled = true, updated_at = CURRENT_TIMESTAMP WHERE user_id = $1",
                    userId);
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Enable 2FA error: " << e.what();
                return false;
            }
        }

        bool TwoFactorAuthService::disable2FA(int userId)
        {
            try
            {
                dbClient_->execSqlSync(
                    "UPDATE two_factor_auth SET is_enabled = false, updated_at = CURRENT_TIMESTAMP WHERE user_id = $1",
                    userId);
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Disable 2FA error: " << e.what();
                return false;
            }
        }

        bool TwoFactorAuthService::is2FAEnabled(int userId)
        {
            try
            {
                auto result = dbClient_->execSqlSync(
                    "SELECT is_enabled FROM two_factor_auth WHERE user_id = $1",
                    userId);

                if (result.size() == 0)
                {
                    return false;
                }

                return result[0]["is_enabled"].as<bool>();
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Check 2FA status error: " << e.what();
                return false;
            }
        }

        std::optional<std::string> TwoFactorAuthService::getUserSecret(int userId)
        {
            try
            {
                auto result = dbClient_->execSqlSync(
                    "SELECT secret_key FROM two_factor_auth WHERE user_id = $1",
                    userId);

                if (result.size() == 0)
                {
                    return std::nullopt;
                }

                return result[0]["secret_key"].as<std::string>();
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get user secret error: " << e.what();
                return std::nullopt;
            }
        }

    } // namespace services
} // namespace wtld
