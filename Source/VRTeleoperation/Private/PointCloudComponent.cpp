// Fill out your copyright notice in the Description page of Project Settings.


#include "PointCloudComponent.h"
#include <cstdlib>

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
			NiagaraComp->AddTickPrerequisiteComponent(this);
			FNiagaraParameterStore& Params = NiagaraComp->GetOverrideParameters();
			FNiagaraVariableBase Var(FNiagaraTypeDefinition::GetIntDef(), FName("User.NumPoints"));
			NumPoints = Params.GetParameterValue<int32>(Var);
			PointCapacity = NumPoints;
			UE_LOG(LogTemp, Warning, TEXT("Niagara::PointCapacity: %d"), PointCapacity);

			// SetNumZeroed (no SetNumUninitialized): así todo el array arranca ya en (0,0,0),
			// que es el invariante que TickComponent mantiene después solo tocando el delta.
			ParticlePositions.SetNumZeroed(PointCapacity);
			ParticleColors.SetNumZeroed(PointCapacity);
			PrevNumPoints = 0;
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

	{
		RobotMiddleware::ColorCloudDataGuard guard = middleware.lockColorCloudData();
		if (!guard.valid()) return;
		const RobotMiddleware::ColorCloudData& cloud = guard.get();
		if (cloud.X.empty()) {
			// UE_LOG(LogTemp, Warning, TEXT("⚠️cloud.X.empty()"));
			guard.unlock();
			return; }
		const uint64 CurrentVersion = guard.getVersion();
		if (CurrentVersion == lastCloudVersion)
		{
			guard.unlock();
			return;
		}
		lastCloudVersion = CurrentVersion;

		auto SRGBToLinear = [](float c) {
			return (c <= 0.04045f) ? (c / 12.92f) : FMath::Pow((c + 0.055f) / 1.055f, 2.4f);
		};

		// Clamp por seguridad: nunca escribir más allá de la capacidad instanciada en Niagara.
		NumPoints = FMath::Min(static_cast<int32>(cloud.X.size()), PointCapacity);

		ParallelFor(NumPoints, [&](int32 i)
		{
			ParticlePositions[i][0] = cloud.Y[i] / 10.0f;
			ParticlePositions[i][1] = cloud.X[i] / 10.0f;
			ParticlePositions[i][2] = cloud.Z[i] / 10.0f;
			ParticleColors[i].R = SRGBToLinear(cloud.R[i] / 255.0f);
			ParticleColors[i].G = SRGBToLinear(cloud.G[i] / 255.0f);
			ParticleColors[i].B = SRGBToLinear(cloud.B[i] / 255.0f);
		});

		// Los puntos [0, PointCapacity) fuera de [0, NumPoints) ya están garantizados a 0
		// desde el frame anterior (invariante). Si este frame tiene MENOS puntos válidos
		// que el anterior, solo hay que relocar a (0,0,0) la diferencia; si tiene más o
		// igual, no hace falta tocar nada más: ya estaban a cero y se acaban de sobrescribir.
		if (NumPoints < PrevNumPoints && std::abs(PrevNumPoints - NumPoints) > 10000)
		{
			const int32 ClearCount = PrevNumPoints - NumPoints;
			FMemory::Memzero(ParticlePositions.GetData() + NumPoints, ClearCount * sizeof(FVector));
			FMemory::Memzero(ParticleColors.GetData() + NumPoints, ClearCount * sizeof(FLinearColor));
		}
		PrevNumPoints = NumPoints;



		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
			NiagaraComp,
			FName("User.ParticlePositions"),
			ParticlePositions
		);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(
			NiagaraComp,
			FName("User.ParticleColors"),
			ParticleColors
		);
		NiagaraComp->SetVariableInt(FName("User.NumPoints"), NumPoints);
		guard.unlock();
	}
}
	


