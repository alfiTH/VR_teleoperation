// Fill out your copyright notice in the Description page of Project Settings.
#include "Robot.h"

// Sets default values
ARobot::ARobot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	middleware.initIce();
	SetupPoseComponent();



}

// Called when the game starts or when spawned
void ARobot::BeginPlay()
{
	Super::BeginPlay();
}



void ARobot::SetupPoseComponent()
{
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

void ARobot::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		// You can bind to any of the trigger events here by changing the "ETriggerEvent" enum value
		Input->BindAction(IA_Hand_Grasp_Left, ETriggerEvent::Triggered, this, &ARobot::GraspLeft);
		Input->BindAction(IA_Hand_Grasp_Right, ETriggerEvent::Triggered, this, &ARobot::GraspRight);
		Input->BindAction(IA_Hand_IndexCurl_Left, ETriggerEvent::Triggered, this, &ARobot::TriggerLeft);
		Input->BindAction(IA_Hand_IndexCurl_Right, ETriggerEvent::Triggered, this, &ARobot::TriggerRight);
}

void ARobot::GraspLeft(const FInputActionValue& Value){
	float GraspValue = Value.Get<float>();
	if (GraspValue > 0.1f)
	{
		left.grab = GraspValue;
		controllerChanged = true;
	}
	
};
void ARobot::GraspRight(const FInputActionValue& Value){
	float GraspValue = Value.Get<float>();
	if (GraspValue > 0.1f)
	{
		right.grab = GraspValue;
		controllerChanged = true;
	}
	
};
void ARobot::TriggerLeft(const FInputActionValue& Value){
	float TriggerValue = Value.Get<float>();
	if (TriggerValue > 0.1f)
	{	
		left.trigger = TriggerValue;
		controllerChanged = true;
	}
};
void ARobot::TriggerRight(const FInputActionValue& Value){
	float TriggerValue = Value.Get<float>();
	if (TriggerValue > 0.1f)
	{
		right.trigger = TriggerValue;
		controllerChanged = true;
	}
};



void ARobot::OnTriggerPressed(const FInputActionValue& Value)
{
    // Value devuelve un float entre 0 y 1 según la presión del gatillo
    float TriggerValue = Value.Get<float>();
    UE_LOG(LogTemp, Log, TEXT("Trigger Pressed: %f"), TriggerValue);

    if (TriggerValue > 0.1f)
    {
        // Hacer algo, por ejemplo disparar
    }
}

// Called every frame
void ARobot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (!GEngine)
		return ;
	


	FVector HMDPos = VRCamera->GetComponentLocation();
	FRotator HMDRot = VRCamera->GetComponentRotation();

	FVector LeftPos = LeftController->GetComponentLocation();
	FRotator LeftRot = LeftController->GetComponentRotation();

	FVector RightPos = RightController->GetComponentLocation();
	FRotator RightRot = RightController->GetComponentRotation();


	middleware.sendPose(RobotMiddleware::Pose{HMDPos.X, HMDPos.Y, HMDPos.Z, HMDRot.Pitch, HMDRot.Yaw, HMDRot.Roll},
		RobotMiddleware::Pose{LeftPos.X, LeftPos.Y, LeftPos.Z, LeftRot.Pitch, LeftRot.Yaw, LeftRot.Roll},
		RobotMiddleware::Pose{RightPos.X, RightPos.Y, RightPos.Z, RightRot.Pitch, RightRot.Yaw, RightRot.Roll}
	);

	if (controllerChanged)
	{	
		controllerChanged = false;
		middleware.sendControllers(left, right);
	}


	auto VecToStr2 = [](const FVector& V) {
		return FString::Printf(TEXT("X=%.2f, Y=%.2f, Z=%.2f"), V.X, V.Y, V.Z);
	};

	auto RotToStr2 = [](const FRotator& V) {
		return FString::Printf(TEXT("X=%.2f, Y=%.2f, Z=%.2f"), V.Pitch, V.Yaw, V.Roll);
	};


	auto QuatToStr2 = [](const FQuat& Q) {
		return FString::Printf(TEXT("X=%.5f, Y=%.5f, Z=%.5f, W=%.5f"), Q.X, Q.Y, Q.Z, Q.W);
	};

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Black,
		FString::Printf(TEXT("HMD: %s\n%s"), *VecToStr2(HMDPos), *RotToStr2(HMDRot)), true, FVector2D(2.5, 2.5));

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
		FString::Printf(TEXT("Left: %s\n%s"), *VecToStr2(LeftPos), *RotToStr2(LeftRot)), true, FVector2D(2.5, 2.5));

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue,
		FString::Printf(TEXT("Right: %s\n%s"), *VecToStr2(RightPos), *RotToStr2(RightRot)), true, FVector2D(2.5, 2.5));

	GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange,
		FString::Printf(TEXT("Period: %.3f"), DeltaTime), true, FVector2D(2.5, 2.5));

		// Left controller
	DrawDebugCoordinateSystem(
		GetWorld(),
		LeftPos,                   // Origen
		LeftRot,        // Orientación
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
		RightRot,
		10.0f,
		false,
		-1.f,
		0,
		1.f
	);
}

