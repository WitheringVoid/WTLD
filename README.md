# WTLD – Web‑Platform for Log Analysis

**WTLD** – это современная веб‑платформа для сбора, обработки и визуализации логов в реальном времени. Проект реализован с использованием C++ (backend) и TypeScript/React (frontend), поддерживает двухфакторную аутентификацию (2FA) и масштабируемую работу с базой данных PostgreSQL.

## 📦 Технологический стек

### Backend
- **C++20** – современный язык с поддержкой корутин и многопоточности  
- **Drogon** – высокопроизводительный HTTP‑фреймворк  
- **libpqxx** – клиент PostgreSQL  
- **jwt‑cpp** – генерация и проверка JWT‑токенов  
- **OpenSSL** – криптография, TLS  
- **bcrypt** – хеширование паролей  
- **nlohmann‑json** – работа с JSON  
- **vcpkg** – менеджер пакетов для C++  

### Frontend
- **React 18** + **TypeScript** – UI и типизация  
- **Vite** – быстрый сборщик  
- **TailwindCSS** – стилизация  
- **Zustand** – управление состоянием  
- **React Router** – роутинг  
- **Recharts** – графики и визуализация  
- **Axios** – HTTP‑клиент  

### Database
- **PostgreSQL 15** – основная СУБД  

## 🏗 Архитектура

```
frontend (React)  <--HTTP/WebSocket-->  backend (C++ Drogon)  <--libpqxx-->  PostgreSQL
```

- **Контроллеры**: Auth, Log, Analytics, TwoFA, WebSocket  
- **Сервисы**: Auth, LogParser, LogAnalysis, TwoFactorAuth, WebSocket  
- **Middleware**: JWT‑auth, Rate‑limiting  

## 🚀 Быстрый старт

### Предварительные требования
- **C++ компилятор** с поддержкой C++20 (MSVC 2022 или clang/gcc ≥ 10)  
- **CMake** ≥ 3.15  
- **vcpkg** (см. https://github.com/microsoft/vcpkg)  
- **Node.js** ≥ 18  
- **Docker** (опционально, для БД)  
- **PostgreSQL** ≥ 15  

### 1. Запуск базы данных
```bash
cd database
docker-compose up -d          # или установить PostgreSQL вручную
```

### 2. Применение миграций
```bash
psql -U wtld_user -d wtld_db -f database/migrations/001_create_users.sql
psql -U wtld_user -d wtld_db -f database/migrations/002_create_logs.sql
# … остальные миграции …
```

### 3. Сборка бэкенда
```bash
cd backend

# Установка зависимостей через vcpkg
vcpkg install drogon libpqxx jwt-cpp openssl nlohmann-json bcrypt

# Конфигурация и сборка
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 \
    -DCMAKE_TOOLCHAIN_FILE="D:/VS_Code_projects/sources/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build . --config Release
```

Запуск сервера:
```bash
cd build/Release
.\WTLD_Backend.exe   # Windows
# или ./WTLD_Backend   # Linux/macOS
```

### 4. Сборка и запуск фронтенда
```bash
cd ../../frontend
npm install
npm run dev          # разработка (http://localhost:5173)
npm run build        # продакшн‑сборка в ./dist
```

### 5. Доступ к приложению
- **Frontend**: http://localhost:5173  
- **Backend API**: http://localhost:8080  
- **Swagger/OpenAPI**: http://localhost:8080/api-docs  

## 📚 Документация API
Подробное описание эндпоинтов находится в `docs/API.md`. Основные группы:

| Группа | Эндпоинты |
|--------|-----------|
| **Аутентификация** | `/api/auth/register`, `/api/auth/login`, `/api/auth/logout`, `/api/auth/profile` |
| **Логи** | `/api/logs/upload`, `/api/logs`, `/api/logs/{id}`, `/api/logs/{id}/stats` |
| **Аналитика** | `/api/analytics/dashboard`, `/api/analytics/{logId}`, `/api/analytics/{logId}/anomalies`, `/api/analytics/rules` |
| **2FA** | `/api/2fa/setup`, `/api/2fa/verify`, `/api/2fa/enable`, `/api/2fa/disable`, `/api/2fa/status` |
| **WebSocket** | `GET /api/ws?token=<jwt>` (реальное время) |

## 🔐 Безопасность
- Хеширование паролей с **bcrypt**  
- JWT‑токены с сроком жизни 24 ч.  
- Поддержка **TOTP 2FA** (Google Authenticator совместимо)  
- **Rate limiting** для защиты от brute‑force атак  
- Рекомендовано использовать **HTTPS** в продакшене (OpenSSL)  

## 🤝 Вклад
1. Fork репозиторий  
2. Создайте ветку `feature/your-feature`  
3. Commit с осмысленным сообщением  
4. Push и откройте Pull Request  

## 📄 Лицензия
MIT License – см. файл [LICENSE](LICENSE)