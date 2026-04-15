#include "../../include/controllers/TwoFAController.h"
#include "../../include/utils/AuthUtils.h"
#include "../../include/services/TwoFactorAuthService.h"
#include <drogon/HttpResponse.h>
#include <nlohmann/json.hpp>
#include <sstream>

namespace wtld
{
    namespace controllers
    {

        TwoFAController::TwoFAController()
        {
            dbClient_ = drogon::app().getDbClient();
            twoFAService_ = std::make_shared<services::TwoFactorAuthService>(dbClient_);
        }

        void TwoFAController::setup2FA(const drogon::HttpRequestPtr &req,
                                       std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = utils::getUserIdFromRequest(req, dbClient_);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                // Проверка, не включен ли уже 2FA
                if (twoFAService_->is2FAEnabled(userId))
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("2FA is already enabled");
                    callback(resp);
                    return;
                }

                // Генерация секрета
                std::string secret = twoFAService_->generateSecret();

                // Получение username пользователя
                auto userResult = dbClient_->execSqlSync("SELECT username FROM users WHERE id = $1", userId);
                std::string username = userResult[0]["username"].as<std::string>();

                // Генерация URI для QR-кода
                std::string qrUri = twoFAService_->generateQRCodeUri(username, secret);

                // Сохранение секрета и резервных кодов
                auto backupCodes = twoFAService_->generateBackupCodes();
                if (!twoFAService_->saveUser2FAInfo(userId, secret, backupCodes))
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k500InternalServerError);
                    resp->setBody("Failed to save 2FA info");
                    callback(resp);
                    return;
                }

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = {
                    {"secret", secret},
                    {"qr_uri", qrUri},
                    {"backup_codes", backupCodes}};

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Setup 2FA error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void TwoFAController::verify2FA(const drogon::HttpRequestPtr &req,
                                        std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = utils::getUserIdFromRequest(req, dbClient_);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                auto json = req->getJsonObject();
                if (!json)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("Invalid JSON");
                    callback(resp);
                    return;
                }

                std::string code = (*json)["code"].asString();

                if (code.empty() || code.length() != 6)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("Invalid code format");
                    callback(resp);
                    return;
                }

                // Проверка кода
                auto secretOpt = twoFAService_->getUserSecret(userId);
                if (!secretOpt)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("2FA not configured");
                    callback(resp);
                    return;
                }

                bool isValid = twoFAService_->verifyCode(*secretOpt, code);

                nlohmann::json response;
                response["status"] = isValid ? "success" : "error";
                response["valid"] = isValid;

                if (!isValid)
                {
                    response["message"] = "Invalid code";
                }

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Verify 2FA error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void TwoFAController::enable2FA(const drogon::HttpRequestPtr &req,
                                        std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = utils::getUserIdFromRequest(req, dbClient_);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                if (!twoFAService_->enable2FA(userId))
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k500InternalServerError);
                    resp->setBody("Failed to enable 2FA");
                    callback(resp);
                    return;
                }

                nlohmann::json response;
                response["status"] = "success";
                response["message"] = "2FA enabled successfully";

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Enable 2FA error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void TwoFAController::disable2FA(const drogon::HttpRequestPtr &req,
                                         std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = utils::getUserIdFromRequest(req, dbClient_);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                if (!twoFAService_->disable2FA(userId))
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k500InternalServerError);
                    resp->setBody("Failed to disable 2FA");
                    callback(resp);
                    return;
                }

                nlohmann::json response;
                response["status"] = "success";
                response["message"] = "2FA disabled successfully";

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Disable 2FA error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void TwoFAController::get2FAStatus(const drogon::HttpRequestPtr &req,
                                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = utils::getUserIdFromRequest(req, dbClient_);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                bool isEnabled = twoFAService_->is2FAEnabled(userId);

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = {
                    {"is_enabled", isEnabled},
                    {"is_configured", twoFAService_->getUserSecret(userId).has_value()}};

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get 2FA status error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void TwoFAController::getBackupCodes(const drogon::HttpRequestPtr &req,
                                             std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = utils::getUserIdFromRequest(req, dbClient_);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                auto result = dbClient_->execSqlSync(
                    "SELECT backup_codes FROM two_factor_auth WHERE user_id = $1",
                    userId);

                if (result.size() == 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody("{\"status\":\"error\",\"message\":\"2FA not configured\"}");
                    callback(resp);
                    return;
                }

                // PostgreSQL array format: {code1,code2,...}
                std::string rawCodes = result[0]["backup_codes"].as<std::string>();
                std::vector<std::string> backupCodes;
                if (rawCodes.size() > 2)
                {
                    // Убираем { и }
                    std::string inner = rawCodes.substr(1, rawCodes.size() - 2);
                    std::stringstream ss(inner);
                    std::string item;
                    while (std::getline(ss, item, ','))
                    {
                        // Убираем кавычки если есть
                        if (item.size() >= 2 && item.front() == '"' && item.back() == '"')
                            item = item.substr(1, item.size() - 2);
                        if (!item.empty())
                            backupCodes.push_back(item);
                    }
                }

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = {
                    {"backup_codes", backupCodes},
                    {"remaining", static_cast<int>(backupCodes.size())}};

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get backup codes error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

    } // namespace controllers
} // namespace wtld
