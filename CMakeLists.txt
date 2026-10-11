cmake_minimum_required(VERSION 3.16)
project(dontfall-app VERSION 0.1.0 LANGUAGES CXX)

add_executable(dontfall-app src/main.cpp)
target_compile_features(dontfall-app PRIVATE cxx_std_17)
target_compile_definitions(dontfall-app PRIVATE APP_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
install(TARGETS dontfall-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
