#include <drogon/drogon.h>
#include <iostream>

int main() {
    try {
        // Загружаем конфигурацию
        drogon::app().loadConfigFile("../config.json");
        
        // Инициализируем базу данных
        LOG_INFO << "Starting WTLD Backend Server...";
        
        // Запускаем сервер
        drogon::app().run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}