#include "../../include/controllers/AuthController.h"
#include "../../include/services/TwoFactorAuthService.h"
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
                    resp->setBody("{\"status\":\"error\",\"message\":\"All fields are required\"}");
                    callback(resp);
                    return;
                }

                auto user = authService_->registerUser(username, email, password);
                if (!user)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k409Conflict);
                    resp->setBody("{\"status\":\"error\",\"message\":\"User already exists\"}");
                    callback(resp);
                    return;
                }

                auto token = authService_->generateToken(*user);
                nlohmann::json result;
                result["status"] = "success";
                result["token"] = token;
                result["user"] = user->toJson();

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
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
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
                    resp->setBody("{\"status\":\"error\",\"message\":\"username and password are required\"}");
                    callback(resp);
                    return;
                }

                auto user = authService_->authenticate(username, password);
                if (!user)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Invalid credentials\"}");
                    callback(resp);
                    return;
                }

                if (authService_->isTwoFactorEnabled(user->id))
                {
                    nlohmann::json result;
                    result["status"] = "two_factor_required";
                    result["two_factor_token"] = authService_->generateTwoFactorToken(*user);

                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k200OK);
                    resp->setContentTypeString("application/json");
                    resp->setBody(result.dump());
                    callback(resp);
                    return; // Полный JWT НЕ выдаём, прерываем выполнение
                }

                if (authService_->isTwoFactorEnabled(user->id))
                {
                    nlohmann::json tfaResult;
                    tfaResult["status"] = "two_factor_required";
                    tfaResult["two_factor_token"] = authService_->generateTwoFactorToken(*user);
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k200OK);
                    resp->setContentTypeString("application/json");
                    resp->setBody(tfaResult.dump());
                    callback(resp);
                    return;
                }

                auto token = authService_->generateToken(*user);
                nlohmann::json result;
                result["status"] = "success";
                result["token"] = token;
                result["user"] = user->toJson();

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
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
                callback(resp);
            }
        }

        void AuthController::logout(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            nlohmann::json result;
            result["status"] = "success";
            result["message"] = "Logged out successfully";

            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->setContentTypeString("application/json");
            resp->setBody(result.dump());
            callback(resp);
        }

        void AuthController::getProfile(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = -1;
                try
                {
                    userId = req->attributes()->get<int>("userId");
                }
                catch (...)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Unauthorized\"}");
                    callback(resp);
                    return;
                }

                auto user = authService_->getUserById(userId);
                if (!user)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody("{\"status\":\"error\",\"message\":\"User not found\"}");
                    callback(resp);
                    return;
                }

                nlohmann::json result;
                result["status"] = "success";
                result["data"] = user->toJson();

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
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
                callback(resp);
            }
        }

        void AuthController::verifyTwoFactorLogin(const drogon::HttpRequestPtr &req,
                                                  std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto json = nlohmann::json::parse(req->body());
                std::string preToken = json.value("two_factor_token", "");
                std::string code = json.value("code", "");
                if (preToken.empty() || code.empty())
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("{\"status\":\"error\",\"message\":\"two_factor_token and code are required\"}");
                    callback(resp);
                    return;
                }
                auto userIdOpt = authService_->validateTwoFactorToken(preToken);
                if (!userIdOpt)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Invalid or expired two-factor token\"}");
                    callback(resp);
                    return;
                }
                auto db = drogon::app().getDbClient();
                services::TwoFactorAuthService tfaService(db);
                auto secretOpt = tfaService.getUserSecret(*userIdOpt);
                bool ok = secretOpt.has_value() && tfaService.verifyCode(*secretOpt, code);
                if (!ok)
                {
                    // 400, а не 401: чтобы фронт не перезагружал страницу при ошибочном коде
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Invalid code\"}");
                    callback(resp);
                    return;
                }
                auto user = authService_->getUserById(*userIdOpt);
                if (!user)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"User not found\"}");
                    callback(resp);
                    return;
                }
                auto token = authService_->generateToken(*user);
                nlohmann::json result;
                result["status"] = "success";
                result["token"] = token;
                result["user"] = user->toJson();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "2FA login error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
                callback(resp);
            }
        }

    } // namespace controllers
} // namespace wtld
