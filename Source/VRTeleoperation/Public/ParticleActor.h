// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParticleActor.generated.h"

USTRUCT()
struct FParticle
{
	GENERATED_BODY()

	FVector Position;
	FLinearColor Color;
};

UCLASS()
class VRTELEOPERATION_API AParticleActor : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this actor's properties
	AParticleActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void GenerateRandomParticles(int32 Num);

	TArray<FParticle> ParticleArray;
	int32 NumParticles = 100000; // ej: 100k puntos

};
