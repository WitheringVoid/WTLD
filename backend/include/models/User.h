#pragma once
#include <string>
#include <ctime>
#include <nlohmann/json.hpp>

namespace wtld {
namespace models {

struct User {
    int id;
    std::string username;
    std::string email;
    std::string password_hash;
    std::time_t created_at;
    std::time_t updated_at;
    std::time_t last_login;
    bool is_active;

    nlohmann::json toJson() const {
        return nlohmann::json{
            {"id", id},
            {"username", username},
            {"email", email},
            {"created_at", created_at},
            {"last_login", last_login},
            {"is_active", is_active}
        };
    }
};

} // namespace models
} // namespace wtld