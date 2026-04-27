// Fill out your copyright notice in the Description page of Project Settings.


#include "Kinova.h"

// Sets default values
AKinova::AKinova()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PointCloudComponent = CreateDefaultSubobject<UPointCloudComponent>(TEXT("PointCloudComponent"));

}

// Called when the game starts or when spawned
void AKinova::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKinova::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

