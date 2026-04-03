#include "../../include/services/LogAnalysisService.h"
#include <regex>
#include <chrono>
#include <algorithm>
#include <numeric>

namespace wtld
{
    namespace services
    {

        LogAnalysisService::LogAnalysisService(const drogon::orm::DbClientPtr &dbClient)
            : dbClient_(dbClient) {}

        std::vector<AnalysisResult> LogAnalysisService::analyzeLogs(int logFileId, int userId, const std::vector<models::LogEntry> &entries)
        {
            std::vector<AnalysisResult> results;

            if (entries.empty())
            {
                return results;
            }

            // 1. Анализ уровня ошибок
            auto errorRateResult = analyzeErrorRate(entries);
            if (errorRateResult.severity != "low")
            {
                results.push_back(errorRateResult);
                saveAnalysisResult(logFileId, userId, errorRateResult);
            }

            // 2. Обнаружение всплесков ошибок
            auto errorBurstResult = analyzeErrorBurst(entries);
            if (errorBurstResult.isAnomaly)
            {
                results.push_back(errorBurstResult);
                saveAnalysisResult(logFileId, userId, errorBurstResult);
            }

            // 3. Поиск необычных паттернов
            auto unusualPatternsResult = analyzeUnusualPatterns(entries);
            if (unusualPatternsResult.isAnomaly)
            {
                results.push_back(unusualPatternsResult);
                saveAnalysisResult(logFileId, userId, unusualPatternsResult);
            }

            // 4. Поиск по пользовательским правилам
            auto patternResults = findPatterns(entries, userId);
            for (const auto &result : patternResults)
            {
                results.push_back(result);
                saveAnalysisResult(logFileId, userId, result);
            }

            return results;
        }

        std::vector<AnalysisResult> LogAnalysisService::detectAnomalies(const std::vector<models::LogEntry> &entries)
        {
            std::vector<AnalysisResult> anomalies;

            // Анализ временных рядов для обнаружения аномалий
            std::map<std::string, int> errorsPerMinute;
            std::map<std::string, int> totalPerMinute;

            for (const auto &entry : entries)
            {
                // Извлечение минуты из timestamp
                std::string minute = entry.timestamp.substr(0, 16); // YYYY-MM-DD HH:MM
                totalPerMinute[minute]++;

                if (entry.level == "ERROR" || entry.level == "CRITICAL" || entry.level == "FATAL")
                {
                    errorsPerMinute[minute]++;
                }
            }

            // Расчет статистики
            if (!errorsPerMinute.empty())
            {
                std::vector<int> errorCounts;
                for (const auto &[minute, count] : errorsPerMinute)
                {
                    errorCounts.push_back(count);
                }

                double sum = std::accumulate(errorCounts.begin(), errorCounts.end(), 0.0);
                double mean = sum / errorCounts.size();

                double sqSum = 0.0;
                for (int count : errorCounts)
                {
                    sqSum += (count - mean) * (count - mean);
                }
                double stddev = std::sqrt(sqSum / errorCounts.size());

                // Поиск аномалий (более 2 стандартных отклонений)
                for (const auto &[minute, count] : errorsPerMinute)
                {
                    if (mean > 0 && stddev > 0)
                    {
                        double zScore = (count - mean) / stddev;
                        if (zScore > 2.0)
                        {
                            AnalysisResult result;
                            result.type = "anomaly";
                            result.severity = zScore > 3.0 ? "critical" : "high";
                            result.title = "Аномальный всплеск ошибок";
                            result.description = "Обнаружено необычно высокое количество ошибок в " + minute;
                            result.data = {
                                {"minute", minute},
                                {"error_count", count},
                                {"average", mean},
                                {"z_score", zScore}};
                            result.isAnomaly = true;
                            anomalies.push_back(result);
                        }
                    }
                }
            }

            return anomalies;
        }

        std::vector<AnalysisResult> LogAnalysisService::findPatterns(const std::vector<models::LogEntry> &entries, int userId)
        {
            std::vector<AnalysisResult> results;
            auto rules = loadUserRules(userId);

            for (const auto &rule : rules)
            {
                if (!rule.isActive)
                    continue;

                std::vector<std::string> matchedLogs;
                int matchCount = 0;

                for (const auto &entry : entries)
                {
                    if (matchesPattern(entry.message, rule.pattern) ||
                        matchesPattern(entry.source, rule.pattern))
                    {
                        matchCount++;
                        if (matchedLogs.size() < 5) // Сохраняем первые 5 совпадений
                        {
                            matchedLogs.push_back(entry.message);
                        }
                    }
                }

                if (matchCount > 0)
                {
                    AnalysisResult result;
                    result.type = "pattern_match";
                    result.severity = rule.severity;
                    result.title = "Совпадение с правилом: " + rule.name;
                    result.description = "Найдено совпадений: " + std::to_string(matchCount);
                    result.data = {
                        {"rule_id", rule.id},
                        {"rule_name", rule.name},
                        {"match_count", matchCount},
                        {"samples", matchedLogs}};
                    result.detectedPatterns.push_back(rule.pattern);
                    result.isAnomaly = (rule.severity == "high" || rule.severity == "critical");
                    results.push_back(result);
                }
            }

            return results;
        }

        nlohmann::json LogAnalysisService::calculateStatistics(const std::vector<models::LogEntry> &entries)
        {
            nlohmann::json stats;

            int total = entries.size();
            std::map<std::string, int> levelCounts;
            std::map<std::string, int> sourceCounts;
            std::map<std::string, int> hourlyCounts;

            for (const auto &entry : entries)
            {
                levelCounts[entry.level]++;
                if (!entry.source.empty())
                {
                    sourceCounts[entry.source]++;
                }

                // Извлечение часа из timestamp
                if (entry.timestamp.length() >= 13)
                {
                    std::string hour = entry.timestamp.substr(0, 13); // YYYY-MM-DD HH
                    hourlyCounts[hour]++;
                }
            }

            stats["total"] = total;
            stats["by_level"] = levelCounts;
            stats["by_source"] = sourceCounts;
            stats["hourly_distribution"] = hourlyCounts;

            // Расчет процента ошибок
            auto countOrZero = [&levelCounts](const std::string &key) -> int
            {
                auto it = levelCounts.find(key);
                return (it != levelCounts.end()) ? it->second : 0;
            };
            int errorCount = countOrZero("ERROR") + countOrZero("CRITICAL") + countOrZero("FATAL");
            stats["error_rate"] = total > 0 ? (double)errorCount / total * 100.0 : 0.0;

            return stats;
        }

        bool LogAnalysisService::saveAnalysisResult(int logFileId, int userId, const AnalysisResult &result)
        {
            try
            {
                nlohmann::json patternsJson = result.detectedPatterns;
                dbClient_->execSqlSync(
                    "INSERT INTO log_analytics (log_file_id, user_id, analysis_type, severity_level, "
                    "title, description, data, detected_patterns, is_anomaly) "
                    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)",
                    logFileId, userId, result.type, result.severity,
                    result.title, result.description, result.data.dump(),
                    patternsJson.dump(), result.isAnomaly);

                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Save analysis result error: " << e.what();
                return false;
            }
        }

        std::vector<AnalysisRule> LogAnalysisService::loadUserRules(int userId)
        {
            std::vector<AnalysisRule> rules;

            try
            {
                auto result = dbClient_->execSqlSync(
                    "SELECT id, user_id, rule_name, rule_type, pattern, severity, is_active "
                    "FROM analysis_rules WHERE user_id = $1 ORDER BY id",
                    userId);

                for (const auto &row : result)
                {
                    AnalysisRule rule;
                    rule.id = row["id"].as<int>();
                    rule.userId = row["user_id"].as<int>();
                    rule.name = row["rule_name"].as<std::string>();
                    rule.type = row["rule_type"].as<std::string>();
                    rule.pattern = row["pattern"].as<std::string>();
                    rule.severity = row["severity"].as<std::string>();
                    rule.isActive = row["is_active"].as<bool>();
                    rules.push_back(rule);
                }
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Load user rules error: " << e.what();
            }

            return rules;
        }

        int LogAnalysisService::createRule(int userId, const std::string &name, const std::string &type,
                                           const std::string &pattern, const std::string &severity)
        {
            try
            {
                auto result = dbClient_->execSqlSync(
                    "INSERT INTO analysis_rules (user_id, rule_name, rule_type, pattern, severity) "
                    "VALUES ($1, $2, $3, $4, $5) RETURNING id",
                    userId, name, type, pattern, severity);

                return result[0]["id"].as<int>();
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Create rule error: " << e.what();
                return -1;
            }
        }

        bool LogAnalysisService::deleteRule(int ruleId)
        {
            try
            {
                dbClient_->execSqlSync("DELETE FROM analysis_rules WHERE id = $1", ruleId);
                return true;
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << "Delete rule error: " << e.what();
                return false;
            }
        }

        AnalysisResult LogAnalysisService::analyzeRealTimeLog(const models::LogEntry &entry, int userId)
        {
            AnalysisResult result;
            result.type = "realtime";
            result.isAnomaly = false;
            result.severity = "low";

            // Проверка на критические уровни
            if (entry.level == "CRITICAL" || entry.level == "FATAL")
            {
                result.isAnomaly = true;
                result.severity = "critical";
                result.title = "Критическая ошибка в реальном времени";
                result.description = entry.message;
                result.data = {
                    {"timestamp", entry.timestamp},
                    {"level", entry.level},
                    {"source", entry.source}};
                return result;
            }

            // Проверка по правилам пользователя
            auto rules = loadUserRules(userId);
            for (const auto &rule : rules)
            {
                if (rule.isActive && matchesPattern(entry.message, rule.pattern))
                {
                    result.isAnomaly = true;
                    result.severity = rule.severity;
                    result.title = "Совпадение правила: " + rule.name;
                    result.description = entry.message;
                    result.data = {
                        {"rule_id", rule.id},
                        {"rule_name", rule.name}};
                    result.detectedPatterns.push_back(rule.pattern);
                    return result;
                }
            }

            return result;
        }

        AnalysisResult LogAnalysisService::analyzeErrorRate(const std::vector<models::LogEntry> &entries)
        {
            AnalysisResult result;
            result.type = "error_rate";
            result.isAnomaly = false;

            int total = entries.size();
            int errorCount = 0;

            for (const auto &entry : entries)
            {
                if (entry.level == "ERROR" || entry.level == "CRITICAL" || entry.level == "FATAL")
                {
                    errorCount++;
                }
            }

            double errorRate = total > 0 ? (double)errorCount / total * 100.0 : 0.0;
            result.data = {
                {"total_logs", total},
                {"error_count", errorCount},
                {"error_rate_percent", errorRate}};

            if (errorRate > 50.0)
            {
                result.severity = "critical";
                result.title = "Критический уровень ошибок";
                result.description = "Более 50% логов являются ошибками";
                result.isAnomaly = true;
            }
            else if (errorRate > 25.0)
            {
                result.severity = "high";
                result.title = "Высокий уровень ошибок";
                result.description = "Более 25% логов являются ошибками";
                result.isAnomaly = true;
            }
            else if (errorRate > 10.0)
            {
                result.severity = "medium";
                result.title = "Повышенный уровень ошибок";
                result.description = "Более 10% логов являются ошибками";
                result.isAnomaly = true;
            }
            else
            {
                result.severity = "low";
                result.title = "Нормальный уровень ошибок";
                result.description = "Уровень ошибок в пределах нормы";
            }

            return result;
        }

        AnalysisResult LogAnalysisService::analyzeErrorBurst(const std::vector<models::LogEntry> &entries)
        {
            AnalysisResult result;
            result.type = "error_burst";
            result.isAnomaly = false;
            result.severity = "low";

            // Группировка ошибок по минутам
            std::map<std::string, int> errorsPerMinute;
            for (const auto &entry : entries)
            {
                if (entry.level == "ERROR" || entry.level == "CRITICAL" || entry.level == "FATAL")
                {
                    std::string minute = entry.timestamp.length() >= 16 ? entry.timestamp.substr(0, 16) : entry.timestamp;
                    errorsPerMinute[minute]++;
                }
            }

            // Поиск всплесков (более 10 ошибок в минуту)
            int maxErrors = 0;
            std::string maxMinute;
            for (const auto &[minute, count] : errorsPerMinute)
            {
                if (count > maxErrors)
                {
                    maxErrors = count;
                    maxMinute = minute;
                }
            }

            if (maxErrors > 50)
            {
                result.isAnomaly = true;
                result.severity = "critical";
                result.title = "Массовый всплеск ошибок";
                result.description = "Обнаружено " + std::to_string(maxErrors) + " ошибок в минуту (" + maxMinute + ")";
                result.data = {
                    {"max_errors_per_minute", maxErrors},
                    {"timestamp", maxMinute}};
            }
            else if (maxErrors > 20)
            {
                result.isAnomaly = true;
                result.severity = "high";
                result.title = "Всплеск ошибок";
                result.description = "Обнаружено " + std::to_string(maxErrors) + " ошибок в минуту";
                result.data = {
                    {"max_errors_per_minute", maxErrors},
                    {"timestamp", maxMinute}};
            }

            return result;
        }

        AnalysisResult LogAnalysisService::analyzeUnusualPatterns(const std::vector<models::LogEntry> &entries)
        {
            AnalysisResult result;
            result.type = "unusual_pattern";
            result.isAnomaly = false;
            result.severity = "low";

            // Поиск необычных паттернов в сообщениях
            static const std::vector<std::pair<const char *, const char *>> criticalPatterns = {
                {R"(out\s+of\s+memory)", "OutOfMemory"},
                {R"(disk\s+full)", "DiskFull"},
                {R"(connection\s+(refused|timeout|lost))", "ConnectionIssue"},
                {R"(authentication\s+failed)", "AuthFailed"},
                {R"(permission\s+denied)", "PermissionDenied"},
                {R"(segmentation\s+fault)", "SegFault"},
            };

            std::map<std::string, int> patternMatches;
            for (const auto &entry : entries)
            {
                for (const auto &[patternStr, name] : criticalPatterns)
                {
                    std::regex pattern(patternStr, std::regex_constants::icase);
                    if (std::regex_search(entry.message, pattern))
                    {
                        patternMatches[name]++;
                    }
                }
            }

            if (!patternMatches.empty())
            {
                result.isAnomaly = true;
                result.severity = "high";
                result.title = "Обнаружены критические паттерны";

                nlohmann::json patternsJson;
                for (const auto &[name, count] : patternMatches)
                {
                    patternsJson[name] = count;
                    result.detectedPatterns.push_back(name);
                }

                result.description = "Найдены критические паттерны ошибок";
                result.data = {{"patterns", patternsJson}};
            }

            return result;
        }

        bool LogAnalysisService::matchesPattern(const std::string &text, const std::string &pattern)
        {
            try
            {
                std::regex regex(pattern, std::regex_constants::icase);
                return std::regex_search(text, regex);
            }
            catch (const std::exception &)
            {
                // Если паттерн не валидный regex, пробуем простое совпадение
                return text.find(pattern) != std::string::npos;
            }
        }

        LogAnalysisService::BaselineMetrics LogAnalysisService::calculateBaseline(const std::vector<models::LogEntry> &entries)
        {
            BaselineMetrics metrics;
            metrics.avgErrorsPerMinute = 0.0;
            metrics.avgWarningsPerMinute = 0.0;
            metrics.errorToInfoRatio = 0.0;

            std::map<std::string, int> errorsPerMinute;
            std::map<std::string, int> warningsPerMinute;
            int infoCount = 0;
            int errorCount = 0;

            for (const auto &entry : entries)
            {
                std::string minute = entry.timestamp.length() >= 16 ? entry.timestamp.substr(0, 16) : entry.timestamp;

                if (entry.level == "ERROR" || entry.level == "CRITICAL" || entry.level == "FATAL")
                {
                    errorsPerMinute[minute]++;
                    errorCount++;
                }
                else if (entry.level == "WARNING" || entry.level == "WARN")
                {
                    warningsPerMinute[minute]++;
                }
                else if (entry.level == "INFO")
                {
                    infoCount++;
                }
            }

            if (!errorsPerMinute.empty())
            {
                double sum = 0.0;
                for (const auto &[minute, count] : errorsPerMinute)
                {
                    sum += count;
                }
                metrics.avgErrorsPerMinute = sum / errorsPerMinute.size();
            }

            if (!warningsPerMinute.empty())
            {
                double sum = 0.0;
                for (const auto &[minute, count] : warningsPerMinute)
                {
                    sum += count;
                }
                metrics.avgWarningsPerMinute = sum / warningsPerMinute.size();
            }

            if (infoCount > 0)
            {
                metrics.errorToInfoRatio = (double)errorCount / infoCount;
            }

            return metrics;
        }

    } // namespace services
} // namespace wtld
