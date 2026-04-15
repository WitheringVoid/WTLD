#include "../../include/utils/AuthUtils.h"
#include "../../include/services/AuthService.h"
#include <sstream>

namespace wtld
{
    namespace utils
    {

        int getUserIdFromRequest(const drogon::HttpRequestPtr &req,
                                 const drogon::orm::DbClientPtr &dbClient)
        {
            // Сначала пробуем из атрибутов (если JwtMiddleware уже установил)
            try
            {
                return req->attributes()->get<int>("userId");
            }
            catch (...)
            {
                // Не найден — извлекаем из токена
            }

            auto authHeader = req->getHeader("Authorization");
            if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
            {
                return -1;
            }

            auto token = authHeader.substr(7);
            auto authService = std::make_shared<services::AuthService>(dbClient);
            auto user = authService->validateToken(token);

            if (user)
            {
                req->attributes()->insert("userId", user->id);
                return user->id;
            }

            return -1;
        }

        std::vector<std::string> parsePgArray(const std::string &raw)
        {
            std::vector<std::string> result;
            if (raw.size() <= 2)
                return result;
            std::string inner = raw.substr(1, raw.size() - 2);
            std::stringstream ss(inner);
            std::string item;
            while (std::getline(ss, item, ','))
            {
                if (item.size() >= 2 && item.front() == '"' && item.back() == '"')
                    item = item.substr(1, item.size() - 2);
                if (!item.empty())
                    result.push_back(item);
            }
            return result;
        }

    } // namespace utils
} // namespace wtld
