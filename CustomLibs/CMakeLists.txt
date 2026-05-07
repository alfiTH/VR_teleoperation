cmake_minimum_required(VERSION 3.15)
project(Middleware)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)  

# Buscar Ice y componentes
ADD_DEFINITIONS ("-DICE_CPP11_MAPPING")
FIND_PACKAGE( Ice REQUIRED COMPONENTS Ice IceStorm OPTIONAL_COMPONENTS IceUtil )

find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    LIST(APPEND LIBS ${LIBS} OpenMP::OpenMP_CXX)
endif()


# Crear la librería compartida
add_library(Middleware SHARED
    src/RobotMiddleware.cpp
    src/RobotMiddlewareParsers.cpp
    src/RobotMiddleware.tpp
    include/VRController.cpp
    include/Lidar3D.cpp
    include/KinovaArm.cpp
    include/OmniRobot.cpp
    include/GenericBase.cpp
    utils/DataRecord.cpp

)

# Incluir directorios
target_include_directories(Middleware PUBLIC
    include
    utils
    src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ICE_INCLUDE_DIRS}
)



# Opciones de compilación
target_compile_options(Middleware PRIVATE 
    -fmax-errors=1 
    -fno-char8_t 
    -O3 
    -std=c++26 
    -flto=auto 
    -fno-plt 
    -fopenmp
)

set(ICE_LIBRARIES /usr/lib/x86_64-linux-gnu/libIce++11.so /usr/lib/x86_64-linux-gnu/libIceStorm++11.so)

message(STATUS "ICE_LIBRARIES = ${ICE_LIBRARIES}")

# Enlazar con librerías de Ice
target_link_libraries(Middleware PRIVATE ${ICE_LIBRARIES} ${LIBS})

# RPATH para que el loader encuentre Ice al cargar el .so
set_target_properties(Middleware PROPERTIES
    OUTPUT_NAME "RobotMiddleware"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN:/usr/lib/x86_64-linux-gnu"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
