#pragma once
#include <drogon/WebSocketController.h>

namespace wtld
{
    namespace controllers
    {

        class WebSocketController : public drogon::WebSocketController<WebSocketController>
        {
        public:
            void handleNewConnection(const HttpRequestPtr &req,
                                     WebSocketConnectionPtr &&wsConnPtr) override;

            void handleNewMessage(const WebSocketConnectionPtr &wsConnPtr,
                                  std::string &&message,
                                  const WebSocketMessageType &type) override;

            void handleConnectionClosed(const WebSocketConnectionPtr &wsConnPtr) override;

            WS_PATH_LIST_BEGIN
            WS_PATH_ADD("/api/ws"); // Frontend: ws://host:8080/api/ws?token=xxx
            WS_PATH_LIST_END
        };

    } // namespace controllers
} // namespace wtld