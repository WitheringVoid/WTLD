#include "../../include/services/AuthService.h"
#include "../../include/services/TwoFactorAuthService.h"
#include <sodium.h>
#include <jwt-cpp/jwt.h>
#include <openssl/rand.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>
#include <drogon/HttpAppFramework.h>
#include <drogon/config.h>

namespace wtld
{
    namespace services
    {

        // Глобальная инициализация libsodium
        static struct SodiumInit
        {
            SodiumInit() { sodium_init(); }
        } sodiumInitGuard;

        // Чтение JWT конфигурации из config.json
        static std::string getConfigValue(const std::string &key, const std::string &defaultVal)
        {
            try
            {
                auto &cfg = drogon::app().getConfig();
                if (cfg.isMember("jwt") && cfg["jwt"].isMember(key))
                {
                    return cfg["jwt"][key].asString();
                }
            }
            catch (...)
            {
            }
            return defaultVal;
        }

        static int getConfigInt(const std::string &key, int defaultVal)
        {
            try
            {
                auto &cfg = drogon::app().getConfig();
                if (cfg.isMember("jwt") && cfg["jwt"].isMember(key))
                {
                    return cfg["jwt"][key].asInt();
                }
            }
            catch (...)
            {
            }
            return defaultVal;
        }

        std::string AuthService::getJwtSecret()
        {
            return getConfigValue("secret", "wtld-secret-key-change-in-production");
        }

        std::string AuthService::getJwtIssuer()
        {
            return getConfigValue("issuer", "wtld-auth");
        }

        int AuthService::getJwtExpirationHours()
        {
            return getConfigInt("expiration_hours", 24);
        }

        // Парсинг PostgreSQL timestamp формата: "2024-01-15 10:30:45.123456" -> time_t
        static std::time_t parsePgTimestamp(const std::string &ts)
        {
            if (ts.empty())
                return 0;

            std::tm tm = {};
            std::istringstream ss(ts);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

            if (ss.fail())
                return 0;

#ifdef _WIN32
            return _mkgmtime(&tm);
#else
            return timegm(&tm);
#endif
        }

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

                // Чтение timestamps из БД
                auto createdAtStr = insertResult[0]["created_at"].as<std::string>();
                auto updatedAtStr = insertResult[0]["updated_at"].as<std::string>();
                user.created_at = parsePgTimestamp(createdAtStr);
                user.updated_at = parsePgTimestamp(updatedAtStr);
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

                // Чтение timestamps из БД
                auto createdAtStr = result[0]["created_at"].as<std::string>();
                auto updatedAtStr = result[0]["updated_at"].as<std::string>();
                auto lastLoginStr = result[0]["last_login"].as<std::string>();
                user.created_at = parsePgTimestamp(createdAtStr);
                user.updated_at = parsePgTimestamp(updatedAtStr);
                user.last_login = lastLoginStr.empty() ? 0 : parsePgTimestamp(lastLoginStr);
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

                // Чтение timestamps из БД
                auto createdAtStr = result[0]["created_at"].as<std::string>();
                auto updatedAtStr = result[0]["updated_at"].as<std::string>();
                auto lastLoginStr = result[0]["last_login"].as<std::string>();
                user.created_at = parsePgTimestamp(createdAtStr);
                user.updated_at = parsePgTimestamp(updatedAtStr);
                user.last_login = lastLoginStr.empty() ? 0 : parsePgTimestamp(lastLoginStr);
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
                const std::string secret = getJwtSecret();
                const std::string issuer = getJwtIssuer();
                const int expirationHours = getJwtExpirationHours();

                auto token = jwt::create()
                                 .set_issuer(issuer)
                                 .set_type("JWT")
                                 .set_id(std::to_string(user.id))
                                 .set_payload_claim("username", jwt::claim(user.username))
                                 .set_payload_claim("email", jwt::claim(user.email))
                                 .set_issued_at(std::chrono::system_clock::now())
                                 .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{expirationHours})
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
                const std::string secret = getJwtSecret();
                const std::string issuer = getJwtIssuer();

                auto decoded = jwt::decode(token);
                auto verifier = jwt::verify()
                                    .allow_algorithm(jwt::algorithm::hs256{secret})
                                    .with_issuer(issuer);

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

                std::string backupCodesJson = result[0]["backup_codes"].as<std::string>();
                auto backupCodes = nlohmann::json::parse(backupCodesJson).get<std::vector<std::string>>();
                for (auto it = backupCodes.begin(); it != backupCodes.end(); ++it)
                {
                    if (*it == code)
                    {
                        // Удаляем использованный код
                        backupCodes.erase(it);
                        std::string newJson = nlohmann::json(backupCodes).dump();
                        dbClient_->execSqlSync(
                            "UPDATE two_factor_auth SET backup_codes = $1 WHERE user_id = $2",
                            newJson, userId);
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
            // Формат хранения: $<opslimit>$<memlimit>$<salt_hex>$<hash_hex>
            unsigned char salt[crypto_pwhash_SALTBYTES];
            randombytes_buf(salt, sizeof(salt));

            unsigned long long opslimit = crypto_pwhash_OPSLIMIT_MODERATE;
            size_t memlimit = crypto_pwhash_MEMLIMIT_MODERATE;

            constexpr size_t HASH_SIZE = 32;
            std::vector<unsigned char> hash(HASH_SIZE);
            if (crypto_pwhash(hash.data(), hash.size(),
                              password.c_str(), password.size(),
                              salt, opslimit, memlimit,
                              crypto_pwhash_ALG_ARGON2ID13) != 0)
            {
                throw std::runtime_error("Password hashing failed (out of memory?)");
            }

            // Сериализация в строку
            std::stringstream ss;
            ss << "$" << opslimit << "$" << memlimit << "$";
            for (int i = 0; i < (int)sizeof(salt); i++)
                ss << std::hex << std::setfill('0') << std::setw(2) << (int)salt[i];
            ss << "$";
            for (auto b : hash)
                ss << std::hex << std::setfill('0') << std::setw(2) << (int)b;

            return ss.str();
        }

        bool AuthService::verifyPassword(const std::string &password, const std::string &hash)
        {
            // Парсинг формата: $opslimit$memlimit$salt$hash
            std::istringstream ss(hash);
            std::string token;

            std::getline(ss, token, '$'); // пустой до первого $
            unsigned long long opslimit;
            {
                std::string t;
                std::getline(ss, t, '$');
                opslimit = std::stoull(t);
            }
            size_t memlimit;
            {
                std::string t;
                std::getline(ss, t, '$');
                memlimit = std::stoull(t);
            }
            std::string saltHex, hashHex;
            std::getline(ss, saltHex, '$');
            std::getline(ss, hashHex, '$');

            constexpr size_t HASH_SIZE = 32;
            if (saltHex.size() != crypto_pwhash_SALTBYTES * 2 ||
                hashHex.size() != HASH_SIZE * 2)
            {
                return false;
            }

            // Декодирование соли
            std::vector<unsigned char> salt(crypto_pwhash_SALTBYTES);
            for (size_t i = 0; i < salt.size(); i++)
                std::istringstream(saltHex.substr(i * 2, 2)) >> std::hex >> (int &)salt[i];

            // Декодирование эталонного хеша
            std::vector<unsigned char> expectedHash(HASH_SIZE);
            for (size_t i = 0; i < expectedHash.size(); i++)
                std::istringstream(hashHex.substr(i * 2, 2)) >> std::hex >> (int &)expectedHash[i];

            // Вычисление хеша из пароля
            std::vector<unsigned char> computedHash(HASH_SIZE);
            if (crypto_pwhash(computedHash.data(), computedHash.size(),
                              password.c_str(), password.size(),
                              salt.data(), opslimit, memlimit,
                              crypto_pwhash_ALG_ARGON2ID13) != 0)
            {
                return false;
            }

            return crypto_verify_32(expectedHash.data(), computedHash.data()) == 0;
        }

    } // namespace services
} // namespace wtld
