#pragma once
#include <drogon/HttpController.h>
#include <memory>
#include <drogon/orm/DbClient.h>
#include "../services/LogAnalysisService.h"

namespace wtld
{
    namespace controllers
    {

        class AnalyticsController : public drogon::HttpController<AnalyticsController>
        {
        public:
            AnalyticsController();

            METHOD_LIST_BEGIN
            ADD_METHOD_TO(AnalyticsController::getDashboard, "/api/analytics/dashboard", drogon::Get);
            ADD_METHOD_TO(AnalyticsController::getAnalytics, "/api/analytics/{logId}", drogon::Get);
            ADD_METHOD_TO(AnalyticsController::getAnomalies, "/api/analytics/{logId}/anomalies", drogon::Get);
            ADD_METHOD_TO(AnalyticsController::getRules, "/api/analytics/rules", drogon::Get);
            ADD_METHOD_TO(AnalyticsController::createRule, "/api/analytics/rules", drogon::Post);
            ADD_METHOD_TO(AnalyticsController::deleteRule, "/api/analytics/rules/{id}", drogon::Delete);
            METHOD_LIST_END

            void getDashboard(const drogon::HttpRequestPtr &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void getAnalytics(const drogon::HttpRequestPtr &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void getAnomalies(const drogon::HttpRequestPtr &req,
                              std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void getRules(const drogon::HttpRequestPtr &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void createRule(const drogon::HttpRequestPtr &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback);

            void deleteRule(const drogon::HttpRequestPtr &req,
                            std::function<void(const drogon::HttpResponsePtr &)> &&callback);

        private:
            std::shared_ptr<services::LogAnalysisService> logAnalysisService_;
            drogon::orm::DbClientPtr dbClient_;
        };

    } // namespace controllers
} // namespace wtld
