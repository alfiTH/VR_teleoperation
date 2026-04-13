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
		// 1. Conversión de metros a centímetros (si tu middleware usa metros)
		FVector Pos = FVector(pose.x, pose.y, pose.z);
    
		// 2. Crear el Cuaternión explícitamente y normalizarlo
		// El constructor es FQuat(x, y, z, w)
		FQuat Rot = FQuat(pose.qrx, pose.qry, pose.qrz, pose.qrw);
		Rot.Normalize(); 

		// 3. Aplicar movimiento
		SetActorLocationAndRotation(Pos, Rot);

		// 4. Printado por terminal (Output Log)
		UE_LOG(LogTemp, Log, TEXT("Robot Pose -> Pos: %s | Rot: %s"), *Pos.ToString(), *Rot.ToString());
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo obtener la pose del middleware"));
	}
}

