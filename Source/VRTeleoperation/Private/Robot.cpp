// Fill out your copyright notice in the Description page of Project Settings.
#include "Robot.h"

const float TOLERANCE_FLOAT = 0.01f;

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
		Input->BindAction(IA_Hand_Grasp_Left, ETriggerEvent::Completed, this, &ARobot::GraspReleaseLeft);
		Input->BindAction(IA_Hand_Grasp_Right, ETriggerEvent::Triggered, this, &ARobot::GraspRight);
		Input->BindAction(IA_Hand_Grasp_Right, ETriggerEvent::Completed, this, &ARobot::GraspReleaseRight);
		Input->BindAction(IA_Hand_IndexCurl_Left, ETriggerEvent::Triggered, this, &ARobot::TriggerLeft);
		Input->BindAction(IA_Hand_IndexCurl_Left, ETriggerEvent::Completed, this, &ARobot::TriggerReleaseLeft);
		Input->BindAction(IA_Hand_IndexCurl_Right, ETriggerEvent::Triggered, this, &ARobot::TriggerRight);
		Input->BindAction(IA_Hand_IndexCurl_Right, ETriggerEvent::Completed, this, &ARobot::TriggerReleaseRight);
		Input->BindAction(IA_Hand_A, ETriggerEvent::Started, this, &ARobot::PushA);
		Input->BindAction(IA_Hand_A, ETriggerEvent::Completed, this, &ARobot::ReleaseA);
		Input->BindAction(IA_Hand_B, ETriggerEvent::Started, this, &ARobot::PushB);
		Input->BindAction(IA_Hand_B, ETriggerEvent::Completed, this, &ARobot::ReleaseB);
		Input->BindAction(IA_Hand_X, ETriggerEvent::Started, this, &ARobot::PushX);
		Input->BindAction(IA_Hand_X, ETriggerEvent::Completed, this, &ARobot::ReleaseX);
		Input->BindAction(IA_Hand_Y, ETriggerEvent::Started, this, &ARobot::PushY);
		Input->BindAction(IA_Hand_Y, ETriggerEvent::Completed, this, &ARobot::ReleaseY);
		Input->BindAction(IA_Hand_Thumbstick_Left, ETriggerEvent::Triggered, this, &ARobot::ThumbStickLeft);
		Input->BindAction(IA_Hand_Thumbstick_Left, ETriggerEvent::Completed, this, &ARobot::ThumbStickReleaseLeft);
		Input->BindAction(IA_Hand_Thumbstick_Right, ETriggerEvent::Triggered, this, &ARobot::ThumbStickRight);
		Input->BindAction(IA_Hand_Thumbstick_Right, ETriggerEvent::Completed, this, &ARobot::ThumbStickReleaseRight);

}

#pragma region Inputs
void ARobot::GraspLeft(const FInputActionValue& Value){
	left.grab = Value.Get<float>();
	controllerChanged = true;
	
};
void ARobot::GraspReleaseLeft(const FInputActionValue& Value){
	left.grab = 0;
	controllerChanged = true;
	
};
void ARobot::GraspRight(const FInputActionValue& Value){
	right.grab =  Value.Get<float>();
	controllerChanged = true;
	
};

void ARobot::GraspReleaseRight(const FInputActionValue& Value){
	right.grab = 0;
	controllerChanged = true;
	
};
void ARobot::TriggerLeft(const FInputActionValue& Value){
	left.trigger = Value.Get<float>();
	controllerChanged = true;
};
void ARobot::TriggerReleaseLeft(const FInputActionValue& Value){
	left.trigger = 0;
	controllerChanged = true;
};
void ARobot::TriggerRight(const FInputActionValue& Value){
	right.trigger = Value.Get<float>();
	controllerChanged = true;

};
void ARobot::TriggerReleaseRight(const FInputActionValue& Value){
	right.trigger = 0;
	controllerChanged = true;

};
#pragma region Button
void ARobot::PushA(const FInputActionValue& Value){
	right.aButton= true;
	controllerChanged = true;
};
void ARobot::ReleaseA(const FInputActionValue& Value){
	right.aButton= false;
	controllerChanged = true;
};
void ARobot::PushB(const FInputActionValue& Value){
	right.bButton= true;
	controllerChanged = true;
};
void ARobot::ReleaseB(const FInputActionValue& Value){
	right.bButton= false;
	controllerChanged = true;
};
void ARobot::PushX(const FInputActionValue& Value){
	left.aButton= true;
	controllerChanged = true;
};
void ARobot::ReleaseX(const FInputActionValue& Value){
	left.aButton= false;
	controllerChanged = true;
};
void ARobot::PushY(const FInputActionValue& Value){
	left.bButton= true;
	controllerChanged = true;
};
void ARobot::ReleaseY(const FInputActionValue& Value){
	left.bButton= false;
	controllerChanged = true;
};
#pragma endregion
void ARobot::ThumbStickLeft(const FInputActionValue& Value)
{
    FVector2D StickValue = Value.Get<FVector2D>();
	left.x = StickValue.X;
	left.y = StickValue.Y;

    controllerChanged = true;
}
void ARobot::ThumbStickReleaseLeft(const FInputActionValue& Value)
{
	left.x = 0;
    left.y = 0;
    controllerChanged = true;

}
void ARobot::ThumbStickRight(const FInputActionValue& Value)
{
    FVector2D StickValue = Value.Get<FVector2D>();
	right.x = StickValue.X;
	right.y = StickValue.Y;
	controllerChanged = true;
}
void ARobot::ThumbStickReleaseRight(const FInputActionValue& Value)
{
	right.x = 0;
	right.y = 0;
	controllerChanged = true;
}
#pragma endregion


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

