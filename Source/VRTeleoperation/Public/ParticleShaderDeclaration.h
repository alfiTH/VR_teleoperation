#pragma once

#include "GlobalShader.h"
#include "ParticleActor.h"
#include "ShaderParameterStruct.h"

// Vertex Shader
class FParticleShaderVS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FParticleShaderVS);
	SHADER_USE_PARAMETER_STRUCT(FParticleShaderVS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<FParticle>, ParticleBuffer)
	END_SHADER_PARAMETER_STRUCT()

};

// Pixel Shader
class FParticleShaderPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FParticleShaderPS);
	SHADER_USE_PARAMETER_STRUCT(FParticleShaderPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// aquí podrías pasar constantes si quieres
	END_SHADER_PARAMETER_STRUCT()
};
