// Fill out your copyright notice in the Description page of Project Settings.


#include "P3Bot.h"

// Sets default values
AP3Bot::AP3Bot()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PointCloudComponent = CreateDefaultSubobject<UPointCloudComponent>(TEXT("PointCloudComponent"));
}

// Called when the game starts or when spawned
void AP3Bot::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AP3Bot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RobotMiddleware::Pose pose;
	if (middleware.getRobotPose(pose))
	{
		lastPose = pose;
		hasPose = true;
		// UE_LOG(LogTemp, Log, TEXT("Robot Pose -> Pos: (%f, %f, %f)"), pose.x, pose.y, pose.z);
	}

	if (hasPose)
	{
		FVector Pos(lastPose.x, lastPose.y, lastPose.z);
		FQuat Rot(lastPose.qrx, lastPose.qry, lastPose.qrz, lastPose.qrw);
		Rot.Normalize();
		SetActorLocationAndRotation(Pos, Rot);
	}
}

