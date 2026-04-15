#pragma once
#include <drogon/HttpRequest.h>
#include <drogon/orm/DbClient.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace wtld
{
    namespace utils
    {

        // Извлечение userId из запроса: сначала из атрибутов (если middleware уже установил),
        // затем из JWT токена в заголовке Authorization
        int getUserIdFromRequest(const drogon::HttpRequestPtr &req,
                                 const drogon::orm::DbClientPtr &dbClient);

        // Парсинг PostgreSQL array формата {a,b,"c,d"} в vector<string>
        std::vector<std::string> parsePgArray(const std::string &raw);

    } // namespace utils
} // namespace wtld
