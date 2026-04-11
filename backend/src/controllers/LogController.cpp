#include "../../include/controllers/LogController.h"
#include "../../include/services/AuthService.h"
#include <drogon/HttpResponse.h>
#include <drogon/MultiPart.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>

// Хелпер для парсинга PostgreSQL array формата {a,b,c}
[[maybe_unused]] static std::vector<std::string> parsePgArray(const std::string &raw)
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

namespace wtld
{
    namespace controllers
    {

        LogController::LogController()
        {
            dbClient_ = drogon::app().getDbClient();
            logParserService_ = std::make_shared<services::LogParserService>();
            logAnalysisService_ = std::make_shared<services::LogAnalysisService>(dbClient_);
        }

        void LogController::uploadLog(const drogon::HttpRequestPtr &req,
                                      std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Unauthorized\"}");
                    callback(resp);
                    return;
                }

                drogon::MultiPartParser parser;
                if (parser.parse(req) != 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Failed to parse multipart data\"}");
                    callback(resp);
                    return;
                }

                auto &files = parser.getFiles();
                if (files.empty())
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k400BadRequest);
                    resp->setBody("{\"status\":\"error\",\"message\":\"No file uploaded\"}");
                    callback(resp);
                    return;
                }

                const auto &file = files[0];
                std::string content(file.fileContent().data(), file.fileContent().size());
                std::string filename = file.getFileName();
                long fileSize = static_cast<long>(content.size());

                std::string fileType = "txt";
                if (filename.size() > 5 && filename.substr(filename.size() - 5) == ".json")
                    fileType = "json";

                auto entries = logParserService_->parseLogs(content, fileType);

                nlohmann::json entriesJson = nlohmann::json::array();
                for (const auto &entry : entries)
                    entriesJson.push_back(entry.toJson());

                auto result = dbClient_->execSqlSync(
                    "INSERT INTO logs (user_id, filename, original_content, parsed_content, file_type, file_size, status) "
                    "VALUES ($1, $2, $3, $4, $5, $6, 'processing') RETURNING id",
                    userId, filename, content, entriesJson.dump(), fileType, fileSize);

                int logId = result[0]["id"].as<int>();
                auto analysisResults = logAnalysisService_->analyzeLogs(logId, userId, entries);
                dbClient_->execSqlSync("UPDATE logs SET status = 'completed' WHERE id = $1", logId);

                int anomaliesCount = 0;
                for (const auto &r : analysisResults)
                    if (r.isAnomaly)
                        anomaliesCount++;

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = {
                    {"id", logId},
                    {"filename", filename},
                    {"file_type", fileType},
                    {"file_size", fileSize},
                    {"entries_count", static_cast<int>(entries.size())},
                    {"anomalies_found", anomaliesCount}};

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                resp->setStatusCode(drogon::k201Created);
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Upload log error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
                callback(resp);
            }
        }

        void LogController::getLogs(const drogon::HttpRequestPtr &req,
                                    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Unauthorized\"}");
                    callback(resp);
                    return;
                }

                std::string status = req->getParameter("status");
                std::string fileType = req->getParameter("fileType");
                int limit = 20;
                try
                {
                    auto limitStr = req->getParameter("limit");
                    if (!limitStr.empty())
                        limit = std::stoi(limitStr);
                }
                catch (...)
                {
                }

                drogon::orm::Result result;
                const char *base = "SELECT id, filename, file_type, file_size, upload_date, status FROM logs WHERE user_id = $1";

                if (!status.empty() && !fileType.empty())
                    result = dbClient_->execSqlSync(std::string(base) + " AND status = $2 AND file_type = $3 ORDER BY upload_date DESC LIMIT $4", userId, status, fileType, limit);
                else if (!status.empty())
                    result = dbClient_->execSqlSync(std::string(base) + " AND status = $2 ORDER BY upload_date DESC LIMIT $3", userId, status, limit);
                else if (!fileType.empty())
                    result = dbClient_->execSqlSync(std::string(base) + " AND file_type = $2 ORDER BY upload_date DESC LIMIT $3", userId, fileType, limit);
                else
                    result = dbClient_->execSqlSync(std::string(base) + " ORDER BY upload_date DESC LIMIT $2", userId, limit);

                nlohmann::json logs = nlohmann::json::array();
                for (const auto &row : result)
                {
                    nlohmann::json log;
                    log["id"] = row["id"].as<int>();
                    log["filename"] = row["filename"].as<std::string>();
                    log["file_type"] = row["file_type"].as<std::string>();
                    log["file_size"] = row["file_size"].as<long long>();
                    log["upload_date"] = row["upload_date"].as<std::string>();
                    log["status"] = row["status"].as<std::string>();
                    logs.push_back(log);
                }

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = logs;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get logs error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
                callback(resp);
            }
        }

        void LogController::getLogById(const drogon::HttpRequestPtr &req,
                                       std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Unauthorized\"}");
                    callback(resp);
                    return;
                }

                auto id = req->getParameter("id");
                auto result = dbClient_->execSqlSync(
                    "SELECT id, filename, file_type, file_size, upload_date, status, parsed_content "
                    "FROM logs WHERE id = $1 AND user_id = $2",
                    id, userId);

                if (result.empty())
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Log not found\"}");
                    callback(resp);
                    return;
                }

                const auto &row = result[0];
                nlohmann::json log;
                log["id"] = row["id"].as<int>();
                log["filename"] = row["filename"].as<std::string>();
                log["file_type"] = row["file_type"].as<std::string>();
                log["file_size"] = row["file_size"].as<long long>();
                log["upload_date"] = row["upload_date"].as<std::string>();
                log["status"] = row["status"].as<std::string>();

                std::string parsedContent = row["parsed_content"].as<std::string>();
                if (!parsedContent.empty() && parsedContent != "null")
                    log["entries"] = nlohmann::json::parse(parsedContent);
                else
                    log["entries"] = nlohmann::json::array();

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = log;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get log by ID error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
                callback(resp);
            }
        }

        void LogController::deleteLog(const drogon::HttpRequestPtr &req,
                                      std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Unauthorized\"}");
                    callback(resp);
                    return;
                }

                auto id = req->getParameter("id");
                dbClient_->execSqlSync("DELETE FROM logs WHERE id = $1 AND user_id = $2", id, userId);

                nlohmann::json response;
                response["status"] = "success";
                response["message"] = "Log deleted";

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Delete log error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
                callback(resp);
            }
        }

        void LogController::getLogStats(const drogon::HttpRequestPtr &req,
                                        std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                int userId = getUserIdFromRequest(req);
                if (userId < 0)
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k401Unauthorized);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Unauthorized\"}");
                    callback(resp);
                    return;
                }

                auto id = req->getParameter("id");
                auto logResult = dbClient_->execSqlSync(
                    "SELECT parsed_content FROM logs WHERE id = $1 AND user_id = $2",
                    id, userId);

                if (logResult.empty())
                {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    resp->setBody("{\"status\":\"error\",\"message\":\"Log not found\"}");
                    callback(resp);
                    return;
                }

                std::string parsedContent = logResult[0]["parsed_content"].as<std::string>();
                std::vector<models::LogEntry> entries;
                if (!parsedContent.empty() && parsedContent != "null")
                {
                    auto jsonEntries = nlohmann::json::parse(parsedContent);
                    for (const auto &je : jsonEntries)
                    {
                        models::LogEntry entry;
                        entry.timestamp = je.value("timestamp", "");
                        entry.level = je.value("level", "");
                        entry.message = je.value("message", "");
                        entry.source = je.value("source", "");
                        entry.metadata = je.value("metadata", nlohmann::json::object());
                        entries.push_back(entry);
                    }
                }

                auto stats = logAnalysisService_->calculateStatistics(entries);

                auto analyticsResult = dbClient_->execSqlSync(
                    "SELECT analysis_type, severity_level, title, is_anomaly, created_at "
                    "FROM log_analytics WHERE log_file_id = $1 AND user_id = $2",
                    id, userId);

                nlohmann::json analytics = nlohmann::json::array();
                for (const auto &row : analyticsResult)
                {
                    nlohmann::json item;
                    item["type"] = row["analysis_type"].as<std::string>();
                    item["severity"] = row["severity_level"].as<std::string>();
                    item["title"] = row["title"].as<std::string>();
                    item["is_anomaly"] = row["is_anomaly"].as<bool>();
                    item["created_at"] = row["created_at"].as<std::string>();
                    analytics.push_back(item);
                }

                nlohmann::json response;
                response["status"] = "success";
                response["data"] = {
                    {"statistics", stats},
                    {"analytics", analytics}};

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setContentTypeString("application/json");
                resp->setBody(response.dump());
                callback(resp);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Get log stats error: " << e.what();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                resp->setBody("{\"status\":\"error\",\"message\":\"Internal server error\"}");
                callback(resp);
            }
        }

        int LogController::getUserIdFromRequest(const drogon::HttpRequestPtr &req)
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
                return -1;

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
