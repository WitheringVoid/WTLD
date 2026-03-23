#include "../../include/services/WebSocketService.h"
#include <drogon/HttpAppFramework.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace wtld
{
namespace services
{

WebSocketService &WebSocketService::instance()
{
    static WebSocketService instance;
    return instance;
}

void WebSocketService::addClient(int userId, const drogon::WebSocketConnectionPtr &conn)
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    clientConnections[userId].insert(conn);
    LOG_INFO << "WebSocket client connected. User: " << userId;
}

void WebSocketService::removeClient(int userId, const drogon::WebSocketConnectionPtr &conn)
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    auto it = clientConnections.find(userId);
    if (it != clientConnections.end())
    {
        it->second.erase(conn);
        if (it->second.empty())
        {
            clientConnections.erase(it);
        }
        LOG_INFO << "WebSocket client disconnected. User: " << userId;
    }
}

void WebSocketService::sendEvent(int userId, const WebSocketEvent &event)
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    auto it = clientConnections.find(userId);
    if (it == clientConnections.end())
    {
        return;
    }

    auto jsonMessage = eventToJson(event).dump();

    // Отправка всем подключениям пользователя
    std::vector<drogon::WebSocketConnectionPtr> toRemove;
    for (const auto &conn : it->second)
    {
        if (conn->connected())
        {
            conn->send(jsonMessage);
        }
        else
        {
            toRemove.push_back(conn);
        }
    }

    // Удаление отключенных клиентов
    for (const auto &conn : toRemove)
    {
        it->second.erase(conn);
    }
}

void WebSocketService::notifyAnomaly(int userId, const std::string &title, const std::string &description,
                                     const nlohmann::json &data)
{
    WebSocketEvent event;
    event.type = WebSocketEventType::AnomalyDetected;
    event.title = title;
    event.message = description;
    event.data = data;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    event.timestamp = ss.str();

    sendEvent(userId, event);
}

void WebSocketService::notifyAnalysisComplete(int userId, int logId, int anomaliesCount)
{
    WebSocketEvent event;
    event.type = WebSocketEventType::AnalysisComplete;
    event.title = "Анализ завершен";
    event.message = "Найдено аномалий: " + std::to_string(anomaliesCount);
    event.data = {
        {"log_id", logId},
        {"anomalies_count", anomaliesCount}};

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    event.timestamp = ss.str();

    sendEvent(userId, event);
}

void WebSocketService::broadcastEvent(const WebSocketEvent &event)
{
    std::lock_guard<std::mutex> lock(connectionsMutex);

    auto jsonMessage = eventToJson(event).dump();

    for (auto &[userId, connections] : clientConnections)
    {
        std::vector<drogon::WebSocketConnectionPtr> toRemove;
        for (const auto &conn : connections)
        {
            if (conn->connected())
            {
                conn->send(jsonMessage);
            }
            else
            {
                toRemove.push_back(conn);
            }
        }

        for (const auto &conn : toRemove)
        {
            connections.erase(conn);
        }
    }
}

size_t WebSocketService::getClientCount() const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    size_t count = 0;
    for (const auto &[userId, connections] : clientConnections)
    {
        count += connections.size();
    }
    return count;
}

size_t WebSocketService::getClientCount(int userId) const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    auto it = clientConnections.find(userId);
    if (it == clientConnections.end())
    {
        return 0;
    }
    return it->second.size();
}

std::string WebSocketService::eventToString(WebSocketEventType type) const
{
    switch (type)
    {
    case WebSocketEventType::AnomalyDetected:
        return "ANOMALY_DETECTED";
    case WebSocketEventType::LogUploaded:
        return "LOG_UPLOADED";
    case WebSocketEventType::AnalysisComplete:
        return "ANALYSIS_COMPLETE";
    case WebSocketEventType::RealTimeLog:
        return "REALTIME_LOG";
    case WebSocketEventType::SystemNotification:
        return "SYSTEM_NOTIFICATION";
    default:
        return "UNKNOWN";
    }
}

nlohmann::json WebSocketService::eventToJson(const WebSocketEvent &event) const
{
    return nlohmann::json{
        {"type", eventToString(event.type)},
        {"title", event.title},
        {"message", event.message},
        {"data", event.data},
        {"timestamp", event.timestamp}};
}

} // namespace services
} // namespace wtld
