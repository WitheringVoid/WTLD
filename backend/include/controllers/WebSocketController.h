#pragma once
#include <drogon/HttpController.h>
#include <drogon/WebSocketConnection.h>

namespace wtld
{
namespace controllers
{

class WebSocketController : public drogon::HttpController<WebSocketController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(WebSocketController::handleWebSocket, "/api/ws", drogon::Get);
    METHOD_LIST_END

    void handleWebSocket(const drogon::HttpRequestPtr &req,
                         const drogon::WebSocketConnectionPtr &conn);
};

} // namespace controllers
} // namespace wtld
