#pragma once
#include <drogon/HttpController.h>

namespace wtld
{
    namespace controllers
    {

        class HttpStatusController : public drogon::HttpController<HttpStatusController>
        {
        public:
            METHOD_LIST_BEGIN
            ADD_METHOD_TO(HttpStatusController::getStatus, "/api/ws/status", drogon::Get);
            METHOD_LIST_END

            void getStatus(const drogon::HttpRequestPtr &req,
                           std::function<void(const drogon::HttpResponsePtr &)> &&callback);
        };

    } // namespace controllers
} // namespace wtld
