#include <drogon/drogon.h>
#include <drogon/HttpAppFramework.h>
#include <iostream>

// Контроллеры
#include "../include/controllers/AuthController.h"
#include "../include/controllers/LogController.h"
#include "../include/controllers/AnalyticsController.h"
#include "../include/controllers/TwoFAController.h"
#include "../include/controllers/WebSocketController.h"

int main()
{
    try
    {
        drogon::app().loadConfigFile("config.json");

        LOG_INFO << "Starting WTLD Backend Server...";
        LOG_INFO << "Server will listen on: 0.0.0.0:8080";
        LOG_INFO << "API Endpoints:";
        LOG_INFO << "  POST /api/auth/register - Register new user";
        LOG_INFO << "  POST /api/auth/login - Login user";
        LOG_INFO << "  POST /api/auth/logout - Logout user";
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
        LOG_INFO << "  WS   /api/ws?token=... - WebSocket connection";
        LOG_INFO << "  GET  /api/ws/status - WebSocket status";

        drogon::app().run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
