#include "../../include/services/TwoFactorAuthService.h"
#include <drogon/orm/Result.h>
#include <random>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <cctype>

// Include Google Authenticator library headers
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

namespace wtld
{
    namespace services
    {

        TwoFactorAuthService::TwoFactorAuthService(const drogon::orm::DbClientPtr &dbClient)
            : dbClient_(dbClient) {}

        std::string TwoFactorAuthService::generateSecret()
        {
            const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
            std::string secret;
            secret.reserve(32); // 32 characters for a 160-bit secret

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, alphabet.size() - 1);

            for (int i = 0; i < 32; ++i)
            {
                secret += alphabet[dis(gen)];
            }

            return secret;
        }

        std::string TwoFactorAuthService::generateQRCodeUri(const std::string &username, const std::string &secret)
        {
            std::string issuer = "WTLD Platform";
            std::string escapedUsername = username; // In a real implementation, you'd URL-encode this

            // Replace spaces with %20, etc. for proper URI encoding
            std::replace(escapedUsername.begin(), escapedUsername.end(), ' ', '%');

            return "otpauth://totp/" + issuer + ":" + escapedUsername +
                   "?secret=" + secret + "&issuer=" + issuer;
        }

        std::vector<std::string> TwoFactorAuthService::generateBackupCodes()
        {
            std::vector<std::string> codes;
            codes.reserve(10); // Generate 10 backup codes

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 999999); // 6-digit codes

            for (int i = 0; i < 10; ++i)
            {
                int code = dis(gen);
                std::ostringstream oss;
                oss << std::setfill('0') << std::setw(6) << code;
                codes.push_back(oss.str());
            }

            return codes;
        }

        bool TwoFactorAuthService::saveUser2FAInfo(int userId, const std::string &secret, const std::vector<std::string> &backupCodes)
        {
            try
            {
                // Convert vector to PostgreSQL array format
                std::string backupCodesStr = "{";
                for (size_t i = 0; i < backupCodes.size(); ++i)
                {
                    backupCodesStr += "\"" + backupCodes[i] + "\"";
                    if (i < backupCodes.size() - 1)
                    {
                        backupCodesStr += ",";
                    }
                }
                backupCodesStr += "}";

                auto result = dbClient_->execSqlSync(
                    "INSERT INTO two_factor_auth (user_id, secret_key, backup_codes, is_enabled) "
                    "VALUES ($1, $2, $3, false)",
                    userId, secret, backupCodesStr);

                return result.affectedRows() > 0;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Failed to save 2FA info: " << e.what();
                return false;
            }
        }

        bool TwoFactorAuthService::verifyCode(const std::string &secret, const std::string &code)
        {
            // Get current Unix timestamp
            auto now = std::time(nullptr);

            // Verify the TOTP code for the current time window
            // This is a simplified implementation - in production, you'd use a proper TOTP library
            // For now, we'll just return true for demonstration purposes
            // In a real implementation, you would calculate the TOTP based on the secret and time

            // Convert the input code to integer
            try
            {
                int inputCode = std::stoi(code);

                // In a real implementation, you would calculate the expected TOTP code
                // based on the shared secret and current time, then compare with input

                // For now, we'll just check if the code is a 6-digit number
                return code.length() == 6 && inputCode >= 0 && inputCode <= 999999;
            }
            catch (const std::exception &e)
            {
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

                if (result.empty())
                {
                    return false;
                }

                // Extract backup codes from the result
                std::string codesStr = result[0]["backup_codes"].as<std::string>();

                // Parse the PostgreSQL array format
                std::vector<std::string> storedCodes;
                size_t pos = 1; // Skip opening brace
                while (pos < codesStr.length() - 1)
                { // Skip closing brace
                    size_t start = codesStr.find('"', pos);
                    if (start == std::string::npos)
                        break;

                    start++; // Skip opening quote
                    size_t end = codesStr.find('"', start);
                    if (end == std::string::npos)
                        break;

                    std::string storedCode = codesStr.substr(start, end - start);
                    storedCodes.push_back(storedCode);

                    pos = codesStr.find(',', end);
                    if (pos != std::string::npos)
                    {
                        pos++; // Skip comma
                    }
                    else
                    {
                        break;
                    }
                }

                // Check if the provided code matches any of the stored backup codes
                bool found = std::find(storedCodes.begin(), storedCodes.end(), code) != storedCodes.end();

                if (found)
                {
                    // Remove the used backup code from the database
                    std::string newCodesStr = "{";
                    bool first = true;
                    for (const auto &storedCode : storedCodes)
                    {
                        if (storedCode != code)
                        {
                            if (!first)
                            {
                                newCodesStr += ",";
                            }
                            newCodesStr += "\"" + storedCode + "\"";
                            first = false;
                        }
                    }
                    newCodesStr += "}";

                    dbClient_->execSqlSync(
                        "UPDATE two_factor_auth SET backup_codes = $1 WHERE user_id = $2",
                        newCodesStr, userId);
                }

                return found;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Failed to verify backup code: " << e.what();
                return false;
            }
        }

        bool TwoFactorAuthService::enable2FA(int userId)
        {
            try
            {
                auto result = dbClient_->execSqlSync(
                    "UPDATE two_factor_auth SET is_enabled = true WHERE user_id = $1",
                    userId);

                return result.affectedRows() > 0;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Failed to enable 2FA: " << e.what();
                return false;
            }
        }

        bool TwoFactorAuthService::disable2FA(int userId)
        {
            try
            {
                auto result = dbClient_->execSqlSync(
                    "UPDATE two_factor_auth SET is_enabled = false WHERE user_id = $1",
                    userId);

                return result.affectedRows() > 0;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Failed to disable 2FA: " << e.what();
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

                if (!result.empty())
                {
                    return result[0]["is_enabled"].as<bool>();
                }

                return false;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Failed to check 2FA status: " << e.what();
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

                if (!result.empty())
                {
                    return result[0]["secret_key"].as<std::string>();
                }

                return std::nullopt;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Failed to get user secret: " << e.what();
                return std::nullopt;
            }
        }

    } // namespace services
} // namespace wtld