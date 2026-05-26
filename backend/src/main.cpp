#include <drogon/drogon.h>
#include <drogon/HttpAppFramework.h>
#include <iostream>

// Контроллеры
#include "../include/controllers/AuthController.h"
#include "../include/controllers/LogController.h"
#include "../include/controllers/AnalyticsController.h"
#include "../include/controllers/TwoFAController.h"
#include "../include/controllers/HttpStatusController.h"

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
        drogon::app().registerFilter(std::make_shared<wtld::middleware::RateLimitMiddleware>(), {"api/*"});

        // JWT middleware для защищённых endpoint-ов
        // Применяем ко всем API кроме auth/login и auth/register
        drogon::app().registerFilter(std::make_shared<wtld::middleware::JwtMiddleware>(), {"/api/logs/*"});
        drogon::app().registerFilter(std::make_shared<wtld::middleware::JwtMiddleware>(), {"/api/analytics/*"});
        drogon::app().registerFilter(std::make_shared<wtld::middleware::JwtMiddleware>(), {"/api/2fa/*"});
        drogon::app().registerFilter(std::make_shared<wtld::middleware::JwtMiddleware>(), {"/api/auth/profile"});
        drogon::app().registerFilter(std::make_shared<wtld::middleware::JwtMiddleware>(), {"/api/auth/logout"});
        drogon::app().registerFilter(std::make_shared<wtld::middleware::JwtMiddleware>(), {"/api/ws/status"});

        // Регистрация HTTP-контроллеров
        drogon::app().registerController<wtld::controllers::AuthController>();
        drogon::app().registerController<wtld::controllers::LogController>();
        drogon::app().registerController<wtld::controllers::AnalyticsController>();
        drogon::app().registerController<wtld::controllers::TwoFAController>();
        drogon::app().registerController<wtld::controllers::HttpStatusController>();
        // Регистрация настоящего WebSocket-контроллера (Drogon 1.9.x API)
        drogon::app().regWebsockCtrl<wtld::controllers::WebSocketController>();

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
