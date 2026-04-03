#pragma once
#include <drogon/HttpFilter.h>
#include <memory>
#include "../services/AuthService.h"

namespace wtld
{
    namespace middleware
    {

        class JwtMiddleware : public drogon::HttpFilter<JwtMiddleware>
        {
        public:
            JwtMiddleware();

            void doFilter(
                const drogon::HttpRequestPtr &req,
                drogon::FilterCallback &&fcb,
                drogon::FilterChainCallback &&fccb) override;

        private:
            std::shared_ptr<services::AuthService> authService_;

            std::string extractToken(const drogon::HttpRequestPtr &req);
        };

    } // namespace middleware
} // namespace wtld