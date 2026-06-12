cmake_minimum_required(VERSION 3.15)
project(RobotMiddlewareTests LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── GoogleTest via FetchContent ──────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── ICE ──────────────────────────────────────────────────────────────
add_definitions("-DICE_CPP11_MAPPING")
find_package(Ice REQUIRED COMPONENTS Ice IceStorm OPTIONAL_COMPONENTS IceUtil)

# ── OpenMP ───────────────────────────────────────────────────────────
find_package(OpenMP REQUIRED)

# ── RobotMiddleware como librería estática ───────────────────────────
set(LIB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_library(MiddlewareStatic STATIC
    ${LIB_SRC_DIR}/src/RobotMiddleware.cpp
    ${LIB_SRC_DIR}/src/RobocompMiddlewareParsers.cpp
    ${LIB_SRC_DIR}/src/RobocompMiddleware.tpp
    ${LIB_SRC_DIR}/include/VRController.cpp
    ${LIB_SRC_DIR}/include/Lidar3D.cpp
    ${LIB_SRC_DIR}/include/KinovaArm.cpp
    ${LIB_SRC_DIR}/include/OmniRobot.cpp
    ${LIB_SRC_DIR}/include/GenericBase.cpp
    ${LIB_SRC_DIR}/include/Navigator.cpp
    ${LIB_SRC_DIR}/utils/DataRecord.cpp
    ${LIB_SRC_DIR}/utils/CloudCompressor.h
)

target_include_directories(MiddlewareStatic PUBLIC
    ${LIB_SRC_DIR}
    ${LIB_SRC_DIR}/include
    ${LIB_SRC_DIR}/utils
    ${LIB_SRC_DIR}/src
)

target_compile_options(MiddlewareStatic PRIVATE -fmax-errors=5 -fno-char8_t -O3 -g)

set(ICE_LIBRARIES
    /usr/lib/x86_64-linux-gnu/libIce++11.so
    /usr/lib/x86_64-linux-gnu/libIceStorm++11.so
)

target_link_libraries(MiddlewareStatic PUBLIC ${ICE_LIBRARIES} zstd pthread)

target_compile_options(MiddlewareStatic PRIVATE 
    -fmax-errors=1 
    -fno-char8_t 
    -O3 
    -std=c++26 
    -flto=auto 
    -fno-plt 
    -fopenmp
)

#Seleccion de Robocomp, P3BOT
option(P3BOT "Enable P3BOT" ON)
option(ROBOCOMP "Enable ROBOCOMP" ON)

if(P3BOT)
  message(STATUS "Building with P3BOT support")
  target_compile_definitions(MiddlewareStatic PRIVATE P3BOT)
endif()

if(ROBOCOMP)
  message(STATUS "Building with ROBOCOMP support")
  target_compile_definitions(MiddlewareStatic PRIVATE ROBOCOMP)
endif()



if(OpenMP_CXX_FOUND)
    target_link_libraries(MiddlewareStatic PUBLIC OpenMP::OpenMP_CXX)
endif()

# ── Ejecutable de tests ──────────────────────────────────────────────
add_executable(RobotMiddlewareTests
    test_robot_middleware.cpp
)

target_link_libraries(RobotMiddlewareTests PRIVATE
    MiddlewareStatic
    GTest::gtest_main
)

target_compile_options(RobotMiddlewareTests PRIVATE 
    -fmax-errors=1 
    -fno-char8_t 
    -O3 
    -std=c++26 
    -flto=auto 
    -fno-plt 
    -fopenmp
)

include(GoogleTest)
gtest_discover_tests(RobotMiddlewareTests)
