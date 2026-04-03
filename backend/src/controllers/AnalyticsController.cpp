#include "../../include/controllers/AnalyticsController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpAppFramework.h>
#include <nlohmann/json.hpp>

namespace wtld
{
    namespace controllers
    {

        AnalyticsController::AnalyticsController()
        {
            dbClient_ = drogon::app().getDbClient();
            logAnalysisService_ = std::make_shared<services::LogAnalysisService>(dbClient_);
        }

        void AnalyticsController::getDashboard(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                nlohmann::json dashboard;
                dashboard["status"] = "ok";
                dashboard["total_logs"] = 0;
                dashboard["total_anomalies"] = 0;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(dashboard.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get dashboard error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void AnalyticsController::getAnalytics(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                nlohmann::json analytics;
                analytics["status"] = "ok";

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(analytics.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get analytics error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void AnalyticsController::getAnomalies(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                nlohmann::json anomalies = nlohmann::json::array();

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(anomalies.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get anomalies error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void AnalyticsController::getRules(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto userId = getUserIdFromRequest(req);
                auto rules = logAnalysisService_->loadUserRules(userId);

                nlohmann::json result = nlohmann::json::array();
                for (const auto &rule : rules)
                {
                    nlohmann::json j;
                    j["id"] = rule.id;
                    j["name"] = rule.name;
                    j["type"] = rule.type;
                    j["pattern"] = rule.pattern;
                    j["severity"] = rule.severity;
                    j["is_active"] = rule.isActive;
                    result.push_back(j);
                }

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get rules error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void AnalyticsController::createRule(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto json = nlohmann::json::parse(req->body());
                auto userId = getUserIdFromRequest(req);
                std::string name = json.value("name", "");
                std::string type = json.value("type", "regex");
                std::string pattern = json.value("pattern", "");
                std::string severity = json.value("severity", "medium");

                if (name.empty() || pattern.empty())
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("name and pattern are required");
                    callback(resp);
                    return;
                }

                int ruleId = logAnalysisService_->createRule(userId, name, type, pattern, severity);
                if (ruleId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k500InternalServerError);
                    resp->setBody("Failed to create rule");
                    callback(resp);
                    return;
                }

                nlohmann::json result;
                result["id"] = ruleId;
                result["name"] = name;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k201Created);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Create rule error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        void AnalyticsController::deleteRule(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto idStr = req->getParameter("id");
                int ruleId = std::stoi(idStr);
                bool success = logAnalysisService_->deleteRule(ruleId);

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(success ? drogon::k200OK : drogon::k500InternalServerError);
                resp->setBody(success ? "Rule deleted" : "Failed to delete rule");
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Delete rule error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("Internal server error");
                callback(resp);
            }
        }

        int AnalyticsController::getUserIdFromRequest(const drogon::HttpRequestPtr &req)
        {
            return req->attributes()->get<int>("userId");
        }

    } // namespace controllers
} // namespace wtld
