#include "../../include/middleware/RateLimitMiddleware.h"
#include <drogon/HttpResponse.h>
#include <trantor/utils/Logger.h>

namespace wtld
{
    namespace middleware
    {

        RateLimitMiddleware::RateLimitMiddleware() {}

        void RateLimitMiddleware::doFilter(
            const drogon::HttpRequestPtr &req,
            drogon::FilterCallback &&fcb,
            drogon::FilterChainCallback &&fccb)
        {
            auto clientId = getClientIdentifier(req);

            std::lock_guard<std::mutex> lock(statesMutex);

            auto now = std::chrono::steady_clock::now();
            auto it = clientStates.find(clientId);

            if (it == clientStates.end())
            {
                // Новый клиент
                clientStates[clientId] = {1, now};
                fccb();
                return;
            }

            auto &info = it->second;

            // Проверка, истекло ли окно
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - info.windowStart);

            if (elapsed >= windowDuration)
            {
                // Сброс окна
                info.requestCount = 1;
                info.windowStart = now;
                fccb();
                return;
            }

            // Проверка лимита
            if (info.requestCount >= maxRequestsPerWindow)
            {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k429TooManyRequests);
                resp->setBody("Rate limit exceeded. Try again later.");
                resp->addHeader("X-RateLimit-Limit", std::to_string(maxRequestsPerWindow));
                resp->addHeader("X-RateLimit-Remaining", "0");
                resp->addHeader("X-RateLimit-Reset", std::to_string(
                                                         std::chrono::duration_cast<std::chrono::seconds>(
                                                             windowDuration - elapsed)
                                                             .count()));
                fcb(resp);
                return;
            }

            // Увеличение счетчика
            info.requestCount++;

            // Добавление заголовков rate limiting
            req->addHeader("X-RateLimit-Limit", std::to_string(maxRequestsPerWindow));
            req->addHeader("X-RateLimit-Remaining", std::to_string(maxRequestsPerWindow - info.requestCount));

            fccb();
        }

        std::string RateLimitMiddleware::getClientIdentifier(const drogon::HttpRequestPtr &req)
        {
            // Идентификация по IP адресу
            auto peerAddr = req->peerAddr();
            auto ip = peerAddr.toIp();
            if (!ip.empty() && ip != "0.0.0.0")
            {
                return peerAddr.toIpPort();
            }

            // Fallback на User-Agent + путь
            return req->getHeader("User-Agent") + req->path();
        }

    } // namespace middleware
} // namespace wtld
