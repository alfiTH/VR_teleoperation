// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PointCloudPLYActor.generated.h"

UCLASS(Blueprintable)
class VRTELEOPERATION_API APointCloudPLYActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APointCloudPLYActor();

	// Ruta relativa del pointcloud
	UPROPERTY(EditAnywhere, Category = "PointCloud", meta = (FilePathFilter = "PLY files (*.ply)|*.ply"))
	FFilePath PLYFile;

	// Cargar desde BeginPlay
	UPROPERTY(EditAnywhere, Category="PointCloud")
	bool bLoadOnBeginPlay = false;

	// Máximo de puntos a dibujar con DrawDebug
	UPROPERTY(EditAnywhere, Category="PointCloud")
	int32 MaxDebugDrawPoints = 2000;

	UPROPERTY(BlueprintReadOnly, Category="PointCloud")
	float DebugPointSize = 4.0f;

	// Resultado del parseo
	UPROPERTY(BlueprintReadOnly, Category = "PointCloud")
	TArray<FVector> Positions;
	UPROPERTY(BlueprintReadOnly, Category = "PointCloud")
	TArray<FLinearColor> Colors;

	// Lanza la carga
	UFUNCTION(CallInEditor, Category="PointCloud")
	void LoadPLY();
	UFUNCTION(CallInEditor, Category="PointCloud")
	void DrawPoints();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	bool ParseASCIIPLY(const FString& FullPath, TArray<FVector>& OutPosition, TArray<FLinearColor>& OutColors, FString& OutError);

};
