# WTLD – Web-Platform for Log Analysis

**WTLD** — веб-платформа для сбора, обработки и визуализации логов в реальном времени.
Бэкенд на C++20 (Drogon), фронтенд на React + TypeScript, база данных PostgreSQL.
Поддерживает JWT-аутентификацию, двухфакторную аутентификацию (TOTP), загрузку и парсинг
лог-файлов, статистику и детект аномалий, WebSocket-канал для real-time обновлений.

## 📦 Технологический стек

### Backend
- **C++20** — корутины, многопоточность
- **Drogon 1.9.x** — высокопроизводительный HTTP-фреймворк
- **libpqxx** — клиент PostgreSQL
- **jwt-cpp** — генерация и проверка JWT-токенов
- **OpenSSL** — криптография, TLS
- **libsodium** — хеширование паролей (Argon2id)
- **nlohmann-json** — работа с JSON
- **vcpkg** — менеджер пакетов (manifest-режим, `backend/vcpkg.json`)

### Frontend
- **React 18 + TypeScript** — UI и типизация
- **Vite** — сборщик и dev-сервер
- **TailwindCSS** — стилизация
- **Zustand** — управление состоянием
- **React Router** — маршрутизация
- **Recharts** — графики и визуализация
- **Axios** — HTTP-клиент

### Database
- **PostgreSQL 15+** (проверено на 16 и 18)

## 🏗 Архитектура

```
frontend (React)  <--HTTP/WebSocket-->  backend (C++ Drogon)  <--libpqxx-->  PostgreSQL
```

- **Контроллеры**: Auth, Log, Analytics, TwoFA, WebSocket, HttpStatus.
  В Drogon 1.9.x роуты регистрируются автоматически через CRTP-макросы
  (`METHOD_LIST_BEGIN` / `ADD_METHOD_TO`, `WS_PATH_ADD`).
- **Сервисы**: Auth, LogParser, LogAnalysis, TwoFactorAuth, WebSocket.
- **Middleware**: JwtMiddleware, RateLimitMiddleware (модули фильтров; проверка JWT
  дополнительно выполняется в каждом защищённом контроллере через
  `utils::getUserIdFromRequest`).

## 🚀 Быстрый старт

### Предварительные требования
- Visual Studio 2022 с компонентом C++ (MSVC, поддержка C++20)
- CMake ≥ 3.15 (с поддержкой presets), Ninja
- vcpkg (https://github.com/microsoft/vcpkg)
- Node.js ≥ 18
- PostgreSQL 15+ **или** Docker (для базы данных)

### 1. Запуск базы данных

**Вариант A — Docker:**
```bash
cd database
docker compose up -d
```
Миграции применяются автоматически при первом старте контейнера
(каталог `migrations/` смонтирован в `/docker-entrypoint-initdb.d`).

**Вариант B — локальный PostgreSQL:**
```sql
CREATE USER wtld_user WITH PASSWORD 'wtld_password';
CREATE DATABASE wtld_db OWNER wtld_user;
```
Применить миграции по порядку (001…007), например:
```bash
psql -U wtld_user -d wtld_db -f database/migrations/001_create_users.sql
psql -U wtld_user -d wtld_db -f database/migrations/002_create_logs.sql
# … и так далее до 007
```
Выдать права (если миграции применялись от суперпользователя):
```sql
GRANT ALL ON SCHEMA public TO wtld_user;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO wtld_user;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO wtld_user;
```

### 2. Сборка и запуск бэкенда (Windows)

> ⚠️ Собирать **только** в консоли **«x64 Native Tools Command Prompt for VS 2022»** —
> вне её компилятор не видит заголовки MSVC/Windows SDK.

```bat
cd backend
cmake --preset windows-msvc -DCMAKE_BUILD_TYPE=Release
cmake --build build\windows-msvc
build\windows-msvc\WTLD_Backend.exe
```
Запускать exe **из папки `backend`** — рядом должен лежать `config.json`.

Важно:
- Зависимости ставятся из манифеста `vcpkg.json`. Пакет `drogon` объявлен с фичей
  `postgres` — без неё сервер завершится ошибкой
  `No database is supported by drogon`.
- Сборка **только Release**: смешивание Debug-библиотек с Release-DLL ломает рантайм (CRT).
- DLL копируются POST_BUILD-командой из `build/windows-msvc/vcpkg_installed`
  (см. `VCPKG_BIN_DIR` в `CMakeLists.txt`).
- Если пути к vcpkg/Ninja/MSVC у вас другие — поправьте `cacheVariables`
  в `backend/CMakePresets.json` либо сконфигурируйте вручную:
  ```bat
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_TOOLCHAIN_FILE=<путь к vcpkg>\scripts\buildsystems\vcpkg.cmake ^
        -DVCPKG_TARGET_TRIPLET=x64-windows
  cmake --build build
  ```

### 3. Сборка и запуск фронтенда

```bash
cd frontend
npm install
npm run dev
```
Dev-сервер поднимается на **http://localhost:3000**; прокси `/api → http://localhost:8080`
уже настроен в `vite.config.ts`.

### 4. Доступ к приложению
- **Frontend**: http://localhost:3000 (лендинг `/`, защищённая зона `/app/*`)
- **Backend API**: http://localhost:8080

## 🎬 Типовой сценарий демо
1. Регистрация пользователя (`/register`) или вход (`/login`)
2. Загрузка лог-файла (`.txt` / `.json`) на странице upload
3. Просмотр списка логов и статистики по файлу
4. Дашборд аналитики (`/app/dashboard`) и список аномалий
5. (Опционально) Подключение 2FA: QR-код → код из приложения → резервные коды

## 📚 Документация API

Подробное описание эндпоинтов — в `docs/API.md`. Основные группы:

| Группа | Эндпоинты |
| --- | --- |
| **Аутентификация** | `/api/auth/register`, `/api/auth/login`, `/api/auth/logout`, `/api/auth/profile` |
| **Логи** | `/api/logs/upload`, `/api/logs`, `/api/logs/{id}`, `/api/logs/{id}/stats` |
| **Аналитика** | `/api/analytics/dashboard`, `/api/analytics/{logId}`, `/api/analytics/{logId}/anomalies`, `/api/analytics/rules` |
| **2FA** | `/api/2fa/setup`, `/api/2fa/verify`, `/api/2fa/enable`, `/api/2fa/disable`, `/api/2fa/status` |
| **WebSocket** | `GET /api/ws?token=<jwt>` (real-time канал) |

## 🔐 Безопасность
- Хеширование паролей — **Argon2id** (libsodium)
- JWT-токены со сроком жизни 24 ч
- Поддержка **TOTP 2FA** (совместимо с Google Authenticator)
- Модуль rate limiting (`RateLimitMiddleware`) для защиты от brute-force
- В продакшене рекомендуется HTTPS (OpenSSL)

## 🧰 Troubleshooting (Windows)

| Симптом | Причина / решение |
| --- | --- |
| `fatal error C1083: stdint.h: No such file or directory` | Сборка вне консоли разработчика. Откройте «x64 Native Tools Command Prompt for VS 2022» |
| `LNK1168: не удается открыть WTLD_Backend.exe для записи` | Сервер запущен. Остановите его (Ctrl+C в его окне или `taskkill /F /IM WTLD_Backend.exe`) и пересоберите |
| `No database is supported by drogon` | drogon собран без фичи `postgres`, либо рядом с exe лежит «пустая» `drogon.dll`. Проверьте `vcpkg.json` и пересоберите в Release |
| `docker-compose` не найдена | Используйте команду v2: `docker compose up -d` (без дефиса) |
| Белый экран на `/app/dashboard` | Исправлено в 0.5.0b (optional chaining); если встретится — проверьте ответ `/api/analytics/dashboard` в DevTools → Network |

## 📌 Текущее состояние
- **0.5.0b** — рабочий full-stack: аутентификация, загрузка и парсинг логов,
  аналитика и дашборд, 2FA, WebSocket-канал (real-time обновления в стадии интеграции)

## 🤝 Вклад
1. Fork репозиторий
2. Создайте ветку `feature/your-feature`
3. Commit с осмысленным сообщением
4. Push и откройте Pull Request

## 📄 Лицензия
MIT License – см. файл LICENSE