#include "../../include/controllers/WebSocketController.h"
#include "../../include/services/WebSocketService.h"
#include "../../include/services/AuthService.h"
#include <nlohmann/json.hpp>

namespace wtld
{
namespace controllers
{

void WebSocketController::handleWebSocket(const drogon::HttpRequestPtr &req,
                                          const drogon::WebSocketConnectionPtr &conn)
{
    // Извлечение токена из query параметра или заголовка
    std::string token = req->getParameter("token");

    if (token.empty())
    {
        auto authHeader = req->getHeader("Authorization");
        if (!authHeader.empty() && authHeader.substr(0, 7) == "Bearer ")
        {
            token = authHeader.substr(7);
        }
    }

    if (token.empty())
    {
        conn->forceClose();
        return;
    }

    // Валидация токена
    auto dbClient = drogon::app().getDbClient();
    auto authService = std::make_shared<services::AuthService>(dbClient);
    auto user = authService->validateToken(token);

    if (!user)
    {
        conn->forceClose();
        return;
    }

    // Добавление клиента в WebSocket сервис
    auto &wsService = services::WebSocketService::instance();
    wsService.addClient(user->id, conn);

    // Обработчик сообщений от клиента
    conn->setRecvMessageCallback(
        [userId = user->id](const drogon::WebSocketConnectionPtr &conn,
                            std::string &&message,
                            const drogon::WebSocketMessageType &type)
        {
            if (type == drogon::WebSocketMessageType::Ping)
            {
                conn->send(std::move(message), drogon::WebSocketMessageType::Pong);
            }
            else if (type == drogon::WebSocketMessageType::Text)
            {
                // Обработка входящих сообщений (например, подписка на события)
                try
                {
                    auto json = nlohmann::json::parse(message);
                    std::string action = json.value("action", "");

                    if (action == "subscribe")
                    {
                        // Подписка на определенные события
                        std::string eventType = json.value("event_type", "");
                        LOG_INFO << "User " << userId << " subscribed to " << eventType;
                    }
                    else if (action == "ping")
                    {
                        // Ответ на ping
                        nlohmann::json response;
                        response["type"] = "pong";
                        response["timestamp"] = std::to_string(std::time(nullptr));
                        conn->send(response.dump());
                    }
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR << "WebSocket message parse error: " << e.what();
                }
            }
        });

    // Обработчик отключения
    conn->setCloseCallback(
        [userId = user->id](const drogon::WebSocketConnectionPtr &conn)
        {
            auto &wsService = services::WebSocketService::instance();
            wsService.removeClient(userId, conn);
        });

    // Отправка приветственного сообщения
    nlohmann::json welcome;
    welcome["type"] = "connected";
    welcome["message"] = "WebSocket connected";
    welcome["user_id"] = user->id;
    conn->send(welcome.dump());
}

} // namespace controllers
} // namespace wtld
