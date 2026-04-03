# WTLD - Веб-платформа для анализа логов

Платформа для сбора и анализа логов в реальном времени с веб-интерфейсом, поддержкой 2FA и высокопроизводительной обработкой на C++.

## 📋 Содержание

- [Возможности](#возможности)
- [Стек технологий](#стек-технологий)
- [Архитектура](#архитектура)
- [Быстрый старт](#быстрый-старт)
- [API Reference](#api-reference)
- [Структура проекта](#структура-проекта)

## ✨ Возможности

- **Загрузка логов** - Поддержка JSON и TXT форматов
- **Парсинг логов** - Автоматическое определение формата и извлечение данных
- **Анализ в реальном времени** - Обнаружение аномалий и паттернов
- **Дашборд** - Визуализация статистики и метрик
- **2FA аутентификация** - Двухфакторная защита с TOTP
- **WebSocket уведомления** - Real-time оповещения о событиях
- **Пользовательские правила** - Настройка паттернов для обнаружения
- **Rate Limiting** - Защита от злоупотреблений

## 🛠 Стек технологий

### Backend
- **C++20** - Современный стандарт
- **Drogon** - Высокопроизводительный HTTP фреймворк
- **libpqxx** - Клиент PostgreSQL
- **jwt-cpp** - JWT токены
- **bcrypt** - Хэширование паролей
- **OpenSSL** - Криптография

### Frontend
- **React 18** - UI библиотека
- **TypeScript** - Типизация
- **Vite** - Сборщик
- **TailwindCSS** - Стилизация
- **Zustand** - State management
- **React Router** - Роутинг
- **Recharts** - Графики
- **Axios** - HTTP клиент

### Database
- **PostgreSQL 15** - Основная БД

## 🏗 Архитектура

```
┌─────────────────────────────────────────────────────────────┐
│                    FRONTEND (React)                         │
│  Login │ Dashboard │ Log Viewer │ Analytics │ Settings      │
└─────────────────────────────────────────────────────────────┘
                            ↕ HTTP/WebSocket
┌─────────────────────────────────────────────────────────────┐
│                   BACKEND (C++ Drogon)                      │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Controllers: Auth, Log, Analytics, TwoFA, WebSocket  │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Services: Auth, LogParser, LogAnalysis, TwoFA, WS    │   │
│  └──────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ Middleware: JWT Auth, Rate Limiting                  │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                            ↕ libpqxx
┌─────────────────────────────────────────────────────────────┐
│                  PostgreSQL Database                        │
│  users │ logs │ analytics │ two_factor_auth │ log_queue     │
│  analysis_rules │ log_analytics                             │
└─────────────────────────────────────────────────────────────┘
```

## 🚀 Быстрый старт

### Предварительные требования

- C++ компилятор с поддержкой C++20
- CMake 3.15+
- vcpkg
- PostgreSQL 15+
- Node.js 18+
- Docker (опционально для БД)

### 1. Запуск базы данных

```bash
# С помощью Docker
cd database
docker-compose up -d

# Или вручную установите PostgreSQL и создайте БД
psql -U postgres
CREATE DATABASE wtld_db;
CREATE USER wtld_user WITH PASSWORD 'wtld_password';
GRANT ALL PRIVILEGES ON DATABASE wtld_db TO wtld_user;
```

### 2. Применение миграций

Миграции находятся в `backend/migrations/`. Примените их последовательно:

```bash
psql -U wtld_user -d wtld_db -f backend/migrations/001_create_users.sql
psql -U wtld_user -d wtld_db -f backend/migrations/002_create_logs.sql
psql -U wtld_user -d wtld_db -f backend/migrations/003_create_analytics.sql
psql -U wtld_user -d wtld_db -f backend/migrations/004_create_2fa.sql
psql -U wtld_user -d wtld_db -f backend/migrations/005_create_log_queue.sql
psql -U wtld_user -d wtld_db -f backend/migrations/006_create_enhanced_analytics.sql
```

### 3. Сборка бэкенда

```bash
cd backend

# Установка зависимостей через vcpkg
vcpkg install drogon libpqxx jwt-cpp openssl nlohmann-json bcrypt

# Создание директории сборки
mkdir build && cd build

# Конфигурация и сборка
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# Запуск сервера
./Release/WTLD_Backend.exe   # Windows
# или
./WTLD_Backend               # Linux/Mac
```

### 4. Запуск фронтенда

```bash
cd frontend

# Установка зависимостей
npm install

# Запуск в режиме разработки
npm run dev

# Сборка для продакшена
npm run build
```

### 5. Доступ к приложению

- Frontend: http://localhost:3000
- Backend API: http://localhost:8080
- API Documentation: http://localhost:8080/api

## 📡 API Reference

### Аутентификация

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/api/auth/register` | Регистрация пользователя |
| POST | `/api/auth/login`    | Вход |
| POST | `/api/auth/logout`   | Выход |
| GET  | `/api/auth/profile`  | Профиль пользователя |

### Логи

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST  | `/api/logs/upload`  | Загрузка файла логов |
| GET   | `/api/logs`         | Список логов |
| GET   | `/api/logs/{id}`    | Лог по ID |
| DELETE| `/api/logs/{id}`    | Удаление лога |
| GET   | `/api/logs/{id}/stats` | Статистика лога |

### Аналитика

| Метод | Endpoint | Описание |
|-------|----------|----------|
| GET   | `/api/analytics/dashboard` | Дашборд данные |
| GET   | `/api/analytics/{logId}`   | Аналитика лога |
| GET   | `/api/analytics/{logId}/anomalies` | Аномалии |
| GET   | `/api/analytics/rules`     | Правила анализа |
| POST  | `/api/analytics/rules`     | Создать правило |
| DELETE| `/api/analytics/rules/{id}`| Удалить правило |

### 2FA

| Метод | Endpoint | Описание |
|-------|----------|----------|
| POST | `/api/2fa/setup`  | Настройка 2FA |
| POST | `/api/2fa/verify` | Проверка кода |
| POST | `/api/2fa/enable` | Включить 2FA |
| POST | `/api/2fa/disable`| Отключить 2FA |
| GET  | `/api/2fa/status` | Статус 2FA |
| GET  | `/api/2fa/backup-codes` | Резервные коды |

### WebSocket

| Endpoint | Описание |
|----------|----------|
| `GET /api/ws?token=<jwt>` | WebSocket подключение |

## 📁 Структура проекта

```
WTLD/
├── backend/
│   ├── CMakeLists.txt
│   ├── config.json
│   ├── vcpkg.json
│   ├── include/
│   │   ├── controllers/
│   │   │   ├── AuthController.h
│   │   │   ├── LogController.h
│   │   │   ├── AnalyticsController.h
│   │   │   ├── TwoFAController.h
│   │   │   └── WebSocketController.h
│   │   ├── services/
│   │   │   ├── AuthService.h
│   │   │   ├── LogParserService.h
│   │   │   ├── LogAnalysisService.h
│   │   │   ├── TwoFactorAuthService.h
│   │   │   └── WebSocketService.h
│   │   ├── middleware/
│   │   │   ├── JwtMiddleware.h
│   │   │   └── RateLimitMiddleware.h
│   │   └── models/
│   │       ├── User.h
│   │       └── LogEntry.h
│   ├── src/
│   │   ├── main.cpp
│   │   ├── controllers/
│   │   ├── services/
│   │   └── middleware/
│   └── migrations/
│       └── *.sql
├── frontend/
│   ├── package.json
│   ├── vite.config.ts
│   ├── tsconfig.json
│   ├── tailwind.config.js
│   └── src/
│       ├── App.tsx
│       ├── main.tsx
│       ├── api/
│       ├── components/
│       ├── pages/
│       ├── store/
│       └── types/
├── database/
│   └── docker-compose.yml
└── docs/
```

## 🔒 Безопасность

- Пароли хэшируются с помощью bcrypt
- JWT токены с expiration 24 часа
- Поддержка TOTP 2FA
- Rate limiting для защиты от brute-force
- HTTPS рекомендуется для продакшена

## 📊 Типы анализов

1. **Error Rate Analysis** - Анализ процента ошибок
2. **Error Burst Detection** - Обнаружение всплесков ошибок
3. **Pattern Matching** - Поиск по пользовательским паттернам
4. **Anomaly Detection** - Статистическое обнаружение аномалий
5. **Critical Pattern Detection** - Поиск критических ошибок

## 🤝 Вклад

1. Fork репозиторий
2. Создайте feature branch (`git checkout -b feature/amazing-feature`)
3. Commit изменения (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Откройте Pull Request

## 📄 Лицензия

MIT License - см. файл [LICENSE](LICENSE)
