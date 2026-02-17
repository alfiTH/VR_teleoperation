// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include  "RobotMiddleware.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "NiagaraParameterCollection.h"
#include "NiagaraParameterStore.h"

#include "Components/ActorComponent.h"


#include <vector>
#include <random>

#include "PointCloudComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VRTELEOPERATION_API UPointCloudComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPointCloudComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;


public:
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PointCloud")
	TArray<FVector> ParticlePositions;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PointCloud")
	TArray<FLinearColor> ParticleColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PointCloud")
	int32 NumPoints; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PointCloud")
	UNiagaraComponent* NiagaraComp = nullptr;

private:
	RobotMiddleware& middleware = RobotMiddleware::getInstance();
};
