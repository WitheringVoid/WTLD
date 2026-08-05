#include "../../include/controllers/WebSocketController.h"
#include "../../include/services/AuthService.h"
#include "../../include/services/WebSocketService.h"
#include <drogon/drogon.h>

namespace wtld
{
    namespace controllers
    {

        WebSocketController::WebSocketController()
        {
            // Инициализация сервисов - здесь можно использовать DI контейнер или создать экземпляры
            // Для простоты создаем экземпляры напрямую
            try {
                auto &app = drogon::app();
                authService_ = std::make_shared<services::AuthService>(app.getDbClient("default"));
                webSocketService_ = &services::WebSocketService::instance();
            } catch (...) {
                LOG_ERROR << "Failed to initialize WebSocketController services";
            }
        }

        void WebSocketController::handleNewConnection(const HttpRequestPtr &req,
                                                      WebSocketConnectionPtr &wsConnPtr)
        {
            // Получаем токен из query-параметра
            std::string token = req->getParameter("token");

            if (token.empty() || !authService_)
            {
                wsConnPtr->forceClose();
                return;
            }

            // Валидируем токен
            auto userOpt = authService_->validateToken(token);

            if (!userOpt.has_value())
            {
                wsConnPtr->forceClose();
                return;
            }

            // Сохраняем соединение
            if (webSocketService_) {
                webSocketService_->addClient(userOpt.value().id, wsConnPtr);
            }
            LOG_INFO << "WebSocket connected for user " << userOpt.value().id;
        }

        void WebSocketController::handleNewMessage(const WebSocketConnectionPtr &wsConnPtr,
                                                   std::string &message,
                                                   const WebSocketMessageType &type)
        {
            LOG_DEBUG << "WebSocket message: " << message;
            // Здесь можно добавить обработку команд от фронтенда
        }

        void WebSocketController::handleConnectionClosed(const WebSocketConnectionPtr &wsConnPtr)
        {
            if (webSocketService_) {
                webSocketService_->removeClient(0, wsConnPtr);
            }
            LOG_INFO << "WebSocket disconnected";
        }

    } // namespace controllers
} // namespace wtld