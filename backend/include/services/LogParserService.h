#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../models/LogEntry.h"

namespace wtld {
namespace services {

class LogParserService {
public:
    // Парсинг JSON логов
    std::vector<models::LogEntry> parseJsonLogs(const std::string& content);
    
    // Парсинг TXT логов (например, стандартный формат: [timestamp] [level] message)
    std::vector<models::LogEntry> parseTextLogs(const std::string& content);
    
    // Автоматическое определение формата и парсинг
    std::vector<models::LogEntry> parseLogs(const std::string& content, const std::string& fileType);
    
    // Валидация и очистка данных
    bool validateLogEntry(const models::LogEntry& entry);

private:
    // Парсинг стандартного формата: 2024-01-15 10:30:45 [ERROR] Database connection failed
    models::LogEntry parseLine(const std::string& line);
    
    // Регулярные выражения для различных форматов
    static const std::regex timestampRegex_;
    static const std::regex levelRegex_;
};

} // namespace services
} // namespace wtld