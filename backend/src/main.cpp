#include <drogon/drogon.h>
#include <iostream>

// Контроллеры
#include "../include/controllers/AuthController.h"
#include "../include/controllers/LogController.h"
#include "../include/controllers/AnalyticsController.h"
#include "../include/controllers/TwoFAController.h"
// #include "../include/controllers/HttpStatusController.h"
#include "../include/controllers/WebSocketController.h"

// Middleware
#include "../include/middleware/JwtMiddleware.h"
#include "../include/middleware/RateLimitMiddleware.h"
/*
// WebSocket Service
#include "../include/services/WebSocketService.h"
*/
// WebSocket handler
#include <drogon/WebSocketClient.h>

int main()
{
    try
    {
        drogon::app().loadConfigFile("config.json");

        // Подключение PostgreSQL (значения как в database/docker-compose.yml)
        drogon::app().createDbClient("postgres",
                                     "localhost",
                                     5432,
                                     "wtld_db",
                                     "wtld_user",
                                     "wtld_password",
                                     5);

        LOG_INFO << "Starting WTLD Backend Server on 0.0.0.0:8080";

        drogon::app().run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
