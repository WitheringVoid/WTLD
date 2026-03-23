#include "../../include/controllers/LogController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <nlohmann/json.hpp>
#include <chrono>

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
            resp->setBody("Unauthorized");
            callback(resp);
            return;
        }

        // Получение файла
        auto &fileMap = req->getFiles();
        if (fileMap.empty())
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("No file uploaded");
            callback(resp);
            return;
        }

        const auto &file = fileMap.begin()->second;
        std::string content(file->fileContent().data(), file->fileContent().size());
        std::string filename = file->getFileName();
        long fileSize = content.size();

        // Определение типа файла
        std::string fileType = "txt";
        if (filename.size() > 5 && filename.substr(filename.size() - 5) == ".json")
        {
            fileType = "json";
        }

        // Парсинг логов
        auto entries = logParserService_->parseLogs(content, fileType);

        // Сохранение в БД
        auto result = dbClient_->execSqlSync(
            "INSERT INTO logs (user_id, filename, original_content, parsed_content, file_type, file_size, status) "
            "VALUES ($1, $2, $3, $4, $5, $6, 'processing') RETURNING id",
            userId, filename, content, nlohmann::json(entries).dump(), fileType, fileSize);

        int logId = result[0]["id"].as<int>();

        // Анализ логов (асинхронно можно вынести в очередь)
        auto analysisResults = logAnalysisService_->analyzeLogs(logId, userId, entries);

        // Обновление статуса
        dbClient_->execSqlSync("UPDATE logs SET status = 'completed' WHERE id = $1", logId);

        // Формирование ответа
        nlohmann::json response;
        response["status"] = "success";
        response["data"] = {
            {"id", logId},
            {"filename", filename},
            {"file_type", fileType},
            {"file_size", fileSize},
            {"entries_count", entries.size()},
            {"anomalies_found", std::count_if(analysisResults.begin(), analysisResults.end(),
                                               [](const auto &r)
                                               { return r.isAnomaly; })}};

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        resp->setStatusCode(drogon::k201Created);
        callback(resp);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Upload log error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
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
            resp->setBody("Unauthorized");
            callback(resp);
            return;
        }

        // Получение параметров фильтрации
        std::string status = req->getParameter("status");
        std::string fileType = req->getParameter("fileType");
        int limit = 20;

        if (auto p = req->getParameter("limit"))
        {
            limit = std::stoi(p);
        }

        // Формирование запроса
        std::string query = "SELECT id, filename, file_type, file_size, upload_date, status "
                            "FROM logs WHERE user_id = $1";

        std::vector<std::string> params;
        params.push_back(std::to_string(userId));
        int paramCount = 1;

        if (!status.empty())
        {
            query += " AND status = $" + std::to_string(++paramCount);
            params.push_back(status);
        }

        if (!fileType.empty())
        {
            query += " AND file_type = $" + std::to_string(++paramCount);
            params.push_back(fileType);
        }

        query += " ORDER BY upload_date DESC LIMIT $" + std::to_string(++paramCount);
        params.push_back(std::to_string(limit));

        auto result = dbClient_->execSqlSync(query, params);

        nlohmann::json logs = nlohmann::json::array();
        for (const auto &row : result)
        {
            nlohmann::json log;
            log["id"] = row["id"].as<int>();
            log["filename"] = row["filename"].as<std::string>();
            log["file_type"] = row["file_type"].as<std::string>();
            log["file_size"] = row["file_size"].as<long>();
            log["upload_date"] = row["upload_date"].as<std::string>();
            log["status"] = row["status"].as<std::string>();
            logs.push_back(log);
        }

        nlohmann::json response;
        response["status"] = "success";
        response["data"] = logs;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Get logs error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
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
            resp->setBody("Unauthorized");
            callback(resp);
            return;
        }

        auto id = req->getParameter("id");

        auto result = dbClient_->execSqlSync(
            "SELECT id, filename, file_type, file_size, upload_date, status, parsed_content "
            "FROM logs WHERE id = $1 AND user_id = $2",
            id, userId);

        if (result.size() == 0)
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            resp->setBody("Log not found");
            callback(resp);
            return;
        }

        const auto &row = result[0];
        nlohmann::json log;
        log["id"] = row["id"].as<int>();
        log["filename"] = row["filename"].as<std::string>();
        log["file_type"] = row["file_type"].as<std::string>();
        log["file_size"] = row["file_size"].as<long>();
        log["upload_date"] = row["upload_date"].as<std::string>();
        log["status"] = row["status"].as<std::string>();
        log["entries"] = nlohmann::json::parse(row["parsed_content"].as<std::string>());

        nlohmann::json response;
        response["status"] = "success";
        response["data"] = log;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Get log by ID error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
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
            resp->setBody("Unauthorized");
            callback(resp);
            return;
        }

        auto id = req->getParameter("id");

        dbClient_->execSqlSync("DELETE FROM logs WHERE id = $1 AND user_id = $2", id, userId);

        nlohmann::json response;
        response["status"] = "success";
        response["message"] = "Log deleted";

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Delete log error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
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
            resp->setBody("Unauthorized");
            callback(resp);
            return;
        }

        auto id = req->getParameter("id");

        // Получение логов
        auto logResult = dbClient_->execSqlSync(
            "SELECT parsed_content FROM logs WHERE id = $1 AND user_id = $2",
            id, userId);

        if (logResult.size() == 0)
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k404NotFound);
            resp->setBody("Log not found");
            callback(resp);
            return;
        }

        auto entries = nlohmann::json::parse(logResult[0]["parsed_content"].as<std::string>())
                           .get<std::vector<models::LogEntry>>();

        // Расчет статистики
        auto stats = logAnalysisService_->calculateStatistics(entries);

        // Получение аналитики из БД
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

        auto resp = drogon::HttpResponse::newHttpJsonResponse(response);
        callback(resp);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Get log stats error: " << e.what();
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        resp->setBody("Internal server error");
        callback(resp);
    }
}

int LogController::getUserIdFromRequest(const drogon::HttpRequestPtr &req)
{
    // Получение user_id из JWT токена (через атрибут запроса)
    auto userIdPtr = req->getAttachment<int>("userId");
    if (userIdPtr)
    {
        return *userIdPtr;
    }

    // Альтернативно: извлечение из заголовка Authorization
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
    {
        return -1;
    }

    // Валидация токена через AuthService
    auto token = authHeader.substr(7);
    auto authService = std::make_shared<services::AuthService>(dbClient_);
    auto user = authService->validateToken(token);

    if (user)
    {
        req->setAttribute("userId", user->id);
        return user->id;
    }

    return -1;
}

} // namespace controllers
} // namespace wtld
