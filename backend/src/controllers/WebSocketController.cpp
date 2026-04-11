#include "../../include/controllers/WebSocketController.h"
#include "../../include/services/WebSocketService.h"
#include <drogon/HttpResponse.h>
#include <nlohmann/json.hpp>

namespace wtld
{
    namespace controllers
    {

        void WebSocketController::getStatus(
            const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback)
        {
            (void)req;
            nlohmann::json result;
            result["status"] = "ok";
            result["connected_clients"] = services::WebSocketService::instance().getClientCount();

            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setContentTypeString("application/json");
            resp->setBody(result.dump());
            callback(resp);
        }

    } // namespace controllers
} // namespace wtld
