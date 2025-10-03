#include "ParticleActor.h"
#include "ParticleShaderDeclaration.h"
#include "RHI.h"
#include "RHIResources.h"
#include "RenderResource.h"
#include "Engine/World.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "GlobalRenderResources.h" // para GTileVertexDeclaration

AParticleActor::AParticleActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AParticleActor::BeginPlay()
{
    Super::BeginPlay();

    GenerateRandomParticles(NumParticles);

    // Encolar en el render thread (UE 5.6 usa RDG)
    ENQUEUE_RENDER_COMMAND(FUploadParticles)(
        [Particles = ParticleArray](FRHICommandListImmediate& RHICmdList)
        {
            FRDGBuilder GraphBuilder(RHICmdList);

            // Crear StructuredBuffer en GPU
            FRDGBufferRef ParticleBuffer = CreateStructuredBuffer(
                GraphBuilder,
                TEXT("ParticleBuffer"),
                Particles
            );

            // Crear SRV
            FRDGBufferSRVRef ParticleSRV = GraphBuilder.CreateSRV(ParticleBuffer);

            // ⚠️ Aquí ya puedes pasar ParticleSRV a tus shaders

            FParticleShaderVS::FParameters* PassParameters = GraphBuilder.AllocParameters<FParticleShaderVS::FParameters>();
            PassParameters->ParticleBuffer = GraphBuilder.CreateSRV(ParticleBuffer);

            // Registrar un pass que dibuje puntos
            GraphBuilder.AddPass(
                RDG_EVENT_NAME("DrawParticles"),
                PassParameters,
                ERDGPassFlags::Raster,
                [PassParameters, Count = Particles.Num()](FRHICommandList& RHICmdList)
                {
                    TShaderMapRef<FParticleShaderVS> VertexShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
                    TShaderMapRef<FParticleShaderPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

                    FGraphicsPipelineStateInitializer PSOInit;
                    RHICmdList.ApplyCachedRenderTargets(PSOInit);

                    PSOInit.BoundShaderState.VertexDeclarationRHI = GTileVertexDeclaration.VertexDeclarationRHI;
                    PSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
                    PSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
                    PSOInit.PrimitiveType = PT_PointList;

                    SetGraphicsPipelineState(RHICmdList, PSOInit, 0);
                    SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), *PassParameters);

                    RHICmdList.DrawPrimitive(0, Count, 1);
                }
            );

            GraphBuilder.Execute();
        }
    );
}


void AParticleActor::GenerateRandomParticles(int32 Num)
{
    ParticleArray.Empty(Num);

    for (int32 i = 0; i < Num; i++)
    {
        FParticle P;
        P.Position = FVector(
            FMath::FRandRange(-10.f, 10.f),
            FMath::FRandRange(-10.f, 10.f),
            FMath::FRandRange(-5000.f, 5000.f)
        );

        P.Color = FLinearColor::MakeRandomColor();

        ParticleArray.Add(P);
    }
}

void AParticleActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}
