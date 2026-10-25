cmake_minimum_required(VERSION 3.16)
project(dontfall-app VERSION 0.3.0 LANGUAGES CXX)

find_package(rclcpp REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(SDL2 REQUIRED)
# OpenGL ES 2.0 (ST's gcnano on the board); the headers come with SDL2
find_library(GLESV2_LIBRARY GLESv2 REQUIRED)

add_executable(dontfall-app
    src/main.cpp
    src/m4_link.cpp
    src/sysfs_led.cpp
)
target_link_libraries(dontfall-app PRIVATE rclcpp::rclcpp ${diagnostic_msgs_TARGETS})
target_compile_features(dontfall-app PRIVATE cxx_std_17)
target_compile_options(dontfall-app PRIVATE -Wall -Wextra)
target_compile_definitions(dontfall-app PRIVATE APP_VERSION="${PROJECT_VERSION}")

# The robot's eyes on the DSI panel: SDL2 (KMS/DRM) + OpenGL ES 2.0.
add_executable(dontfall-face
    src/face/idle.cpp
    src/face/main.cpp
    src/face/renderer.cpp
)
target_link_libraries(dontfall-face PRIVATE SDL2::SDL2 ${GLESV2_LIBRARY})
target_compile_features(dontfall-face PRIVATE cxx_std_17)
target_compile_options(dontfall-face PRIVATE -Wall -Wextra)
target_compile_definitions(dontfall-face PRIVATE APP_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
install(TARGETS dontfall-app dontfall-face RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
