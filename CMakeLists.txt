cmake_minimum_required(VERSION 3.16)
project(dontfall-app VERSION 0.2.0 LANGUAGES CXX)

add_executable(dontfall-app
    src/main.cpp
    src/heartbeat.cpp
    src/protocol.cpp
    src/rpmsg_tty.cpp
    src/sysfs_led.cpp
)
target_compile_features(dontfall-app PRIVATE cxx_std_17)
target_compile_options(dontfall-app PRIVATE -Wall -Wextra)
target_compile_definitions(dontfall-app PRIVATE APP_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
install(TARGETS dontfall-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
