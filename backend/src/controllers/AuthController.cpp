#include "../../include/controllers/AuthController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpAppFramework.h>
#include <nlohmann/json.hpp>

namespace wtld
{
    namespace controllers
    {

        AuthController::AuthController()
        {
            authService_ = std::make_shared<services::AuthService>(drogon::app().getDbClient());
        }

        void AuthController::registerUser(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto json = nlohmann::json::parse(req->body());
                std::string username = json.value("username", "");
                std::string email = json.value("email", "");
                std::string password = json.value("password", "");

                if (username.empty() || email.empty() || password.empty())
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("username, email and password are required");
                    callback(resp);
                    return;
                }

                auto user = authService_->registerUser(username, email, password);
                if (!user)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k409Conflict);
                    resp->setBody("User already exists");
                    callback(resp);
                    return;
                }

                auto token = authService_->generateToken(*user);
                nlohmann::json result;
                result["user"] = user->toJson();
                result["token"] = token;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k201Created);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Register error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void AuthController::login(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto json = nlohmann::json::parse(req->body());
                std::string username = json.value("username", "");
                std::string password = json.value("password", "");

                if (username.empty() || password.empty())
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("username and password are required");
                    callback(resp);
                    return;
                }

                auto user = authService_->authenticate(username, password);
                if (!user)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Invalid credentials");
                    callback(resp);
                    return;
                }

                auto token = authService_->generateToken(*user);
                nlohmann::json result;
                result["user"] = user->toJson();
                result["token"] = token;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Login error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void AuthController::logout(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->setBody("Logged out successfully");
            callback(resp);
        }

        void AuthController::getProfile(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto userId = req->attributes()->get<int>("userId");
                auto user = authService_->getUserById(userId);
                if (!user)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody("User not found");
                    callback(resp);
                    return;
                }

                nlohmann::json result = user->toJson();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get profile error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

    } // namespace controllers
} // namespace wtld
