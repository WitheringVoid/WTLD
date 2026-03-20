#include "../../include/services/AuthService.h"
#include <drogon/orm/Result.h>
#include <jwt-cpp/jwt.h>
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace wtld {
namespace services {

AuthService::AuthService(const drogon::orm::DbClientPtr& dbClient)
    : dbClient_(dbClient) {}

std::string AuthService::hashPassword(const std::string& password) {
    // Простой хэш для демо (в продакшене используйте bcrypt или argon2)
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.size());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool AuthService::verifyPassword(const std::string& password, const std::string& hash) {
    return hashPassword(password) == hash;
}

std::optional<models::User> AuthService::registerUser(
    const std::string& username,
    const std::string& email,
    const std::string& password) {
    
    auto hashedPassword = hashPassword(password);
    
    try {
        auto result = dbClient_->execSqlCoro(
            "INSERT INTO users (username, email, password_hash) "
            "VALUES ($1, $2, $3) RETURNING id, created_at, updated_at",
            username, email, hashedPassword
        ).get();
        
        if (!result.empty()) {
            models::User user;
            user.id = result[0]["id"].as<int>();
            user.username = username;
            user.email = email;
            user.password_hash = hashedPassword;
            user.created_at = result[0]["created_at"].as<std::time_t>();
            user.updated_at = result[0]["updated_at"].as<std::time_t>();
            user.is_active = true;
            
            return user;
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Registration failed: " << e.what();
    }
    
    return std::nullopt;
}

std::optional<models::User> AuthService::authenticate(
    const std::string& username,
    const std::string& password) {
    
    try {
        auto result = dbClient_->execSqlCoro(
            "SELECT id, username, email, password_hash, created_at, updated_at, "
            "last_login, is_active FROM users WHERE username = $1 AND is_active = true",
            username
        ).get();
        
        if (!result.empty()) {
            auto hash = result[0]["password_hash"].as<std::string>();
            if (verifyPassword(password, hash)) {
                models::User user;
                user.id = result[0]["id"].as<int>();
                user.username = result[0]["username"].as<std::string>();
                user.email = result[0]["email"].as<std::string>();
                user.password_hash = hash;
                user.created_at = result[0]["created_at"].as<std::time_t>();
                user.updated_at = result[0]["updated_at"].as<std::time_t>();
                user.last_login = result[0]["last_login"].as<std::time_t>();
                user.is_active = result[0]["is_active"].as<bool>();
                
                // Обновляем время последнего входа
                dbClient_->execSqlCoro(
                    "UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = $1",
                    user.id
                ).get();
                
                return user;
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Authentication failed: " << e.what();
    }
    
    return std::nullopt;
}

std::string AuthService::generateToken(const models::User& user) {
    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::hours(24);
    
    auto token = jwt::create()
        .set_type("JWT")
        .set_issuer("wtld-backend")
        .set_subject(std::to_string(user.id))
        .set_issued_at(now)
        .set_expires_at(expires)
        .set_payload_claim("username", jwt::claim(user.username))
        .sign(jwt::algorithm::hs256{"your-secret-key-change-this-in-production"});
    
    return token;
}

std::optional<models::User> AuthService::validateToken(const std::string& token) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{"your-secret-key-change-this-in-production"})
            .with_issuer("wtld-backend");
        
        verifier.verify(decoded);
        
        int userId = std::stoi(decoded.get_subject());
        return getUserById(userId);
        
    } catch (const std::exception& e) {
        LOG_ERROR << "Token validation failed: " << e.what();
        return std::nullopt;
    }
}

std::optional<models::User> AuthService::getUserById(int userId) {
    try {
        auto result = dbClient_->execSqlCoro(
            "SELECT id, username, email, password_hash, created_at, updated_at, "
            "last_login, is_active FROM users WHERE id = $1",
            userId
        ).get();
        
        if (!result.empty()) {
            models::User user;
            user.id = result[0]["id"].as<int>();
            user.username = result[0]["username"].as<std::string>();
            user.email = result[0]["email"].as<std::string>();
            user.password_hash = result[0]["password_hash"].as<std::string>();
            user.created_at = result[0]["created_at"].as<std::time_t>();
            user.updated_at = result[0]["updated_at"].as<std::time_t>();
            user.last_login = result[0]["last_login"].as<std::time_t>();
            user.is_active = result[0]["is_active"].as<bool>();
            
            return user;
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Get user failed: " << e.what();
    }
    
    return std::nullopt;
}

} // namespace services
} // namespace wtld