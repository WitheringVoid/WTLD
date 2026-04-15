#pragma once
#include <drogon/HttpController.h>
#include <memory>
#include <drogon/orm/DbClient.h>
#include "../services/TwoFactorAuthService.h"

namespace wtld
{
    namespace controllers
    {

        class TwoFAController : public drogon::HttpController<TwoFAController>
        {
        public:
            TwoFAController();

            METHOD_LIST_BEGIN
            ADD_METHOD_TO(TwoFAController::setup2FA, "/api/2fa/setup", drogon::Post);
            ADD_METHOD_TO(TwoFAController::verify2FA, "/api/2fa/verify", drogon::Post);
            ADD_METHOD_TO(TwoFAController::enable2FA, "/api/2fa/enable", drogon::Post);
            ADD_METHOD_TO(TwoFAController::disable2FA, "/api/2fa/disable", drogon::Post);
            ADD_METHOD_TO(TwoFAController::get2FAStatus, "/api/2fa/status", drogon::Get);
            ADD_METHOD_TO(TwoFAController::getBackupCodes, "/api/2fa/backup-codes", drogon::Get);
            METHOD_LIST_END

            void setup2FA(const drogon::HttpRequestPtr &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void verify2FA(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void enable2FA(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void disable2FA(const drogon::HttpRequestPtr &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void get2FAStatus(const drogon::HttpRequestPtr &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void getBackupCodes(const drogon::HttpRequestPtr &req,
                                std::function<void(const drogon::HttpResponsePtr &)> &&callback);

        private:
            std::shared_ptr<services::TwoFactorAuthService> twoFAService_;
            drogon::orm::DbClientPtr dbClient_;
        };

    } // namespace controllers
} // namespace wtld
