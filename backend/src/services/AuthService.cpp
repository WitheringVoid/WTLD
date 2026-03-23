#include "../../include/services/AuthService.h"
#include <bcrypt/bcrypt.h>
#include <jwt-cpp/jwt.h>
#include <openssl/rand.h>
#include <drogon/HttpAppFramework.h>

namespace wtld
{
namespace services
{

AuthService::AuthService(const drogon::orm::DbClientPtr &dbClient)
    : dbClient_(dbClient) {}

std::optional<models::User> AuthService::registerUser(
    const std::string &username,
    const std::string &email,
    const std::string &password)
{
    try
    {
        // Проверка существования пользователя
        auto result = dbClient_->execSqlSync(
            "SELECT id FROM users WHERE username = $1 OR email = $2",
            username, email);

        if (result.size() > 0)
        {
            return std::nullopt;
        }

        // Хэширование пароля
        std::string passwordHash = hashPassword(password);

        // Создание пользователя
        auto insertResult = dbClient_->execSqlSync(
            "INSERT INTO users (username, email, password_hash) VALUES ($1, $2, $3) RETURNING id, username, email, created_at, updated_at, is_active",
            username, email, passwordHash);

        if (insertResult.size() == 0)
        {
            return std::nullopt;
        }

        models::User user;
        user.id = insertResult[0]["id"].as<int>();
        user.username = insertResult[0]["username"].as<std::string>();
        user.email = insertResult[0]["email"].as<std::string>();
        user.password_hash = passwordHash;
        user.created_at = std::time(nullptr);
        user.updated_at = std::time(nullptr);
        user.last_login = 0;
        user.is_active = insertResult[0]["is_active"].as<bool>();

        return user;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Registration error: " << e.what();
        return std::nullopt;
    }
}

std::optional<models::User> AuthService::authenticate(
    const std::string &username,
    const std::string &password)
{
    try
    {
        auto result = dbClient_->execSqlSync(
            "SELECT id, username, email, password_hash, created_at, updated_at, last_login, is_active "
            "FROM users WHERE username = $1",
            username);

        if (result.size() == 0)
        {
            return std::nullopt;
        }

        models::User user;
        user.id = result[0]["id"].as<int>();
        user.username = result[0]["username"].as<std::string>();
        user.email = result[0]["email"].as<std::string>();
        user.password_hash = result[0]["password_hash"].as<std::string>();
        user.created_at = std::time(nullptr); // TODO: convert from timestamp
        user.updated_at = std::time(nullptr);
        user.last_login = 0;
        user.is_active = result[0]["is_active"].as<bool>();

        if (!user.is_active)
        {
            return std::nullopt;
        }

        // Проверка пароля
        if (!verifyPassword(password, user.password_hash))
        {
            return std::nullopt;
        }

        // Обновление last_login
        dbClient_->execSqlSync("UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = $1", user.id);

        return user;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Authentication error: " << e.what();
        return std::nullopt;
    }
}

std::optional<models::User> AuthService::getUserById(int userId)
{
    try
    {
        auto result = dbClient_->execSqlSync(
            "SELECT id, username, email, password_hash, created_at, updated_at, last_login, is_active "
            "FROM users WHERE id = $1",
            userId);

        if (result.size() == 0)
        {
            return std::nullopt;
        }

        models::User user;
        user.id = result[0]["id"].as<int>();
        user.username = result[0]["username"].as<std::string>();
        user.email = result[0]["email"].as<std::string>();
        user.password_hash = result[0]["password_hash"].as<std::string>();
        user.created_at = std::time(nullptr);
        user.updated_at = std::time(nullptr);
        user.last_login = 0;
        user.is_active = result[0]["is_active"].as<bool>();

        return user;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Get user error: " << e.what();
        return std::nullopt;
    }
}

std::string AuthService::generateToken(const models::User &user)
{
    try
    {
        auto secret = drogon::app().getSecretKey();
        if (secret.empty())
        {
            secret = "wtld-secret-key-change-in-production";
        }

        auto token = jwt::create()
                         .set_issuer("wtld-auth")
                         .set_type("JWT")
                         .set_id(std::to_string(user.id))
                         .set_payload_claim("username", jwt::claim(user.username))
                         .set_payload_claim("email", jwt::claim(user.email))
                         .set_issued_at(std::chrono::system_clock::now())
                         .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
                         .sign(jwt::algorithm::hs256{secret});

        return token;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Token generation error: " << e.what();
        return "";
    }
}

std::optional<models::User> AuthService::validateToken(const std::string &token)
{
    try
    {
        auto secret = drogon::app().getSecretKey();
        if (secret.empty())
        {
            secret = "wtld-secret-key-change-in-production";
        }

        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{secret})
                            .with_issuer("wtld-auth");

        verifier.verify(decoded);

        // Получение user_id из токена
        auto userId = std::stoi(decoded.get_id());
        return getUserById(userId);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Token validation error: " << e.what();
        return std::nullopt;
    }
}

bool AuthService::verifyTwoFactorCode(int userId, const std::string &code)
{
    try
    {
        auto result = dbClient_->execSqlSync(
            "SELECT secret_key FROM two_factor_auth WHERE user_id = $1 AND is_enabled = true",
            userId);

        if (result.size() == 0)
        {
            return false;
        }

        std::string secret = result[0]["secret_key"].as<std::string>();
        TwoFactorAuthService totp(dbClient_);
        return totp.verifyCode(secret, code);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "2FA verification error: " << e.what();
        return false;
    }
}

bool AuthService::verifyBackupCode(int userId, const std::string &code)
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

        auto backupCodes = result[0]["backup_codes"].as<std::vector<std::string>>();
        for (const auto &backupCode : backupCodes)
        {
            if (backupCode == code)
            {
                // Удаляем использованный код
                backupCodes.erase(std::find(backupCodes.begin(), backupCodes.end(), backupCode));
                dbClient_->execSqlSync(
                    "UPDATE two_factor_auth SET backup_codes = $1 WHERE user_id = $2",
                    backupCodes, userId);
                return true;
            }
        }

        return false;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Backup code verification error: " << e.what();
        return false;
    }
}

std::string AuthService::hashPassword(const std::string &password)
{
    // Генерация соли
    std::string salt = bcrypt_gensalt(12);
    // Хэширование пароля
    return bcrypt_hashpw(password.c_str(), salt.c_str());
}

bool AuthService::verifyPassword(const std::string &password, const std::string &hash)
{
    return bcrypt_verify(password.c_str(), hash.c_str()) == 0;
}

} // namespace services
} // namespace wtld
