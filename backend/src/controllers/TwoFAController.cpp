#include "../../include/controllers/TwoFAController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpAppFramework.h>
#include <nlohmann/json.hpp>

namespace wtld
{
    namespace controllers
    {

        TwoFAController::TwoFAController()
        {
            dbClient_ = drogon::app().getDbClient();
            twoFAService_ = std::make_shared<services::TwoFactorAuthService>(dbClient_);
        }

        void TwoFAController::setup2FA(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto userId = getUserIdFromRequest(req);
                auto secret = twoFAService_->generateSecret();
                auto user = drogon::app().getDbClient();

                // Получаем username из БД
                auto dbResult = user->execSqlSync("SELECT username FROM users WHERE id = $1", userId);
                if (dbResult.size() == 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody("User not found");
                    callback(resp);
                    return;
                }

                std::string username = dbResult[0]["username"].as<std::string>();
                auto qrUri = twoFAService_->generateQRCodeUri(username, secret);
                auto backupCodes = twoFAService_->generateBackupCodes();
                twoFAService_->saveUser2FAInfo(userId, secret, backupCodes);

                nlohmann::json jsonResult;
                jsonResult["secret"] = secret;
                jsonResult["qr_uri"] = qrUri;
                jsonResult["backup_codes"] = backupCodes;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(jsonResult.dump());
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

        void TwoFAController::verify2FA(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto json = nlohmann::json::parse(req->body());
                auto userId = getUserIdFromRequest(req);
                std::string code = json.value("code", "");

                auto secretOpt = twoFAService_->getUserSecret(userId);
                if (!secretOpt)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("2FA not setup");
                    callback(resp);
                    return;
                }

                bool valid = twoFAService_->verifyCode(*secretOpt, code);
                if (!valid)
                {
                    valid = twoFAService_->verifyBackupCode(userId, code);
                }

                nlohmann::json result;
                result["valid"] = valid;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
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

        void TwoFAController::enable2FA(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto userId = getUserIdFromRequest(req);
                bool success = twoFAService_->enable2FA(userId);

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(success ? drogon::k200OK : drogon::k500InternalServerError);
                resp->setBody(success ? "2FA enabled" : "Failed to enable 2FA");
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

        void TwoFAController::disable2FA(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto userId = getUserIdFromRequest(req);
                bool success = twoFAService_->disable2FA(userId);

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(success ? drogon::k200OK : drogon::k500InternalServerError);
                resp->setBody(success ? "2FA disabled" : "Failed to disable 2FA");
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

        void TwoFAController::get2FAStatus(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto userId = getUserIdFromRequest(req);
                bool enabled = twoFAService_->is2FAEnabled(userId);

                nlohmann::json result;
                result["enabled"] = enabled;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
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

        void TwoFAController::getBackupCodes(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                nlohmann::json result;
                result["message"] = "Use setup endpoint to get backup codes";

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
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

        int TwoFAController::getUserIdFromRequest(const drogon::HttpRequestPtr &req)
        {
            return req->attributes()->get<int>("userId");
        }

    } // namespace controllers
} // namespace wtld
