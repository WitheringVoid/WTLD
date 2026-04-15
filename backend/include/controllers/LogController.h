#pragma once
#include <drogon/HttpController.h>
#include <memory>
#include <drogon/orm/DbClient.h>
#include "../services/AuthService.h"
#include "../services/LogParserService.h"
#include "../services/LogAnalysisService.h"

namespace wtld
{
    namespace controllers
    {

        class LogController : public drogon::HttpController<LogController>
        {
        public:
            LogController();

            METHOD_LIST_BEGIN
            ADD_METHOD_TO(LogController::uploadLog, "/api/logs/upload", drogon::Post);
            ADD_METHOD_TO(LogController::getLogs, "/api/logs", drogon::Get);
            ADD_METHOD_TO(LogController::getLogById, "/api/logs/{id}", drogon::Get);
            ADD_METHOD_TO(LogController::deleteLog, "/api/logs/{id}", drogon::Delete);
            ADD_METHOD_TO(LogController::getLogStats, "/api/logs/{id}/stats", drogon::Get);
            METHOD_LIST_END

            void uploadLog(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void getLogs(const drogon::HttpRequestPtr &req,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void getLogById(const drogon::HttpRequestPtr &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void deleteLog(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void getLogStats(const drogon::HttpRequestPtr &req,
                             std::function<void(const drogon::HttpResponsePtr &)> &&callback);

        private:
            std::shared_ptr<services::LogParserService> logParserService_;
            std::shared_ptr<services::LogAnalysisService> logAnalysisService_;
            drogon::orm::DbClientPtr dbClient_;
        };

    } // namespace controllers
} // namespace wtld
