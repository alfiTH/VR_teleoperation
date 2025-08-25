// Fill out your copyright notice in the Description page of Project Settings.
#include "Robot.h"

// Sets default values
ARobot::ARobot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	middleware.initIce();



    // Camera (HMD)
    VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRCamera"));
    VRCamera->SetupAttachment(RootComponent);

    // Motion controllers
    LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
    LeftController->SetupAttachment(RootComponent);
    LeftController->SetTrackingSource(EControllerHand::Left);

    RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
    RightController->SetupAttachment(RootComponent);
    RightController->SetTrackingSource(EControllerHand::Right);
}

// Called when the game starts or when spawned
void ARobot::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARobot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (GEngine)
		return 
	
	
	auto VecToStr2 = [](const FVector& V) {
		return FString::Printf(TEXT("X=%.2f, Y=%.2f, Z=%.2f"), V.X, V.Y, V.Z);
	};

	auto QuatToStr2 = [](const FQuat& Q) {
		return FString::Printf(TEXT("X=%.5f, Y=%.5f, Z=%.5f, W=%.5f"), Q.X, Q.Y, Q.Z, Q.W);
	};

	FVector HMDPos = VRCamera->GetComponentLocation();
	FQuat HMDQuat = VRCamera->GetComponentQuat();

	FVector LeftPos = LeftController->GetComponentLocation();
	FQuat LeftQuat = LeftController->GetComponentQuat();

	FVector RightPos = RightController->GetComponentLocation();
	FQuat RightQuat = RightController->GetComponentQuat();


	middleware.sendPose(Pose{HMDPos.X, HMDPos.Y, HMDPos.Z, HMDQuat.X, HMDQuat.Y, HMDQuat.Z},
		Pose{HMDPos.X, HMDPos.Y, HMDPos.Z, HMDQuat.X, HMDQuat.Y, HMDQuat.Z},
		Pose{HMDPos.X, HMDPos.Y, HMDPos.Z, HMDQuat.X, HMDQuat.Y, HMDQuat.Z}
	);


	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Black,
		FString::Printf(TEXT("HMD: %s\n%s"), *VecToStr2(HMDPos), *QuatToStr2(HMDQuat)), true, FVector2D(2.5, 2.5));

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
		FString::Printf(TEXT("Left: %s\n%s"), *VecToStr2(LeftPos), *QuatToStr2(LeftQuat)), true, FVector2D(2.5, 2.5));

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue,
		FString::Printf(TEXT("Right: %s\n%s"), *VecToStr2(RightPos), *QuatToStr2(RightQuat)), true, FVector2D(2.5, 2.5));

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange,
		FString::Printf(TEXT("Period: %.3f"), DeltaTime), true, FVector2D(2.5, 2.5));

		// Left controller
	DrawDebugCoordinateSystem(
		GetWorld(),
		LeftPos,                   // Origen
		LeftQuat.Rotator(),        // Orientación
		10.0f,                     // Longitud de los ejes
		false,                     // Persistente
		-1.f,                      // Tiempo de vida (segundos)
		0,                         // DepthPriority
		1.f                       // Grosor
	);

	// Right controller
	DrawDebugCoordinateSystem(
		GetWorld(),
		RightPos,
		RightQuat.Rotator(),
		10.0f,
		false,
		-1.f,
		0,
		1.f
	);
}


// Called to bind functionality to input
void ARobot::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green, TEXT("Hola desde SetupPlayerInputComponent"));

	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

