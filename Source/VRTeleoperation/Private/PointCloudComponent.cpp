// Fill out your copyright notice in the Description page of Project Settings.


#include "PointCloudComponent.h"

// Sets default values for this component's properties
UPointCloudComponent::UPointCloudComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	// ...
}


// Called when the game starts
void UPointCloudComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetWorld() && GetWorld()->IsGameWorld())
	{

		if (!NiagaraComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ Niagara Component not assigned in %s⚠️"), *GetOwner()->GetName());
		}
		else
		{
			NiagaraComp->SetAutoActivate(true);
			NiagaraComp->Activate(true);
			FNiagaraParameterStore& Params = NiagaraComp->GetOverrideParameters();
			FNiagaraVariableBase Var(FNiagaraTypeDefinition::GetIntDef(), FName("User.NumPoints"));
			NumPoints = Params.GetParameterValue<int32>(Var);
	
			ParticlePositions.SetNumUninitialized(NumPoints);
			ParticleColors.SetNumUninitialized(NumPoints);	
		}
		
		if (!middleware.isRunning() or !NiagaraComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️Middleware or Niagara is not running⚠️"));
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				UKismetSystemLibrary::QuitGame(
					GetWorld(),
					PlayerController,
					EQuitPreference::Quit,
					true // true cierra sin mostrar mensaje de confirmación
				);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Editor mode"));
	}

}

// Called every frame
void UPointCloudComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// UE_LOG(LogTemp, Warning, TEXT("⚠️ Niagara tiking⚠️"));
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GEngine or !GetWorld() or !GetWorld()->IsGameWorld())
		return ;
		
	if (!middleware.isRunning()) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Middleware not initialized in UP3botAnimInstance"));
		return;
	}

	if (!NiagaraComp or !NiagaraComp->IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ Niagara Component does not activate"));
		return ;
	}


	// if (NiagaraComp && NiagaraComp->IsRegistered())
	// {
	// 	bool bIsActive = NiagaraComp->IsActive();
	// 	bool bIsPaused = NiagaraComp->IsPaused();
	// 	ENiagaraTickBehavior TickBehavior = NiagaraComp->GetTickBehavior();
	//
	// 	UE_LOG(LogTemp, Warning, TEXT("Niagara: Active=%d Paused=%d TickBehavior=%d"),
	// 		bIsActive ? 1 : 0,
	// 		bIsPaused ? 1 : 0,
	// 		(int32)TickBehavior);
	// }
	
	double StartTime = FPlatformTime::Seconds();  

	const RobotMiddleware::ColorCloudData& cloud  = middleware.getColorCloudData();

	double EndTime = FPlatformTime::Seconds(); 
	double DurationMs = (EndTime - StartTime) * 1000.0;
	
	// UE_LOG(LogTemp, Display, TEXT("get cloud took %.3f ms for %d points"), DurationMs, cloud.X.size());
	if (cloud.X.size() > 0)
	{
		auto SRGBToLinear = [](float c) {
			return (c <= 0.04045f) ? (c / 12.92f) : FMath::Pow((c + 0.055f) / 1.055f, 2.4f);
		};
		
		StartTime = FPlatformTime::Seconds();
		middleware.lockUlockGetColorCloudData(true);
		NumPoints = cloud.X.size();
		ParallelFor( NumPoints, [&](int32 i)
		{
			ParticlePositions[i][0] = cloud.Y[i]/10.0f;
			ParticlePositions[i][1] = cloud.X[i]/10.0f;
			ParticlePositions[i][2] =  cloud.Z[i]/10.0f;
			ParticleColors[i].R = SRGBToLinear(cloud.R[i]/255.0f); // entre 0.0 y 1.0
			ParticleColors[i].G = SRGBToLinear(cloud.G[i]/255.0f);
			ParticleColors[i].B = SRGBToLinear(cloud.B[i]/255.0f);
		});
		middleware.lockUlockGetColorCloudData(false);
		// UE_LOG(LogTemp, Display, TEXT("R:%f, G:%f B:%f"), ParticleColors[5000].R, ParticleColors[5000].G, ParticleColors[5000].B);;

		
		EndTime = FPlatformTime::Seconds(); 
		DurationMs = (EndTime - StartTime) * 1000.0;
		UE_LOG(LogTemp, Display, TEXT("ParallelFor took %.3f ms for %d points"), DurationMs, NumPoints);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
				NiagaraComp,
				FName("User.ParticlePositions"), // Nombre del parámetro
				ParticlePositions
			);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(
				NiagaraComp,
				FName("User.ParticleColors"), // Nombre del parámetro
				ParticleColors
			);

		NiagaraComp->SetVariableInt(FName("User.NumPoints"), NumPoints);
	}


}
	


