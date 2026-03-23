# WTLD API Documentation

## Base URL
```
http://localhost:8080/api
```

## Authentication

Большинство endpoints требуют JWT токен в заголовке:
```
Authorization: Bearer <your_jwt_token>
```

---

## Auth Endpoints

### POST /auth/register

Регистрация нового пользователя.

**Request Body:**
```json
{
  "username": "string (required, min 3 chars)",
  "email": "string (required, email format)",
  "password": "string (required, min 6 chars)"
}
```

**Response 201 Created:**
```json
{
  "status": "success",
  "data": {
    "id": 1,
    "username": "john_doe",
    "email": "john@example.com",
    "created_at": 1711180800,
    "is_active": true
  }
}
```

**Response 409 Conflict:**
```json
{
  "status": "error",
  "message": "User already exists"
}
```

---

### POST /auth/login

Вход в систему.

**Request Body:**
```json
{
  "username": "string",
  "password": "string"
}
```

**Response 200 OK:**
```json
{
  "status": "success",
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "user": {
    "id": 1,
    "username": "john_doe",
    "email": "john@example.com",
    "is_active": true
  }
}
```

**Response 401 Unauthorized:**
```json
{
  "status": "error",
  "message": "Invalid credentials"
}
```

---

### POST /auth/logout

Выход из системы.

**Response 200 OK:**
```json
{
  "status": "success",
  "message": "Logged out successfully"
}
```

---

### GET /auth/profile

Получение профиля пользователя.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "id": 1,
  "username": "john_doe",
  "email": "john@example.com",
  "created_at": 1711180800,
  "last_login": 1711267200,
  "is_active": true
}
```

---

## Log Endpoints

### POST /logs/upload

Загрузка файла логов.

**Headers:**
```
Authorization: Bearer <token>
Content-Type: multipart/form-data
```

**Request Body:**
- `file`: File (required, .txt, .json, .log)

**Response 201 Created:**
```json
{
  "status": "success",
  "data": {
    "id": 1,
    "filename": "app.log",
    "file_type": "txt",
    "file_size": 1024000,
    "entries_count": 1500,
    "anomalies_found": 3
  }
}
```

---

### GET /logs

Получение списка логов.

**Headers:**
```
Authorization: Bearer <token>
```

**Query Parameters:**
- `status` (optional): pending | processing | completed | failed
- `fileType` (optional): json | txt
- `limit` (optional, default: 20)

**Response 200 OK:**
```json
{
  "status": "success",
  "data": [
    {
      "id": 1,
      "filename": "app.log",
      "file_type": "txt",
      "file_size": 1024000,
      "upload_date": "2024-03-23T10:30:00",
      "status": "completed"
    }
  ]
}
```

---

### GET /logs/{id}

Получение лога по ID.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "data": {
    "id": 1,
    "filename": "app.log",
    "file_type": "txt",
    "file_size": 1024000,
    "upload_date": "2024-03-23T10:30:00",
    "status": "completed",
    "entries": [
      {
        "timestamp": "2024-03-23T10:30:00",
        "level": "ERROR",
        "message": "Database connection failed",
        "source": "DatabaseService",
        "metadata": {}
      }
    ]
  }
}
```

---

### DELETE /logs/{id}

Удаление лога.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "message": "Log deleted"
}
```

---

### GET /logs/{id}/stats

Получение статистики лога.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "data": {
    "statistics": {
      "total": 1500,
      "by_level": {
        "INFO": 1000,
        "WARNING": 300,
        "ERROR": 150,
        "CRITICAL": 50
      },
      "by_source": {
        "DatabaseService": 500,
        "AuthService": 300
      },
      "error_rate": 13.33
    },
    "analytics": [
      {
        "type": "error_rate",
        "severity": "medium",
        "title": "Повышенный уровень ошибок",
        "is_anomaly": true
      }
    ]
  }
}
```

---

## Analytics Endpoints

### GET /analytics/dashboard

Получение данных дашборда.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "statistics": {
    "total_logs": 25,
    "completed_logs": 23,
    "total_anomalies": 15,
    "active_rules": 5
  },
  "recent_anomalies": [
    {
      "id": 1,
      "type": "error_rate",
      "severity": "high",
      "title": "Высокий уровень ошибок",
      "created_at": "2024-03-23T10:30:00"
    }
  ],
  "severity_distribution": {
    "critical": 2,
    "high": 5,
    "medium": 6,
    "low": 2
  }
}
```

---

### GET /analytics/{logId}

Получение аналитики лога.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "data": [
    {
      "id": 1,
      "type": "error_rate",
      "severity": "high",
      "title": "Высокий уровень ошибок",
      "description": "Более 25% логов являются ошибками",
      "data": {
        "total_logs": 1500,
        "error_count": 450,
        "error_rate_percent": 30.0
      },
      "detected_patterns": [],
      "is_anomaly": true,
      "created_at": "2024-03-23T10:30:00"
    }
  ]
}
```

---

### GET /analytics/{logId}/anomalies

Получение аномалий лога.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "data": [
    {
      "id": 1,
      "type": "anomaly",
      "severity": "critical",
      "title": "Аномальный всплеск ошибок",
      "description": "Обнаружено необычно высокое количество ошибок",
      "data": {
        "minute": "2024-03-23T10:30",
        "error_count": 150,
        "average": 25.5,
        "z_score": 3.5
      },
      "created_at": "2024-03-23T10:30:00"
    }
  ]
}
```

---

### GET /analytics/rules

Получение правил анализа.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "data": [
    {
      "id": 1,
      "name": "Database Errors",
      "type": "regex",
      "pattern": ".*database.*error.*",
      "severity": "high",
      "is_active": true
    }
  ]
}
```

---

### POST /analytics/rules

Создание правила анализа.

**Headers:**
```
Authorization: Bearer <token>
```

**Request Body:**
```json
{
  "name": "string (required)",
  "type": "regex | keyword | threshold | custom",
  "pattern": "string (required)",
  "severity": "low | medium | high | critical"
}
```

**Response 201 Created:**
```json
{
  "status": "success",
  "data": {
    "id": 1
  }
}
```

---

### DELETE /analytics/rules/{id}

Удаление правила.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "message": "Rule deleted"
}
```

---

## 2FA Endpoints

### POST /2fa/setup

Настройка 2FA.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "data": {
    "secret": "JBSWY3DPEHPK3PXP",
    "qr_uri": "otpauth://totp/WTLD:john_doe?secret=JBSWY3DPEHPK3PXP&issuer=WTLD&algorithm=SHA1&digits=6&period=30",
    "backup_codes": ["12345678", "87654321", ...]
  }
}
```

---

### POST /2fa/verify

Проверка 2FA кода.

**Headers:**
```
Authorization: Bearer <token>
```

**Request Body:**
```json
{
  "code": "string (6 digits)"
}
```

**Response 200 OK:**
```json
{
  "status": "success",
  "valid": true
}
```

---

### POST /2fa/enable

Включение 2FA.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "message": "2FA enabled successfully"
}
```

---

### POST /2fa/disable

Отключение 2FA.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "message": "2FA disabled successfully"
}
```

---

### GET /2fa/status

Статус 2FA.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "data": {
    "is_enabled": true,
    "is_configured": true
  }
}
```

---

### GET /2fa/backup-codes

Получение резервных кодов.

**Headers:**
```
Authorization: Bearer <token>
```

**Response 200 OK:**
```json
{
  "status": "success",
  "data": {
    "backup_codes": ["12345678", "87654321", ...],
    "remaining": 8
  }
}
```

---

## WebSocket

### GET /api/ws?token=<jwt>

WebSocket подключение для real-time уведомлений.

**Query Parameters:**
- `token`: JWT токен

**Event Types:**
- `ANOMALY_DETECTED` - Обнаружена аномалия
- `LOG_UPLOADED` - Лог загружен
- `ANALYSIS_COMPLETE` - Анализ завершен
- `REALTIME_LOG` - Real-time лог
- `SYSTEM_NOTIFICATION` - Системное уведомление

**Example Event:**
```json
{
  "type": "ANOMALY_DETECTED",
  "title": "Критическая ошибка",
  "message": "Обнаружена критическая ошибка в логе",
  "data": {
    "log_id": 1,
    "severity": "critical"
  },
  "timestamp": "2024-03-23T10:30:00"
}
```

---

## Error Codes

| Code | Description |
|------|-------------|
| 200 | Success |
| 201 | Created |
| 400 | Bad Request |
| 401 | Unauthorized |
| 403 | Forbidden |
| 404 | Not Found |
| 409 | Conflict |
| 429 | Too Many Requests |
| 500 | Internal Server Error |

---

## Rate Limiting

- Default: 100 requests per minute per IP
- Headers included in response:
  - `X-RateLimit-Limit`: Maximum requests
  - `X-RateLimit-Remaining`: Remaining requests
  - `X-RateLimit-Reset`: Seconds until reset
