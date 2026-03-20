#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace wtld {
namespace models {

struct LogEntry {
    std::string timestamp;
    std::string level;      // INFO, WARNING, ERROR, DEBUG
    std::string message;
    std::string source;     // Исходный файл/компонент
    nlohmann::json metadata; // Дополнительные поля

    nlohmann::json toJson() const {
        return nlohmann::json{
            {"timestamp", timestamp},
            {"level", level},
            {"message", message},
            {"source", source},
            {"metadata", metadata}
        };
    }
};

struct LogFile {
    int id;
    int user_id;
    std::string filename;
    std::vector<LogEntry> entries;
    std::string file_type;
    long file_size;
    std::string status;
    std::time_t upload_date;
};

} // namespace models
} // namespace wtld