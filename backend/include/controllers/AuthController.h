#pragma once
#include <drogon/HttpController.h>
#include <memory>
#include "../services/AuthService.h"

namespace wtld {
namespace controllers {

class AuthController : public drogon::HttpController<AuthController> {
public:
    AuthController();
    
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", drogon::Post);
        ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);
        ADD_METHOD_TO(AuthController::logout, "/api/auth/logout", drogon::Post);
        ADD_METHOD_TO(AuthController::getProfile, "/api/auth/profile", drogon::Get);
    METHOD_LIST_END
    
    void registerUser(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    
    void login(const drogon::HttpRequestPtr& req,
              std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    
    void logout(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    
    void getProfile(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback);

private:
    std::shared_ptr<services::AuthService> authService_;
};

} // namespace controllers
} // namespace wtld