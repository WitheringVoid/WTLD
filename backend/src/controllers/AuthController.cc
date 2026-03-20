#include "../../include/controllers/AuthController.h"
#include <drogon/HttpResponse.h>
#include <nlohmann/json.hpp>

namespace wtld {
namespace controllers {

AuthController::AuthController() {
    // В реальном коде нужно получать dbClient из конфигурации
    // auto dbClient = drogon::app().getDbClient();
    // authService_ = std::make_shared<services::AuthService>(dbClient);
}

void AuthController::registerUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }
    
    std::string username = (*json)["username"].get<std::string>();
    std::string email = (*json)["email"].get<std::string>();
    std::string password = (*json)["password"].get<std::string>();
    
    // Валидация
    if (username.empty() || email.empty() || password.empty()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("All fields are required");
        callback(resp);
        return;
    }
    
    auto user = authService_->registerUser(username, email, password);
    
    if (user) {
        nlohmann::json response;
        response["status"] = "success";
        response["data"] = user->toJson();
        
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k409Conflict);
        resp->setBody("User already exists");
        callback(resp);
    }
}

void AuthController::login(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }
    
    std::string username = (*json)["username"].get<std::string>();
    std::string password = (*json)["password"].get<std::string>();
    
    auto user = authService_->authenticate(username, password);
    
    if (user) {
        auto token = authService_->generateToken(*user);
        
        nlohmann::json response;
        response["status"] = "success";
        response["token"] = token;
        response["user"] = user->toJson();
        
        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    } else {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        resp->setBody("Invalid credentials");
        callback(resp);
    }
}

void AuthController::logout(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    
    // JWT stateless, просто возвращаем успех
    nlohmann::json response;
    response["status"] = "success";
    response["message"] = "Logged out successfully";
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void AuthController::getProfile(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback) {
    
    // Здесь нужно получить user_id из JWT токена
    // Временно возвращаем заглушку
    nlohmann::json response;
    response["status"] = "error";
    response["message"] = "Not implemented";
    
    auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
    resp->setStatusCode(drogon::k501NotImplemented);
    callback(resp);
}

} // namespace controllers
} // namespace wtld