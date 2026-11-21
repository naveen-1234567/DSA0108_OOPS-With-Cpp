cmake_minimum_required(VERSION 3.16)
project(CampusEvents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# Header-only libraries expected via include path or vcpkg
# cpp-httplib and jwt-cpp should be available in include path

add_executable(campus_events_server
    src/main.cpp
    src/database.cpp
    src/auth.cpp
    src/events.cpp
    src/registrations.cpp
    src/utils.cpp
)

target_include_directories(campus_events_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SQLite3_INCLUDE_DIRS}
)

target_link_libraries(campus_events_server PRIVATE
    SQLite::SQLite3
    Threads::Threads
)

# Optional: if OpenSSL needed for better JWT/HMAC
find_package(OpenSSL)
if(OpenSSL_FOUND)
    target_link_libraries(campus_events_server PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(campus_events_server PRIVATE USE_OPENSSL)
endif()

install(TARGETS campus_events_server DESTINATION bin)
