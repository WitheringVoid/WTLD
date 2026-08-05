#pragma once
#include <drogon/WebSocketController.h>
#include <memory>
#include "../../services/AuthService.h"
#include "../../services/WebSocketService.h"

namespace wtld
{
    namespace controllers
    {
        class WebSocketController : public drogon::WebSocketController<WebSocketController>
        {
        public:
            WebSocketController();
            
            void handleNewConnection(const HttpRequestPtr &req,
                                     WebSocketConnectionPtr &wsConnPtr) override;

            void handleNewMessage(const WebSocketConnectionPtr &wsConnPtr,
                                  std::string &message,
                                  const WebSocketMessageType &type) override;

            void handleConnectionClosed(const WebSocketConnectionPtr &wsConnPtr) override;

            WS_PATH_LIST_BEGIN
            WS_PATH_ADD("/api/ws");
            WS_PATH_LIST_END

        private:
            std::shared_ptr<services::AuthService> authService_;
            std::shared_ptr<services::WebSocketService> webSocketService_;
        };

    } // namespace controllers
} // namespace wtld