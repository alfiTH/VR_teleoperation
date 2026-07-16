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
    utils/DataRecord.cpp
    utils/CloudCompressor.h
)

# Incluir directorios
target_include_directories(Middleware PUBLIC
    include
    utils
    src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Opciones de compilación
target_compile_options(Middleware PRIVATE 
    -fmax-errors=1 
    -fno-char8_t 
    -O3 
    -std=c++26 
    -fopenmp

)
#    -flto=auto    -fno-plt     

set(ICE_LIBRARIES /usr/lib/x86_64-linux-gnu/libIce++11.so /usr/lib/x86_64-linux-gnu/libIceStorm++11.so)

message(STATUS "ICE_LIBRARIES = ${ICE_LIBRARIES}")

# Enlazar con librerías de Ice
target_link_libraries(Middleware PRIVATE ${ICE_LIBRARIES} ${LIBS})

#Seleccion de Robocomp, P3BOT
option(P3BOT "Enable P3BOT" ON)
option(ROBOCOMP "Enable ROBOCOMP" ON)

if(P3BOT)
  message(STATUS "Building with P3BOT support")
  target_compile_definitions(Middleware PRIVATE P3BOT)
else()
  message(STATUS "Building without P3BOT support")
endif()

if(ROBOCOMP)
  message(STATUS "Building with ROBOCOMP support")
  target_compile_definitions(Middleware PRIVATE ROBOCOMP)
  target_sources(Middleware PRIVATE
    src/RobocompMiddlewareParsers.cpp
    src/RobocompMiddleware.tpp
    include/VRController.cpp
    include/Lidar3D.cpp
    include/KinovaArm.cpp
    include/OmniRobot.cpp
    include/GenericBase.cpp
    include/Navigator.cpp
)
endif()

# RPATH para que el loader encuentre Ice al cargar el .so
set_target_properties(Middleware PROPERTIES
    OUTPUT_NAME "RobotMiddleware"
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN:/usr/lib/x86_64-linux-gnu"
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
