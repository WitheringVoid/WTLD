cmake_minimum_required(VERSION 3.15)
project(WTLD_Backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Автоматическое определение vcpkg
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE PATH "vcpkg toolchain")
elseif(EXISTS "D:/VS_Code_projects/sources/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "D:/VS_Code_projects/sources/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE PATH "vcpkg toolchain")
endif()

# Поиск зависимостей
find_package(Drogon CONFIG REQUIRED)
find_package(Pgxx REQUIRED)
find_package(jwt-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Добавление исходников
include_directories(include)
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Линковка библиотек
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Drogon::Drogon
    Pgxx::Pgxx
    jwt-cpp::jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Копирование .dll файлов в папку сборки (для Windows)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_PREFIX_PATH}/bin"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Копирование DLL файлов..."
    )
endif()