#include "../../include/services/LogParserService.h"
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

namespace wtld {
namespace services {

const std::regex LogParserService::timestampRegex_(
    R"((\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}(?:\.\d+)?))"
);
const std::regex LogParserService::levelRegex_(
    R"(\[(ERROR|WARNING|INFO|DEBUG)\])"
);

std::vector<models::LogEntry> LogParserService::parseJsonLogs(const std::string& content) {
    std::vector<models::LogEntry> entries;
    
    try {
        auto json = nlohmann::json::parse(content);
        
        // Поддерживаем два формата: массив логов или объект с полем logs
        if (json.is_array()) {
            for (const auto& item : json) {
                models::LogEntry entry;
                entry.timestamp = item.value("timestamp", "");
                entry.level = item.value("level", "INFO");
                entry.message = item.value("message", "");
                entry.source = item.value("source", "");
                entry.metadata = item.value("metadata", nlohmann::json::object());
                entries.push_back(entry);
            }
        } else if (json.contains("logs")) {
            for (const auto& item : json["logs"]) {
                models::LogEntry entry;
                entry.timestamp = item.value("timestamp", "");
                entry.level = item.value("level", "INFO");
                entry.message = item.value("message", "");
                entry.source = item.value("source", "");
                entry.metadata = item.value("metadata", nlohmann::json::object());
                entries.push_back(entry);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "JSON parse error: " << e.what();
    }
    
    return entries;
}

models::LogEntry LogParserService::parseLine(const std::string& line) {
    models::LogEntry entry;
    entry.metadata = nlohmann::json::object();
    
    std::smatch match;
    std::string remaining = line;
    
    // Извлекаем timestamp
    if (std::regex_search(remaining, match, timestampRegex_)) {
        entry.timestamp = match[1];
        remaining = match.suffix();
    }
    
    // Извлекаем уровень
    if (std::regex_search(remaining, match, levelRegex_)) {
        entry.level = match[1];
        remaining = match.suffix();
    } else {
        entry.level = "INFO";
    }
    
    // Остальное - сообщение
    entry.message = remaining;
    
    // Очищаем пробелы
    entry.message = std::regex_replace(entry.message, std::regex("^\\s+"), "");
    entry.message = std::regex_replace(entry.message, std::regex("\\s+$"), "");
    
    return entry;
}

std::vector<models::LogEntry> LogParserService::parseTextLogs(const std::string& content) {
    std::vector<models::LogEntry> entries;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            auto entry = parseLine(line);
            if (validateLogEntry(entry)) {
                entries.push_back(entry);
            }
        }
    }
    
    return entries;
}

std::vector<models::LogEntry> LogParserService::parseLogs(
    const std::string& content,
    const std::string& fileType) {
    
    if (fileType == "json") {
        return parseJsonLogs(content);
    } else if (fileType == "txt") {
        return parseTextLogs(content);
    }
    
    return {};
}

bool LogParserService::validateLogEntry(const models::LogEntry& entry) {
    // Базовая валидация
    if (entry.message.empty()) return false;
    
    // Проверяем уровень
    static const std::set<std::string> validLevels = {"INFO", "WARNING", "ERROR", "DEBUG"};
    if (validLevels.find(entry.level) == validLevels.end()) return false;
    
    return true;
}

} // namespace services
} // namespace wtld