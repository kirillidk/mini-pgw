# Mini-PGW Documentation

## Оглавление
1. [Обзор проекта](#обзор-проекта)
2. [Архитектура системы](#архитектура-системы)
3. [Сборка](#сборка)
4. [Конфигурация](#конфигурация)
5. [API документация](#api-документация)
6. [Протоколы и форматы данных](#протоколы-и-форматы-данных)
7. [Использование](#использование)
8. [Архитектурные решения](#архитектурные-решения)

## Обзор проекта

Mini-PGW — это упрощенная модель сетевого компонента Packet Gateway (PGW). Система способна обрабатывать UDP-запросы с IMSI абонентов, управлять их сессиями, вести журнал записей CDR и предоставлять HTTP API для мониторинга и управления.

### Основные возможности

- **UDP сервер** для обработки запросов создания сессий
- **HTTP API** для проверки статуса сессий и управления системой
- **Управление сессиями** с автоматическим истечением по таймеру
- **CDR журналирование** всех операций с сессиями
- **Blacklist поддержка** для блокировки определенных IMSI
- **Graceful shutdown** с контролируемой скоростью завершения сессий
- **Многопоточная архитектура** с использованием thread pool
- **Event-driven система** для координации между компонентами

## Сборка

### Требования

- **Компилятор**: GCC 13+ или Clang 15+ с поддержкой C++23
- **CMake**: версия 3.15+

### Зависимости

Все зависимости загружаются автоматически через CPM:

- **boost-ext/di**: Dependency Injection фреймворк
- **cpp-httplib**: HTTP сервер/клиент библиотека
- **Boost.Log**: Система логирования
- **nlohmann/json**: JSON парсер
- **magic_enum**: Enum to string конвертация

### Сборка

```bash
git clone https://github.com/kirillidk/mini-pgw
cd mini-pgw

mkdir build && cd build

cmake .. -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)
```

### Структура проекта после сборки

```
build/
├── bin/
│   ├── server              # Основной сервер
│   ├── client              # Тестовый клиент
│   ├── server_config.json  # Конфигурация сервера
│   └── client_config.json  # Конфигурация клиента
└── unit/                   # Unit тесты
```

## Архитектура системы

### Общая схема

```
┌─────────────────┐    UDP     ┌─────────────────┐
│   UDP Client    │◄─────────►│   UDP Server    │
└─────────────────┘            └─────────────────┘
                                        │
                               ┌─────────────────┐
                               │ Packet Manager  │
                               └─────────────────┘
                                        │
                               ┌─────────────────┐
                               │Session Manager  │
                               └─────────────────┘
                                        │
                    ┌──────────────────────────────────┐
                    │            Event Bus             │
                    └──────────────────────────────────┘
                             │            │
                    ┌─────────────────┐   ┌─────────────────┐
                    │   CDR Writer    │   │  HTTP Server    │
                    └─────────────────┘   └─────────────────┘
                             │                     │
                    ┌─────────────────┐   ┌─────────────────┐
                    │   cdr.log       │   │  HTTP Client    │
                    └─────────────────┘   └─────────────────┘
```

### Компоненты системы

#### Server Side
- **UDP Server**: Принимает UDP пакеты с IMSI, обрабатывает через epoll
- **HTTP Server**: REST API для проверки сессий и управления системой
- **Packet Manager**: Декодирует BCD пакеты и управляет жизненным циклом запросов
- **Session Manager**: Управляет активными сессиями и blacklist
- **CDR Writer**: Асинхронная запись событий в CDR файл
- **Event Bus**: Координирует взаимодействие между компонентами
- **Thread Pool**: Управляет пулом рабочих потоков

#### Client Side
- **UDP Client**: Отправляет IMSI запросы на сервер с таймаутом

#### Common Components
- **Config**: Загрузка и валидация JSON конфигурации
- **Logger**: Многоуровневое логирование с Boost.Log
- **Utility**: BCD кодирование/декодирование, валидация IMSI


## Конфигурация

### Конфигурация сервера (server_config.json)

```json
{
    "server_ip": "0.0.0.0",    
    "server_port": 9000,              
    "session_timeout_sec": 30,         
    "cdr_file": "cdr.log",            
    "http_port": 8081,                
    "graceful_shutdown_rate": 10,     
    "log_file": "pgw.log",            
    "log_level": "info",              
    "blacklist": [                    
        "001010123456789",
        "001010000000001",
        "001010999999999"
    ]
}
```

### Конфигурация клиента (client_config.json)

```json
{
    "server_ip": "127.0.0.1",         
    "server_port": 9000,              
    "log_file": "client.log",         
    "log_level": "INFO"               
}
```

### Параметры конфигурации

| Параметр | Тип | Описание | По умолчанию |
|----------|-----|----------|--------------|
| server_ip | string | IP адрес для привязки серверов | "0.0.0.0" |
| server_port | integer | UDP порт (1-65535) | 9000 |
| http_port | integer | HTTP API порт (1-65535) | 8081 |
| session_timeout_sec | integer | Время жизни сессии в секундах | 30 |
| graceful_shutdown_rate | integer | Скорость завершения сессий при shutdown | 10 |
| log_level | string | debug/info/warning/error/fatal | "info" |
| blacklist | array | Список заблокированных IMSI | [] |

## API документация

### HTTP API

Сервер предоставляет REST API для мониторинга и управления:

#### GET /check_subscriber

Проверяет статус сессии для указанного IMSI.

**Параметры запроса:**
- `imsi` (required): IMSI абонента (6-15 цифр)

**Примеры запросов:**

```bash
# Проверка активной сессии
curl "http://localhost:8081/check_subscriber?imsi=001010123456789"
# Ответ: active

# Проверка неактивной сессии
curl "http://localhost:8081/check_subscriber?imsi=001010999999999"
# Ответ: not active
```

**Коды ответов:**
- `200 OK`: Успешная проверка
- `400 Bad Request`: Неверные параметры
- `500 Internal Server Error`: Ошибка сервера

#### POST /stop

Инициирует graceful shutdown системы.

```bash
curl -X POST "http://localhost:8081/stop" -d ""
# Ответ: Graceful shutdown initiated
```

**Процесс graceful shutdown:**
1. Остановка приема новых UDP запросов
2. Завершение активных сессий со скоростью `graceful_shutdown_rate`
3. Запись CDR записей для каждой завершенной сессии
4. Закрытие HTTP сервера
5. Завершение работы приложения

### UDP Protocol

#### Запрос создания сессии

**Формат пакета:**
```
Byte 1: Type
Byte 2-3: Length
Byte 4: Spare & Instance
Byte 5+: BCD encoded IMSI
```

**BCD кодирование IMSI:**
- Каждая цифра кодируется в 4 бита
- Если количество цифр нечетное, добавляется 0xF
- Порядок: младшие 4 бита - вторая цифра, старшие 4 бита - первая цифра

**Пример:** IMSI "123456789" → BCD: [0x01, 0x00, 0x05, 0x00, 0x21, 0x43, 0x65, 0x87, 0xF9]

#### Ответы сервера

- `"created"`: Сессия успешно создана
- `"rejected"`: Сессия отклонена (IMSI в blacklist или сессия уже существует)

## Протоколы и форматы данных

### CDR Format

CDR записи сохраняются в текстовом формате:

```
timestamp, IMSI, action
```

**Примеры записей:**
```
2025-01-15 14:30:25.123, 001010123456789, created
2025-01-15 14:30:28.456, 001010123456789, deleted
2025-01-15 14:30:30.789, 001010999999999, rejected
```

**Типы действий:**
- `created`: Сессия создана
- `deleted`: Сессия удалена (по таймауту или при shutdown)
- `rejected`: Запрос отклонен

### Log Format

Логи используют Boost.Log формат:
```
[timestamp] [severity] message
```

**Пример:**
```
[2025-01-15 14:30:25.123456] [info] UDP server started on 0.0.0.0:9000
[2025-01-15 14:30:25.124789] [debug] Session created for IMSI: 001010123456789
[2025-01-15 14:30:28.456123] [info] Session expired for IMSI: 001010123456789
```

## Использование

### Запуск сервера

```bash
cd build/bin

# Запуск с конфигурацией по умолчанию
./server

# Логи будут записываться в pgw.log
# CDR записи в cdr.log
```

### Использование клиента

```bash
cd build/bin

# Отправка валидного IMSI
./client 001010123456789
# Ожидаемый ответ: Server response: created

# Отправка IMSI из blacklist
./client 001010000000001
# Ожидаемый ответ: Server response: rejected

# Отправка некорректного IMSI
./client 12345
# Ошибка: Invalid IMSI format. IMSI must be 6-15 digits.

# Использование кастомной конфигурации
./client 001010123456789 custom_client_config.json
```

## Архитектурные решения

### Design Patterns

#### Dependency Injection

```cpp
auto injector = di::make_injector(
    di::bind<std::filesystem::path>.to(std::filesystem::path{"server_config.json"}),
    di::bind<std::size_t>.to(size_t(std::thread::hardware_concurrency()))
);

auto server = injector.create<std::shared_ptr<udp_server>>();
```

#### Event-Driven Architecture

```cpp
// Подписка на события
_event_bus->subscribe<events::create_session_event>([this](std::string imsi) {
    // Обработка создания сессии
});

// Публикация событий
_event_bus->publish<events::create_session_event>(imsi);
```
