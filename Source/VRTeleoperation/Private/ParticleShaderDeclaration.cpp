#include "ParticleShaderDeclaration.h"

// Registramos los shaders con el compilador de Unreal
IMPLEMENT_GLOBAL_SHADER(FParticleShaderVS, "/Shaders/ParticleShader.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FParticleShaderPS, "/Shaders/ParticleShader.usf", "MainPS", SF_Pixel);
