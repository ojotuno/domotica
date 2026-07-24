cmake_minimum_required(VERSION 3.16)
project(home_panel_backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Crow CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)

add_executable(server src/main.cpp)
target_link_libraries(server PRIVATE Crow::Crow cpr::cpr)

# Copia config.json junto al ejecutable en cada build
add_custom_command(TARGET server POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/config.json
          $<TARGET_FILE_DIR:server>/config.json)
