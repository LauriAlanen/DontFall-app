cmake_minimum_required(VERSION 3.16)
project(dontfall-app VERSION 0.1.0 LANGUAGES CXX)

add_executable(dontfall-app src/main.cpp)
target_compile_features(dontfall-app PRIVATE cxx_std_17)
target_compile_definitions(dontfall-app PRIVATE APP_VERSION="${PROJECT_VERSION}")

include(GNUInstallDirs)
install(TARGETS dontfall-app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME rpmsg_hello
             COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_rpmsg_hello.py
                     $<TARGET_FILE:dontfall-app>)
endif()
