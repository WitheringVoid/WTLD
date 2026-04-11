#include "../../include/controllers/AnalyticsController.h"
#include "../../include/services/AuthService.h"
#include <drogon/HttpResponse.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

namespace wtld
{
    namespace controllers
    {

        // Хелпер для парсинга PostgreSQL array формата {a,b,c}
        static std::vector<std::string> parsePgArray(const std::string &raw)
        {
            std::vector<std::string> result;
            if (raw.size() <= 2)
                return result;
            std::string inner = raw.substr(1, raw.size() - 2);
            std::stringstream ss(inner);
            std::string item;
            while (std::getline(ss, item, ','))
            {
                if (item.size() >= 2 && item.front() == '"' && item.back() == '"')
                    item = item.substr(1, item.size() - 2);
                if (!item.empty())
                    result.push_back(item);
            }
            return result;
        }

        AnalyticsController::AnalyticsController()
        {
            dbClient_ = drogon::app().getDbClient();
            logAnalysisService_ = std::make_shared<services::LogAnalysisService>(dbClient_);
        }

        void AnalyticsController::getDashboard(const drogon::HttpRequestPtr &req,
                                               std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                // Общая статистика пользователя
                auto statsResult = dbClient_->execSqlSync(
                    "SELECT "
                    "  (SELECT COUNT(*) FROM logs WHERE user_id = $1) as total_logs,"
                    "  (SELECT COUNT(*) FROM logs WHERE user_id = $1 AND status = 'completed') as completed_logs,"
                    "  (SELECT COUNT(*) FROM log_analytics WHERE user_id = $1 AND is_anomaly = true) as total_anomalies,"
                    "  (SELECT COUNT(*) FROM analysis_rules WHERE user_id = $1 AND is_active = true) as active_rules",
                    userId);

                const auto &row = statsResult[0];
                nlohmann::json stats;
                stats["total_logs"] = row["total_logs"].as<int>();
                stats["completed_logs"] = row["completed_logs"].as<int>();
                stats["total_anomalies"] = row["total_anomalies"].as<int>();
                stats["active_rules"] = row["active_rules"].as<int>();

                // Последние аномалии
                auto anomaliesResult = dbClient_->execSqlSync(
                    "SELECT id, analysis_type, severity_level, title, created_at "
                    "FROM log_analytics WHERE user_id = $1 AND is_anomaly = true "
                    "ORDER BY created_at DESC LIMIT 10",
                    userId);

                nlohmann::json recentAnomalies = nlohmann::json::array();
                for (const auto &aRow : anomaliesResult)
                {
                    nlohmann::json item;
                    item["id"] = aRow["id"].as<int>();
                    item["type"] = aRow["analysis_type"].as<std::string>();
                    item["severity"] = aRow["severity_level"].as<std::string>();
                    item["title"] = aRow["title"].as<std::string>();
                    item["created_at"] = aRow["created_at"].as<std::string>();
                    recentAnomalies.push_back(item);
                }

                // Статистика по уровням логов (агрегированная)
                auto levelsResult = dbClient_->execSqlSync(
                    "SELECT "
                    "  (SELECT COUNT(*) FROM log_analytics WHERE user_id = $1 AND severity_level = 'critical') as critical,"
                    "  (SELECT COUNT(*) FROM log_analytics WHERE user_id = $1 AND severity_level = 'high') as high,"
                    "  (SELECT COUNT(*) FROM log_analytics WHERE user_id = $1 AND severity_level = 'medium') as medium,"
                    "  (SELECT COUNT(*) FROM log_analytics WHERE user_id = $1 AND severity_level = 'low') as low",
                    userId);

                const auto &lRow = levelsResult[0];
                nlohmann::json severityStats;
                severityStats["critical"] = lRow["critical"].as<int>();
                severityStats["high"] = lRow["high"].as<int>();
                severityStats["medium"] = lRow["medium"].as<int>();
                severityStats["low"] = lRow["low"].as<int>();

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = {
                    {"statistics", stats},
                    {"recent_anomalies", recentAnomalies},
                    {"severity_distribution", severityStats}};

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
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

        void AnalyticsController::getAnalytics(const drogon::HttpRequestPtr &req,
                                               std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                auto logId = req->getParameter("logId");

                auto result = dbClient_->execSqlSync(
                    "SELECT id, analysis_type, severity_level, title, description, data, detected_patterns, is_anomaly, created_at "
                    "FROM log_analytics WHERE log_file_id = $1 AND user_id = $1",
                    logId, userId);

                nlohmann::json analytics = nlohmann::json::array();
                for (const auto &row : result)
                {
                    nlohmann::json item;
                    item["id"] = row["id"].as<int>();
                    item["type"] = row["analysis_type"].as<std::string>();
                    item["severity"] = row["severity_level"].as<std::string>();
                    item["title"] = row["title"].as<std::string>();
                    item["description"] = row["description"].as<std::string>();
                    item["data"] = nlohmann::json::parse(row["data"].as<std::string>());
                    item["detected_patterns"] = parsePgArray(row["detected_patterns"].as<std::string>());
                    item["is_anomaly"] = row["is_anomaly"].as<bool>();
                    item["created_at"] = row["created_at"].as<std::string>();
                    analytics.push_back(item);
                }

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = analytics;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
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

        void AnalyticsController::getAnomalies(const drogon::HttpRequestPtr &req,
                                               std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                auto logId = req->getParameter("logId");

                auto result = dbClient_->execSqlSync(
                    "SELECT id, analysis_type, severity_level, title, description, data, created_at "
                    "FROM log_analytics WHERE log_file_id = $1 AND user_id = $2 AND is_anomaly = true "
                    "ORDER BY created_at DESC",
                    logId, userId);

                nlohmann::json anomalies = nlohmann::json::array();
                for (const auto &row : result)
                {
                    nlohmann::json item;
                    item["id"] = row["id"].as<int>();
                    item["type"] = row["analysis_type"].as<std::string>();
                    item["severity"] = row["severity_level"].as<std::string>();
                    item["title"] = row["title"].as<std::string>();
                    item["description"] = row["description"].as<std::string>();
                    item["data"] = nlohmann::json::parse(row["data"].as<std::string>());
                    item["created_at"] = row["created_at"].as<std::string>();
                    anomalies.push_back(item);
                }

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = anomalies;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
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

        void AnalyticsController::getRules(const drogon::HttpRequestPtr &req,
                                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                auto rules = logAnalysisService_->loadUserRules(userId);

                nlohmann::json rulesJson = nlohmann::json::array();
                for (const auto &rule : rules)
                {
                    nlohmann::json item;
                    item["id"] = rule.id;
                    item["name"] = rule.name;
                    item["type"] = rule.type;
                    item["pattern"] = rule.pattern;
                    item["severity"] = rule.severity;
                    item["is_active"] = rule.isActive;
                    rulesJson.push_back(item);
                }

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = rulesJson;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
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

        void AnalyticsController::createRule(const drogon::HttpRequestPtr &req,
                                             std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
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

                std::string name = (*json)["name"].asString();
                std::string type = (*json)["type"].asString();
                std::string pattern = (*json)["pattern"].asString();
                std::string severity = (*json).get("severity", "medium").asString();

                if (name.empty() || type.empty() || pattern.empty())
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("Name, type, and pattern are required");
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

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = {{"id", ruleId}};

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                resp->setStatusCode(drogon::k201Created);
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

        void AnalyticsController::deleteRule(const drogon::HttpRequestPtr &req,
                                             std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("Unauthorized");
                    callback(resp);
                    return;
                }

                auto id = req->getParameter("id");

                // Проверка что правило принадлежит пользователю
                auto checkResult = dbClient_->execSqlSync(
                    "SELECT id FROM analysis_rules WHERE id = $1 AND user_id = $2",
                    id, userId);

                if (checkResult.size() == 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody("Rule not found");
                    callback(resp);
                    return;
                }

                if (!logAnalysisService_->deleteRule(std::stoi(id)))
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k500InternalServerError);
                    resp->setBody("Failed to delete rule");
                    callback(resp);
                    return;
                }

                nlohmann::json response;
                response["status"] = "success";
                response["message"] = "Rule deleted";

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
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
            try
            {
                return req->attributes()->get<int>("userId");
            }
            catch (...)
            {
            }

            auto authHeader = req->getHeader("Authorization");
            if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
            {
                return -1;
            }

            auto token = authHeader.substr(7);
            auto authService = std::make_shared<services::AuthService>(dbClient_);
            auto user = authService->validateToken(token);

            if (user)
            {
                req->attributes()->insert("userId", user->id);
                return user->id;
            }

            return -1;
        }

    } // namespace controllers
} // namespace wtld
