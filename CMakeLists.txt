cmake_minimum_required(VERSION 3.16)
project(dontfall-app VERSION 0.3.0 LANGUAGES CXX)

find_package(rclcpp REQUIRED)
find_package(diagnostic_msgs REQUIRED)

add_executable(dontfall-app
    src/main.cpp
    src/m4_link.cpp
    src/sysfs_led.cpp
)
target_link_libraries(dontfall-app PRIVATE rclcpp::rclcpp ${diagnostic_msgs_TARGETS})
target_compile_features(dontfall-app PRIVATE cxx_std_17)
target_compile_options(dontfall-app PRIVATE -Wall -Wextra)
target_compile_definitions(dontfall-app PRIVATE APP_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
install(TARGETS dontfall-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
