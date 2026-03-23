#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <drogon/orm/DbClient.h>
#include "../models/LogEntry.h"

namespace wtld
{
namespace services
{

// Результат анализа
struct AnalysisResult
{
    std::string type;          // error_rate, anomaly, pattern_match
    std::string severity;      // low, medium, high, critical
    std::string title;
    std::string description;
    nlohmann::json data;
    std::vector<std::string> detectedPatterns;
    bool isAnomaly;
};

// Паттерн для поиска
struct AnalysisRule
{
    int id;
    int userId;
    std::string name;
    std::string type; // regex, keyword, threshold
    std::string pattern;
    std::string severity;
    bool isActive;
};

class LogAnalysisService
{
public:
    explicit LogAnalysisService(const drogon::orm::DbClientPtr &dbClient);

    // Анализ загруженных логов
    std::vector<AnalysisResult> analyzeLogs(int logFileId, int userId, const std::vector<models::LogEntry> &entries);

    // Обнаружение аномалий
    std::vector<AnalysisResult> detectAnomalies(const std::vector<models::LogEntry> &entries);

    // Поиск паттернов по правилам пользователя
    std::vector<AnalysisResult> findPatterns(const std::vector<models::LogEntry> &entries, int userId);

    // Статистика по логам
    nlohmann::json calculateStatistics(const std::vector<models::LogEntry> &entries);

    // Сохранение результатов анализа в БД
    bool saveAnalysisResult(int logFileId, int userId, const AnalysisResult &result);

    // Загрузка правил анализа для пользователя
    std::vector<AnalysisRule> loadUserRules(int userId);

    // Создание правила анализа
    int createRule(int userId, const std::string &name, const std::string &type,
                   const std::string &pattern, const std::string &severity);

    // Удаление правила
    bool deleteRule(int ruleId);

    // Real-time анализ (для потоковой обработки)
    AnalysisResult analyzeRealTimeLog(const models::LogEntry &entry, int userId);

private:
    drogon::orm::DbClientPtr dbClient_;

    // Внутренние методы анализа
    AnalysisResult analyzeErrorRate(const std::vector<models::LogEntry> &entries);
    AnalysisResult analyzeErrorBurst(const std::vector<models::LogEntry> &entries);
    AnalysisResult analyzeUnusualPatterns(const std::vector<models::LogEntry> &entries);

    // Проверка на соответствие regex паттерну
    bool matchesPattern(const std::string &text, const std::string &pattern);

    // Расчет базовых метрик для сравнения
    struct BaselineMetrics
    {
        double avgErrorsPerMinute;
        double avgWarningsPerMinute;
        double errorToInfoRatio;
    };

    BaselineMetrics calculateBaseline(const std::vector<models::LogEntry> &entries);
};

} // namespace services
} // namespace wtld
