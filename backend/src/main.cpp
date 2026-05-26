#include <drogon/drogon.h>
#include <drogon/HttpAppFramework.h>
#include <iostream>

// Контроллеры
#include "../include/controllers/AuthController.h"
#include "../include/controllers/LogController.h"
#include "../include/controllers/AnalyticsController.h"
#include "../include/controllers/TwoFAController.h"
#include "../include/controllers/WebSocketController.h"

// Middleware
#include "../include/middleware/JwtMiddleware.h"
#include "../include/middleware/RateLimitMiddleware.h"

// WebSocket Service
#include "../include/services/WebSocketService.h"

// WebSocket handler
#include <drogon/WebSocketClient.h>

int main()
{
    try
    {
        drogon::app().loadConfigFile("config.json");

        // Регистрация глобальных фильтров (middleware)
        // Rate Limiting применяется ко ВСЕМ запросам
        drogon::app().registerFilter("/api/*", wtld::middleware::RateLimitMiddleware::newFilter());

        // JWT middleware для защищённых endpoint-ов
        // Применяем ко всем API кроме auth/login и auth/register
        drogon::app().registerFilter("/api/logs/*", wtld::middleware::JwtMiddleware::newFilter());
        drogon::app().registerFilter("/api/analytics/*", wtld::middleware::JwtMiddleware::newFilter());
        drogon::app().registerFilter("/api/2fa/*", wtld::middleware::JwtMiddleware::newFilter());
        drogon::app().registerFilter("/api/auth/profile", wtld::middleware::JwtMiddleware::newFilter());
        drogon::app().registerFilter("/api/auth/logout", wtld::middleware::JwtMiddleware::newFilter());
        drogon::app().registerFilter("/api/ws/status", wtld::middleware::JwtMiddleware::newFilter());

        // Регистрация WebSocket обработчика
        drogon::app().registerWebSocketHandler(
            "/api/ws",
            [](const drogon::WebSocketConnectionPtr &conn)
            {
                // При подключении извлекаем userId из query-параметра token
                auto req = conn->request();
                if (!req)
                {
                    conn->close();
                    return;
                }

                auto token = req->getParameter("token");
                if (token.empty())
                {
                    conn->send(R"({"type":"error","message":"Missing token"})");
                    conn->close();
                    return;
                }

                // Валидируем токен
                auto dbClient = drogon::app().getDbClient();
                auto authService = std::make_shared<wtld::services::AuthService>(dbClient);
                auto user = authService->validateToken(token);

                if (!user)
                {
                    conn->send(R"({"type":"error","message":"Invalid token"})");
                    conn->close();
                    return;
                }

                // Регистрируем клиента
                wtld::services::WebSocketService::instance().addClient(user->id, conn);

                LOG_INFO << "WebSocket client connected for user " << user->id;
            },
            [](const drogon::WebSocketConnectionPtr &conn, const std::string &message)
            {
                // Обработка входящих сообщений от клиента
                LOG_DEBUG << "WebSocket message from user: " << message;
            },
            [](const drogon::WebSocketConnectionPtr &conn)
            {
                // При отключении удаляем клиента
                auto req = conn->request();
                if (req)
                {
                    auto token = req->getParameter("token");
                    if (!token.empty())
                    {
                        auto dbClient = drogon::app().getDbClient();
                        auto authService = std::make_shared<wtld::services::AuthService>(dbClient);
                        auto user = authService->validateToken(token);
                        if (user)
                        {
                            wtld::services::WebSocketService::instance().removeClient(user->id, conn);
                            LOG_INFO << "WebSocket client disconnected for user " << user->id;
                        }
                    }
                }
            });

        LOG_INFO << "Starting WTLD Backend Server...";
        LOG_INFO << "Server will listen on: 0.0.0.0:8080";
        LOG_INFO << "API Endpoints:";
        LOG_INFO << "  POST /api/auth/register - Register new user";
        LOG_INFO << "  POST /api/auth/login - Login user";
        LOG_INFO << "  POST /api/auth/logout - Logout user (JWT)";
        LOG_INFO << "  GET  /api/auth/profile - Get user profile (JWT)";
        LOG_INFO << "  POST /api/logs/upload - Upload log file (JWT)";
        LOG_INFO << "  GET  /api/logs - Get user logs (JWT)";
        LOG_INFO << "  GET  /api/logs/{id} - Get log by ID (JWT)";
        LOG_INFO << "  DELETE /api/logs/{id} - Delete log (JWT)";
        LOG_INFO << "  GET  /api/logs/{id}/stats - Get log statistics (JWT)";
        LOG_INFO << "  GET  /api/analytics/dashboard - Get dashboard data (JWT)";
        LOG_INFO << "  GET  /api/analytics/{logId} - Get log analytics (JWT)";
        LOG_INFO << "  GET  /api/analytics/{logId}/anomalies - Get anomalies (JWT)";
        LOG_INFO << "  GET  /api/analytics/rules - Get analysis rules (JWT)";
        LOG_INFO << "  POST /api/analytics/rules - Create analysis rule (JWT)";
        LOG_INFO << "  DELETE /api/analytics/rules/{id} - Delete rule (JWT)";
        LOG_INFO << "  POST /api/2fa/setup - Setup 2FA (JWT)";
        LOG_INFO << "  POST /api/2fa/verify - Verify 2FA code (JWT)";
        LOG_INFO << "  POST /api/2fa/enable - Enable 2FA (JWT)";
        LOG_INFO << "  POST /api/2fa/disable - Disable 2FA (JWT)";
        LOG_INFO << "  GET  /api/2fa/status - Get 2FA status (JWT)";
        LOG_INFO << "  GET  /api/2fa/backup-codes - Get backup codes (JWT)";
        LOG_INFO << "  WS   /api/ws?token=... - WebSocket connection (JWT)";
        LOG_INFO << "  GET  /api/ws/status - WebSocket status (JWT)";
        LOG_INFO << "";
        LOG_INFO << "Active middleware: RateLimitMiddleware, JwtMiddleware";
        LOG_INFO << "WebSocket handler registered";

        drogon::app().run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
