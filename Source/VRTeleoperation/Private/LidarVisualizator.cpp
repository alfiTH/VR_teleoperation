// Fill out your copyright notice in the Description page of Project Settings.


#include "LidarVisualizator.h"

// Sets default values for this component's properties
ULidarVisualizator::ULidarVisualizator()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULidarVisualizator::BeginPlay()
{
	Super::BeginPlay();

	GenerateWall();
}


void ULidarVisualizator::DrawPointCloud()
{
	if (GetWorld())
	{
		for (const FVector& Point : PointCloud)
		{
			// Dibuja un punto en cada posición de la nube de puntos
			DrawDebugPoint(GetWorld(), Point, 10.0f, FColor::Red, false, -1.0f, 0);
		}
	}
}

void ULidarVisualizator::GenerateWall()
{
	const float WallStart = 100.0f;
	const float WallEnd = 200.0f;
	const float Step = 10.0f;  // Distancia entre cada punto

	for (float X = WallStart; X <= WallEnd; X += Step)
	{
		FVector WallPoint(X, 0, 0); // Y y Z fijos en 0
		PointCloud.Add(WallPoint);   // Añadir al arreglo de puntos
	}
}

// Called every frame
void ULidarVisualizator::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DrawPointCloud();
	// ...
}

