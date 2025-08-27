cmake_minimum_required(VERSION 3.15)
project(Middleware)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # importante para librerías compartidas (.so)

# Buscar Ice y componentes
ADD_DEFINITIONS ("-DICE_CPP11_MAPPING")
FIND_PACKAGE( Ice REQUIRED COMPONENTS Ice IceStorm OPTIONAL_COMPONENTS IceUtil )



# Crear la librería compartida
add_library(Middleware SHARED
    RobotMiddleware.cpp
    VRController.cpp
    Lidar3D.cpp
)

# Incluir directorios
target_include_directories(Middleware PUBLIC
    include
    ${CMAKE_CURRENT_SOURCE_DIR}  # donde está VRController.h
    ${ICE_INCLUDE_DIRS}
)

# Opciones de compilación
target_compile_options(Middleware PRIVATE -fmax-errors=1 -fno-char8_t -O3)

set(ICE_LIBRARIES /usr/lib/x86_64-linux-gnu/libIce++11.so /usr/lib/x86_64-linux-gnu/libIceStorm++11.so)

target_link_libraries(Middleware PRIVATE ${ICE_LIBRARIES})

message(STATUS "ICE_LIBRARIES = ${ICE_LIBRARIES}")

# Enlazar con librerías de Ice
target_link_libraries(Middleware PRIVATE ${ICE_LIBRARIES})

# RPATH para que el loader encuentre Ice al cargar el .so
set_target_properties(Middleware PROPERTIES
    OUTPUT_NAME "RobotMiddleware"
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN:/usr/lib/x86_64-linux-gnu"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
