cmake_minimum_required(VERSION 3.16)
project(quan-ly-san LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ---- Thu vien dung chung: models + core (storage, manager, utils) ----
add_library(core STATIC
    src/core/storage.cpp
    src/core/manager.cpp
    src/core/utils.cpp
    src/core/paths.cpp
    src/models/court.cpp
    src/models/booking.cpp
)
# include/ : header cua du an;  third_party/ : nlohmann/json.hpp (da kem san)
target_include_directories(core PUBLIC include third_party)
if(WIN32)
    target_compile_definitions(core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ---- Chuong trinh 1: giao dien console ----
add_executable(app src/main.cpp)
target_link_libraries(app PRIVATE core)

# ---- Chuong trinh 2: giao dien web (mo bang trinh duyet) ----
add_executable(app_web src/server.cpp src/web/http_server.cpp)
target_link_libraries(app_web PRIVATE core Threads::Threads)
if(WIN32 AND NOT MSVC)
    target_link_libraries(app_web PRIVATE ws2_32)
endif()
