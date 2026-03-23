#include <drogon/drogon.h>
#include <iostream>
#include <drogon/HttpAppFramework.h>

// Контроллеры
#include "../include/controllers/AuthController.h"
#include "../include/controllers/LogController.h"
#include "../include/controllers/AnalyticsController.h"
#include "../include/controllers/TwoFAController.h"
#include "../include/controllers/WebSocketController.h"

// Middleware
#include "../include/middleware/JwtMiddleware.h"
#include "../include/middleware/RateLimitMiddleware.h"

int main()
{
    try
    {
        // Загружаем конфигурацию
        drogon::app().loadConfigFile("../config.json");

        // Регистрация контроллеров
        drogon::app().registerController<wtld::controllers::AuthController>();
        drogon::app().registerController<wtld::controllers::LogController>();
        drogon::app().registerController<wtld::controllers::AnalyticsController>();
        drogon::app().registerController<wtld::controllers::TwoFAController>();
        drogon::app().registerController<wtld::controllers::WebSocketController>();

        // Регистрация middleware
        drogon::app().registerFilter<wtld::middleware::JwtMiddleware>(
            {"/api/logs/*", "/api/analytics/*", "/api/2fa/*"});

        drogon::app().registerFilter<wtld::middleware::RateLimitMiddleware>(
            {"/*"});

        // Настройка логгирования
        LOG_INFO << "Starting WTLD Backend Server...";
        LOG_INFO << "Server will listen on: 0.0.0.0:8080";
        LOG_INFO << "API Endpoints:";
        LOG_INFO << "  POST /api/auth/register - Register new user";
        LOG_INFO << "  POST /api/auth/login - Login user";
        LOG_INFO << "  POST /api/auth/logout - Logout user";
        LOG_INFO << "  GET  /api/auth/profile - Get user profile";
        LOG_INFO << "  POST /api/logs/upload - Upload log file";
        LOG_INFO << "  GET  /api/logs - Get user logs";
        LOG_INFO << "  GET  /api/logs/{id} - Get log by ID";
        LOG_INFO << "  GET  /api/logs/{id}/stats - Get log statistics";
        LOG_INFO << "  GET  /api/analytics/dashboard - Get dashboard data";
        LOG_INFO << "  GET  /api/analytics/{logId} - Get log analytics";
        LOG_INFO << "  GET  /api/analytics/{logId}/anomalies - Get anomalies";
        LOG_INFO << "  GET  /api/analytics/rules - Get analysis rules";
        LOG_INFO << "  POST /api/analytics/rules - Create analysis rule";
        LOG_INFO << "  POST /api/2fa/setup - Setup 2FA";
        LOG_INFO << "  POST /api/2fa/verify - Verify 2FA code";
        LOG_INFO << "  POST /api/2fa/enable - Enable 2FA";
        LOG_INFO << "  POST /api/2fa/disable - Disable 2FA";

        // Запускаем сервер
        drogon::app().run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}