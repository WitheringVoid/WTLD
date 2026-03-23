#include "../../include/middleware/JwtMiddleware.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpAppFramework.h>
#include "../services/AuthService.h"

namespace wtld
{
namespace middleware
{

JwtMiddleware::JwtMiddleware()
{
    // AuthService будет создан в doFilter
}

void JwtMiddleware::doFilter(
    const drogon::HttpRequestPtr &req,
    drogon::FilterCallback &&fcb,
    drogon::FilterChainCallback &&fccb)
{
    // Извлечение токена из заголовка Authorization
    std::string token = extractToken(req);

    if (token.empty())
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        resp->setBody("Missing authorization token");
        fcb(resp);
        return;
    }

    // Валидация токена
    auto dbClient = drogon::app().getDbClient();
    auto authService = std::make_shared<services::AuthService>(dbClient);
    auto user = authService->validateToken(token);

    if (!user)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        resp->setBody("Invalid or expired token");
        fcb(resp);
        return;
    }

    // Проверка 2FA (если включен)
    if (authService->verifyTwoFactorCode(user->id, ""))
    {
        // Если 2FA включен, проверяем наличие заголовка X-2FA-Code
        auto twoFACode = req->getHeader("X-2FA-Code");
        if (twoFACode.empty())
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k403Forbidden);
            resp->setBody("2FA code required");
            resp->addHeader("X-2FA-Required", "true");
            fcb(resp);
            return;
        }

        if (!authService->verifyTwoFactorCode(user->id, twoFACode))
        {
            // Пробуем проверить как backup код
            if (!authService->verifyBackupCode(user->id, twoFACode))
            {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k403Forbidden);
                resp->setBody("Invalid 2FA code");
                fcb(resp);
                return;
            }
        }
    }

    // Добавление user_id в запрос
    req->setAttribute("userId", user->id);
    req->setAttribute("user", *user);

    // Продолжение цепочки
    fccb();
}

std::string JwtMiddleware::extractToken(const drogon::HttpRequestPtr &req)
{
    auto authHeader = req->getHeader("Authorization");

    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
    {
        return "";
    }

    return authHeader.substr(7);
}

} // namespace middleware
} // namespace wtld
