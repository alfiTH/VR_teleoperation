// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PointCloudComponent.h"
#include "RobotMiddleware.h"

#include "P3Bot.generated.h"


UCLASS()
class VRTELEOPERATION_API AP3Bot : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AP3Bot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	RobotMiddleware& middleware = RobotMiddleware::getInstance();
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	UPointCloudComponent* PointCloudComponent;

private:
	RobotMiddleware::Pose lastPose{};
	bool hasPose = false;
};
