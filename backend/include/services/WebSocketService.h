#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <set>
#include <drogon/WebSocketConnection.h>
#include <nlohmann/json.hpp>

namespace wtld
{
namespace services
{

// Типы WebSocket событий
enum class WebSocketEventType
{
    AnomalyDetected,
    LogUploaded,
    AnalysisComplete,
    RealTimeLog,
    SystemNotification
};

struct WebSocketEvent
{
    WebSocketEventType type;
    std::string title;
    std::string message;
    nlohmann::json data;
    std::string timestamp;
};

class WebSocketService
{
public:
    static WebSocketService &instance();

    // Подключение клиента
    void addClient(int userId, const drogon::WebSocketConnectionPtr &conn);

    // Отключение клиента
    void removeClient(int userId, const drogon::WebSocketConnectionPtr &conn);

    // Отправка события пользователю
    void sendEvent(int userId, const WebSocketEvent &event);

    // Отправка события о новой аномалии всем подключенным клиентам пользователя
    void notifyAnomaly(int userId, const std::string &title, const std::string &description,
                       const nlohmann::json &data);

    // Отправка события о завершении анализа
    void notifyAnalysisComplete(int userId, int logId, int anomaliesCount);

    // Рассылка всем подключенным клиентам (для админских уведомлений)
    void broadcastEvent(const WebSocketEvent &event);

    // Получение количества подключенных клиентов
    size_t getClientCount() const;
    size_t getClientCount(int userId) const;

private:
    WebSocketService() = default;
    ~WebSocketService() = default;
    WebSocketService(const WebSocketService &) = delete;
    WebSocketService &operator=(const WebSocketService &) = delete;

    std::string eventToString(WebSocketEventType type) const;
    nlohmann::json eventToJson(const WebSocketEvent &event) const;

    // Хранилище подключений: userId -> set подключений
    std::unordered_map<int, std::set<drogon::WebSocketConnectionPtr>> clientConnections;
    mutable std::mutex connectionsMutex;
};

} // namespace services
} // namespace wtld
