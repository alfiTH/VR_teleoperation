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
			UE_LOG(LogTemp, Warning, TEXT("⚠️ Niagara Component not assigned in %s"), *GetOwner()->GetName());
		}
		middleware = &RobotMiddlewareSingleton::Get();
		if (!middleware->isRunning() or !NiagaraComp)
		{
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
	ParticlePositions.SetNumUninitialized(MAX_POINT_CLOUD);
	ParticleColors.SetNumUninitialized(MAX_POINT_CLOUD);	
}



// Called every frame
void UPointCloudComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!GEngine or !GetWorld() or !GetWorld()->IsGameWorld())
		return ;
		
	if (!middleware) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Middleware not initialized in UP3botAnimInstance"));
		return;
	}

	if (!NiagaraComp or !NiagaraComp->IsActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ Niagara Component does not activate"));
		return ;
	}
	double StartTime = FPlatformTime::Seconds();  

	RobotMiddleware::ColorCloudData cloud  = middleware->getColorCloudData();

	double EndTime = FPlatformTime::Seconds(); 
	double DurationMs = (EndTime - StartTime) * 1000.0;
	
	UE_LOG(LogTemp, Display, TEXT("get cloud took %.3f ms for %d points"), DurationMs, cloud.X.size());
	
	StartTime = FPlatformTime::Seconds();  
	ParallelFor( NumPoints, [&](int32 i)
	{
		ParticlePositions[i][0] = cloud.Y[i]/10.0;
		ParticlePositions[i][1] = cloud.X[i]/10.0;
		ParticlePositions[i][2] =  cloud.Z[i]/10.0;
		ParticleColors[i].R = cloud.R[i]/255.0; // entre 0.0 y 1.0
		ParticleColors[i].G = cloud.G[i]/255.0;
		ParticleColors[i].B = cloud.B[i]/255.0;
	});

	NumPoints = cloud.X.size();
	
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

	NiagaraComp->SetNiagaraVariableInt(FString("User.NumPoints"), NumPoints);


}
	


