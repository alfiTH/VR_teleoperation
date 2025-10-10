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
		ParticlePositions.SetNumUninitialized(NumPoints);
		ParticleColors.SetNumUninitialized(NumPoints);
		if (!NiagaraComp)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ Niagara Component not assigned in %s"), *GetOwner()->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Editor mode"));
	}
	// ...
	
}



// Called every frame
void UPointCloudComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GEngine or !GetWorld() or !GetWorld()->IsGameWorld())
		return ;

	if (!NiagaraComp or !NiagaraComp->IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ Niagara Component does not activate"));
		return ;
	}
	
	double StartTime = FPlatformTime::Seconds();  
	ParallelFor( NumPoints, [&](int32 i)
	{
		ParticlePositions[i][0] = 200;
		ParticlePositions[i][1] = 0;
		ParticlePositions[i][2] =  100;
		// ParticlePositions[i][1] = FMath::FRandRange(-200.0, 200.0);
		// ParticlePositions[i][2] =  FMath::FRandRange(10.0, 200.0);
		ParticleColors[i].R = 0.5; // entre 0.0 y 1.0
		ParticleColors[i].G = 0.5;
		ParticleColors[i].B = 0.5;
		ParticleColors[i].A = 1.0f;
	},EParallelForFlags::BackgroundPriority);
	
	double EndTime = FPlatformTime::Seconds(); 
	double DurationMs = (EndTime - StartTime) * 1000.0;

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

	NiagaraComp->SetNiagaraVariableInt(FString("User.NumPoints"), NumPoints);


}
	


