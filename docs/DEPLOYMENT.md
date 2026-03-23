# WTLD - Инструкция по развертыванию

## Production развертывание

### 1. Подготовка сервера

#### Требования
- Linux server (Ubuntu 22.04 recommended)
- 4+ CPU cores
- 8+ GB RAM
- 50+ GB SSD storage

#### Установка зависимостей

```bash
# Обновление системы
sudo apt update && sudo apt upgrade -y

# Установка компилятора и инструментов
sudo apt install -y build-essential cmake git curl wget

# Установка Node.js
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt install -y nodejs

# Установка PostgreSQL
sudo apt install -y postgresql postgresql-contrib

# Установка vcpkg
cd /opt
sudo git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
sudo ./bootstrap-vcpkg.sh
sudo ln -s /opt/vcpkg/vcpkg /usr/local/bin/
```

### 2. Настройка PostgreSQL

```bash
# Создание БД и пользователя
sudo -u postgres psql << EOF
CREATE DATABASE wtld_db;
CREATE USER wtld_user WITH PASSWORD 'secure_password_here';
GRANT ALL PRIVILEGES ON DATABASE wtld_db TO wtld_user;
\c wtld_db
GRANT ALL ON SCHEMA public TO wtld_user;
EOF

# Применение миграций
sudo -u postgres psql -d wtld_db -f /path/to/backend/migrations/001_create_users.sql
sudo -u postgres psql -d wtld_db -f /path/to/backend/migrations/002_create_logs.sql
sudo -u postgres psql -d wtld_db -f /path/to/backend/migrations/003_create_analytics.sql
sudo -u postgres psql -d wtld_db -f /path/to/backend/migrations/004_create_2fa.sql
sudo -u postgres psql -d wtld_db -f /path/to/backend/migrations/005_create_log_queue.sql
sudo -u postgres psql -d wtld_db -f /path/to/backend/migrations/006_create_enhanced_analytics.sql
```

### 3. Сборка бэкенда

```bash
cd /opt/wtld/backend

# Установка зависимостей
vcpkg install drogon:x64-linux libpqxx:x64-linux jwt-cpp:x64-linux openssl:x64-linux nlohmann-json:x64-linux bcrypt:x64-linux

# Сборка
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
make -j$(nproc)

# Копирование бинарника
sudo cp WTLD_Backend /usr/local/bin/
```

### 4. Настройка systemd сервиса

```bash
sudo nano /etc/systemd/system/wtld-backend.service
```

```ini
[Unit]
Description=WTLD Backend Service
After=network.target postgresql.service

[Service]
Type=simple
User=wtld
WorkingDirectory=/opt/wtld/backend
ExecStart=/usr/local/bin/WTLD_Backend
Restart=on-failure
RestartSec=5
LimitNOFILE=65535

# Security
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
```

```bash
# Создание пользователя
sudo useradd -r -s /bin/false wtld
sudo chown -R wtld:wtld /opt/wtld

# Запуск сервиса
sudo systemctl daemon-reload
sudo systemctl enable wtld-backend
sudo systemctl start wtld-backend
sudo systemctl status wtld-backend
```

### 5. Сборка и настройка фронтенда

```bash
cd /opt/wtld/frontend

# Установка зависимостей
npm ci --production

# Сборка
npm run build

# Настройка nginx
sudo nano /etc/nginx/sites-available/wtld
```

```nginx
server {
    listen 80;
    server_name your-domain.com;

    # Redirect HTTP to HTTPS
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl http2;
    server_name your-domain.com;

    # SSL configuration
    ssl_certificate /etc/letsencrypt/live/your-domain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/your-domain.com/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;

    # Frontend
    root /opt/wtld/frontend/build;
    index index.html;

    location / {
        try_files $uri $uri/ /index.html;
    }

    # Backend API proxy
    location /api {
        proxy_pass http://localhost:8080;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_cache_bypass $http_upgrade;
    }

    # WebSocket
    location /api/ws {
        proxy_pass http://localhost:8080;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }

    # Security headers
    add_header X-Frame-Options "SAMEORIGIN" always;
    add_header X-Content-Type-Options "nosniff" always;
    add_header X-XSS-Protection "1; mode=block" always;
    add_header Referrer-Policy "strict-origin-when-cross-origin" always;
}
```

```bash
# Включение сайта
sudo ln -s /etc/nginx/sites-available/wtld /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl restart nginx

# SSL сертификат
sudo apt install -y certbot python3-certbot-nginx
sudo certbot --nginx -d your-domain.com
```

### 6. Мониторинг и логи

```bash
# Установка Prometheus Node Exporter
sudo useradd -rs /bin/false prometheus
cd /opt
sudo wget https://github.com/prometheus/node_exporter/releases/download/v1.7.0/node_exporter-1.7.0.linux-amd64.tar.gz
sudo tar xvfz node_exporter-*.tar.gz
sudo mv node_exporter-*/node_exporter /usr/local/bin/

# Создание сервиса
sudo nano /etc/systemd/system/node_exporter.service
```

```ini
[Unit]
Description=Node Exporter
After=network.target

[Service]
User=prometheus
ExecStart=/usr/local/bin/node_exporter

[Install]
WantedBy=default.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable node_exporter
sudo systemctl start node_exporter
```

### 7. Бэкап базы данных

```bash
# Скрипт для бэкапа
sudo nano /opt/scripts/backup-wtld.sh
```

```bash
#!/bin/bash
DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR="/var/backups/wtld"
mkdir -p $BACKUP_DIR

pg_dump -U wtld_user wtld_db | gzip > $BACKUP_DIR/wtld_db_$DATE.sql.gz

# Удаление старых бэкапов (старше 30 дней)
find $BACKUP_DIR -name "*.sql.gz" -mtime +30 -delete
```

```bash
# Добавление в crontab
sudo crontab -e
# 0 2 * * * /opt/scripts/backup-wtld.sh
```

### 8. Проверка безопасности

```bash
# Проверка открытых портов
sudo ufw status
sudo ufw allow 22/tcp    # SSH
sudo ufw allow 80/tcp    # HTTP
sudo ufw allow 443/tcp   # HTTPS
sudo ufw enable

# Проверка логов
sudo journalctl -u wtld-backend -f
sudo tail -f /var/log/nginx/error.log

# Аудит безопасности
sudo apt install -y lynis
sudo lynis audit system
```

## Переменные окружения

Создайте файл `.env` для чувствительных данных:

```bash
# Backend config.json
{
    "app": {
        "secret_key": "your-super-secret-key-change-in-production"
    },
    "clients": {
        "postgresql": {
            "password": "secure_password_here"
        }
    }
}
```

## Troubleshooting

### Бэкенд не запускается
```bash
sudo journalctl -u wtld-backend -n 50
# Проверьте логи PostgreSQL
sudo tail -f /var/log/postgresql/postgresql-15-main.log
```

### Frontend не загружается
```bash
# Проверьте nginx конфиг
sudo nginx -t
sudo systemctl status nginx
```

### Проблемы с подключением к БД
```bash
# Проверьте подключение
psql -h localhost -U wtld_user -d wtld_db
# Проверьте pg_hba.conf
sudo nano /etc/postgresql/15/main/pg_hba.conf
```

## Performance tuning

### PostgreSQL оптимизация
```bash
sudo nano /etc/postgresql/15/main/postgresql.conf
```

```conf
shared_buffers = 2GB
effective_cache_size = 6GB
work_mem = 64MB
maintenance_work_mem = 512MB
max_connections = 200
```

### Backend оптимизация
```json
// config.json
{
    "app": {
        "number_of_threads": 8,
        "log_level": 1
    },
    "clients": {
        "postgresql": {
            "number_of_connections": 20
        }
    }
}
```
