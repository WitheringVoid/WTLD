#include "../../include/controllers/WebSocketController.h"
#include "../../include/services/AuthService.h"
#include "../../include/services/WebSocketService.h"
#include <drogon/drogon.h>

namespace wtld
{
    namespace controllers
    {

        void WebSocketController::handleNewConnection(const HttpRequestPtr &req,
                                                      WebSocketConnectionPtr &&wsConnPtr)
        {
            // Получаем токен из query-параметра
            std::string token = req->getParameter("token");

            if (token.empty())
            {
                wsConnPtr->forceClose();
                return;
            }

            // Валидируем токен
            auto userOpt = services::AuthService::instance().validateToken(token);

            if (!userOpt.has_value())
            {
                wsConnPtr->forceClose();
                return;
            }

            // Сохраняем соединение
            services::WebSocketService::instance().addConnection(wsConnPtr, userOpt.value());
            LOG_INFO << "WebSocket connected for user " << userOpt.value().id;
        }

        void WebSocketController::handleNewMessage(const WebSocketConnectionPtr &wsConnPtr,
                                                   std::string &&message,
                                                   const WebSocketMessageType &type)
        {
            LOG_DEBUG << "WebSocket message: " << message;
            // Здесь можно добавить обработку команд от фронтенда
        }

        void WebSocketController::handleConnectionClosed(const WebSocketConnectionPtr &wsConnPtr)
        {
            services::WebSocketService::instance().removeConnection(wsConnPtr);
            LOG_INFO << "WebSocket disconnected";
        }

    } // namespace controllers
} // namespace wtld