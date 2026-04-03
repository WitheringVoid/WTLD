// WebSocket в Drogon 1.9 требует отдельного подхода через app().createWebSocketClient
// Этот файл-заглушка — реальная обработка WebSocket будет в main через отдельный route
#pragma once
#include <drogon/HttpController.h>
#include <nlohmann/json.hpp>
#include "../services/WebSocketService.h"

namespace wtld
{
    namespace controllers
    {

        // WebSocket-эндпоинты в Drogon 1.9 не работают через HttpController с WebSocketConnectionPtr
        // Вместо этого используем обычный HTTP-эндпоинт для проверки статуса
        class WebSocketController : public drogon::HttpController<WebSocketController>
        {
        public:
            METHOD_LIST_BEGIN
            ADD_METHOD_TO(WebSocketController::getStatus, "/api/ws/status", drogon::Get);
            METHOD_LIST_END

            void getStatus(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback)
            {
                nlohmann::json result;
                result["status"] = "WebSocket support requires Drogon WebSocket API";
                result["connected_clients"] = services::WebSocketService::instance().getClientCount();

                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k200OK);
                resp->setContentTypeString("application/json");
                resp->setBody(result.dump());
                callback(resp);
            }
        };

    } // namespace controllers
} // namespace wtld
