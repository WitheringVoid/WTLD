#include "../../include/controllers/LogController.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpAppFramework.h>
#include <nlohmann/json.hpp>

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

        void LogController::uploadLog(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto userId = getUserIdFromRequest(req);
                auto content = std::string(req->body());
                auto fileType = req->getHeader("X-File-Type");
                if (fileType.empty())
                    fileType = "text";

                auto entries = logParserService_->parseLogs(content, fileType);
                auto stats = logAnalysisService_->calculateStatistics(entries);

                nlohmann::json result;
                result["status"] = "uploaded";
                result["entries_count"] = entries.size();
                result["statistics"] = stats;

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k201Created);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
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

        void LogController::getLogs(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                nlohmann::json result = nlohmann::json::array();
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
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

        void LogController::getLogById(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k404NotFound);
                resp->setBody("Log not found");
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

        void LogController::deleteLog(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setBody("Log deleted");
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

        void LogController::getLogStats(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            try
            {
                nlohmann::json stats;
                stats["total"] = 0;
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(stats.dump());
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
            return req->attributes()->get<int>("userId");
        }

    } // namespace controllers
} // namespace wtld
