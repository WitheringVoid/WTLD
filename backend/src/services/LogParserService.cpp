#include "../../include/services/LogParserService.h"
#include <regex>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <drogon/utils/Utilities.h>
#include <trantor/utils/Logger.h>

namespace wtld
{
    namespace services
    {

        // Регулярные выражения для парсинга
        const std::regex LogParserService::timestampRegex_(
            R"((\d{4}-\d{2}-\d{2}[T\s]\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:?\d{2})?))");
        const std::regex LogParserService::levelRegex_(
            R"(\b(DEBUG|INFO|WARNING|WARN|ERROR|CRITICAL|FATAL)\b)", std::regex_constants::icase);

        std::vector<models::LogEntry> LogParserService::parseJsonLogs(const std::string &content)
        {
            std::vector<models::LogEntry> entries;

            try
            {
                auto json = nlohmann::json::parse(content);

                // Если это массив логов
                if (json.is_array())
                {
                    for (const auto &item : json)
                    {
                        models::LogEntry entry;
                        entry.timestamp = item.value("timestamp", item.value("time", item.value("date", "")));
                        entry.level = item.value("level", item.value("severity", "INFO"));
                        entry.message = item.value("message", item.value("msg", ""));
                        entry.source = item.value("source", item.value("logger", item.value("component", "")));

                        // Сохраняем остальные поля в metadata
                        for (auto &el : item.items())
                        {
                            if (el.key() != "timestamp" && el.key() != "time" && el.key() != "date" &&
                                el.key() != "level" && el.key() != "severity" &&
                                el.key() != "message" && el.key() != "msg" &&
                                el.key() != "source" && el.key() != "logger" && el.key() != "component")
                            {
                                entry.metadata[el.key()] = el.value();
                            }
                        }

                        if (validateLogEntry(entry))
                        {
                            entries.push_back(entry);
                        }
                    }
                }
                // Если это одиночный лог
                else if (json.is_object())
                {
                    models::LogEntry entry;
                    entry.timestamp = json.value("timestamp", json.value("time", json.value("date", "")));
                    entry.level = json.value("level", json.value("severity", "INFO"));
                    entry.message = json.value("message", json.value("msg", ""));
                    entry.source = json.value("source", json.value("logger", json.value("component", "")));

                    for (auto &el : json.items())
                    {
                        if (el.key() != "timestamp" && el.key() != "time" && el.key() != "date" &&
                            el.key() != "level" && el.key() != "severity" &&
                            el.key() != "message" && el.key() != "msg" &&
                            el.key() != "source" && el.key() != "logger" && el.key() != "component")
                        {
                            entry.metadata[el.key()] = el.value();
                        }
                    }

                    if (validateLogEntry(entry))
                    {
                        entries.push_back(entry);
                    }
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "JSON log parsing error: " << e.what();
            }

            return entries;
        }

        std::vector<models::LogEntry> LogParserService::parseTextLogs(const std::string &content)
        {
            std::vector<models::LogEntry> entries;
            std::istringstream stream(content);
            std::string line;

            while (std::getline(stream, line))
            {
                if (line.empty() || std::all_of(line.begin(), line.end(), ::isspace))
                {
                    continue;
                }

                auto entry = parseLine(line);
                if (validateLogEntry(entry))
                {
                    entries.push_back(entry);
                }
            }

            return entries;
        }

        std::vector<models::LogEntry> LogParserService::parseLogs(const std::string &content, const std::string &fileType)
        {
            if (fileType == "json")
            {
                return parseJsonLogs(content);
            }
            else
            {
                return parseTextLogs(content);
            }
        }

        bool LogParserService::validateLogEntry(const models::LogEntry &entry)
        {
            // Базовая валидация
            if (entry.message.empty())
            {
                return false;
            }

            // Проверка уровня лога
            static const std::vector<std::string> validLevels = {
                "DEBUG", "INFO", "WARNING", "WARN", "ERROR", "CRITICAL", "FATAL"};

            bool validLevel = std::find(validLevels.begin(), validLevels.end(), entry.level) != validLevels.end();
            if (!validLevel)
            {
                return false;
            }

            return true;
        }

        models::LogEntry LogParserService::parseLine(const std::string &line)
        {
            models::LogEntry entry;
            entry.message = line;
            entry.level = "INFO";

            // Попытка извлечь timestamp
            std::smatch timestampMatch;
            if (std::regex_search(line, timestampMatch, timestampRegex_))
            {
                entry.timestamp = timestampMatch[1].str();
            }
            else
            {
                // Текущее время если не найдено
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
                entry.timestamp = ss.str();
            }

            // Попытка извлечь уровень лога
            std::smatch levelMatch;
            if (std::regex_search(line, levelMatch, levelRegex_))
            {
                entry.level = levelMatch[1].str();
                // Нормализация уровня
                if (entry.level == "WARN")
                    entry.level = "WARNING";
                if (entry.level == "FATAL")
                    entry.level = "CRITICAL";
            }

            // Попытка извлечь источник (например, [ComponentName] или logger_name:)
            static const std::regex sourceRegex(R"(\[([^\]]+)\]|(\w+):)");
            std::smatch sourceMatch;
            if (std::regex_search(line, sourceMatch, sourceRegex))
            {
                entry.source = sourceMatch[1].str();
                if (entry.source.empty())
                {
                    entry.source = sourceMatch[2].str();
                }
            }

            return entry;
        }

    } // namespace services
} // namespace wtld
