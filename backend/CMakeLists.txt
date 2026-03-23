cmake_minimum_required(VERSION 3.15)
project(WTLD_Backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Путь к vcpkg (исправьте если нужно)
set(VCPKG_ROOT "D:/VS_Code_projects/sources/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# Явно указываем пути к библиотекам (для надежности)
set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x64-windows")
set(VCPKG_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/include")
set(VCPKG_LIB_DIR "${VCPKG_INSTALLED_DIR}/lib")
set(VCPKG_BIN_DIR "${VCPKG_INSTALLED_DIR}/bin")

# Сообщаем CMake где искать библиотеки
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")
list(APPEND CMAKE_INCLUDE_PATH "${VCPKG_INCLUDE_DIR}")
list(APPEND CMAKE_LIBRARY_PATH "${VCPKG_LIB_DIR}")

# Поиск зависимостей
find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Threads REQUIRED)

# Поиск libpqxx вручную (так как с CONFIG могут быть проблемы)
find_path(LIBPQXX_INCLUDE_DIR
    NAMES pqxx/pqxx
    PATHS "${VCPKG_INCLUDE_DIR}"
    REQUIRED
)

find_library(LIBPQXX_LIBRARY
    NAMES pqxx libpqxx
    PATHS "${VCPKG_LIB_DIR}"
    REQUIRED
)

# Поиск jwt-cpp вручную
find_path(JWT_CPP_INCLUDE_DIR
    NAMES jwt-cpp/jwt.h
    PATHS "${VCPKG_INCLUDE_DIR}"
    REQUIRED
)

# Выводим найденные пути для отладки
message(STATUS "LIBPQXX_INCLUDE_DIR: ${LIBPQXX_INCLUDE_DIR}")
message(STATUS "LIBPQXX_LIBRARY: ${LIBPQXX_LIBRARY}")
message(STATUS "JWT_CPP_INCLUDE_DIR: ${JWT_CPP_INCLUDE_DIR}")

# Добавляем директории с исходниками
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Создаем структуру директорий, если их нет
if(NOT EXISTS ${SOURCE_DIR})
    file(MAKE_DIRECTORY ${SOURCE_DIR})
endif()

# Собираем все .cpp файлы
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")

# Если нет исходников, создаем минимальный main.cpp
if(NOT SOURCES)
    file(WRITE "${SOURCE_DIR}/main.cpp" 
        "#include <drogon/drogon.h>\n"
        "#include <iostream>\n\n"
        "int main() {\n"
        "    std::cout << \"WTLD Backend Server Starting...\" << std::endl;\n"
        "    drogon::app()\n"
        "        .setLogPath(\"./logs\")\n"
        "        .setLogLevel(trantor::Logger::kInfo)\n"
        "        .addListener(\"0.0.0.0\", 8080)\n"
        "        .run();\n"
        "    return 0;\n"
        "}\n")
    file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")
endif()

# Создаем исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES})

# Добавляем include директории
target_include_directories(${PROJECT_NAME} 
    PRIVATE
    ${INCLUDE_DIR}
    ${LIBPQXX_INCLUDE_DIR}
    ${JWT_CPP_INCLUDE_DIR}
)

# Линковка библиотек
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Drogon::Drogon
    ${LIBPQXX_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    Threads::Threads
)

# Добавляем флаги компиляции для MSVC
target_compile_options(${PROJECT_NAME} PRIVATE
    /W4
    /EHsc
    /utf-8
    /std:c++17
)

# Для Windows - копируем DLL
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VCPKG_BIN_DIR}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying vcpkg DLLs..."
    )
endif()

message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "========================================")