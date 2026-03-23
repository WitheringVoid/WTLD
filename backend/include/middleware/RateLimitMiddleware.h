#pragma once
#include <drogon/HttpFilter.h>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace wtld
{
namespace middleware
{

// Информация о клиенте для rate limiting
struct ClientRateLimitInfo
{
    int requestCount;
    std::chrono::steady_clock::time_point windowStart;
};

class RateLimitMiddleware : public drogon::HttpFilter<RateLimitMiddleware>
{
public:
    RateLimitMiddleware();

    void doFilter(
        const drogon::HttpRequestPtr &req,
        drogon::FilterCallback &&fcb,
        drogon::FilterChainCallback &&fccb) override;

private:
    std::string getClientIdentifier(const drogon::HttpRequestPtr &req);

    // Настройки rate limiting
    const int maxRequestsPerWindow = 100;          // Максимум запросов
    const std::chrono::seconds windowDuration{60}; // Окно в секундах

    // Хранилище состояния (в памяти, для production лучше Redis)
    std::unordered_map<std::string, ClientRateLimitInfo> clientStates;
    std::mutex statesMutex;
};

} // namespace middleware
} // namespace wtld
