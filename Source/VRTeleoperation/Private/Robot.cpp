// Fill out your copyright notice in the Description page of Project Settings.
#include "Robot.h"

const float TOLERANCE_FLOAT = 0.01f;

inline void exportFPSQueueToCSV(const std::deque<float>& FPSQueue, 
						 const std::string& filename = "fps_data.csv",
						 const std::string& columnName = "FPS") {
    
	std::ofstream file(filename);
    
	if (!file.is_open()) {
		std::cerr << "Error: No se pudo abrir el archivo " << filename << std::endl;
		return;
	}
	
	file << columnName << "\n";
    
	for (const float& fps : FPSQueue) {
		file << std::fixed << std::setprecision(5) << fps << "\n";
	}
    
	file.close();
    
	std::cout << "Datos exportados exitosamente a " << filename << std::endl;
	std::cout << "Total de registros exportados: " << FPSQueue.size() << std::endl;
}

// Sets default values
ARobot::ARobot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    SetupPoseComponent();
}

// Called when the game starts or when spawned
void ARobot::BeginPlay()
{
	Super::BeginPlay();
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		if (!middleware.isRunning())
		{
			UE_LOG(LogTemp, Warning, TEXT("Middleware is not running"));
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				UKismetSystemLibrary::QuitGame(
					GetWorld(),
					PlayerController,
					EQuitPreference::Quit,
					true // true cierra sin mostrar mensaje de confirmación
				);
			}
		}
		CachedPC = Cast<APlayerController>(GetController());

		if (!CachedPC.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("ARobot::BeginPlay -> Pawn dont has PlayerController"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Editor mode"));
	}
}
void ARobot::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// exportFPSQueueToCSV(FPSQueue, "/home/robolab/fps_data.csv", "FPS");
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

	LeftGriper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftGriper"));
	RightGriper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightGriper"));

}

void ARobot::TriggerHapticFeedback(
	EControllerHand Hand,
	float Intensity /* = 1.0f */,
	float Frequency /* = 0.5f */)
{

	// Obtener PlayerController
	if (!CachedPC.IsValid())
	{
		CachedPC = Cast<APlayerController>(GetController());
		if (!CachedPC.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("TriggerHapticFeedback: PlayerController no encontrado"));
			return;
		}
	}

	APlayerController* PC = CachedPC.Get();
	if (!PC)
		return;


	// Validar parámetros
	Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	Frequency = FMath::Clamp(Frequency, 0.0f, 1.0f);


	// Aplicar vibración
	if (Intensity > TOLERANCE_FLOAT)
	{
		PC->SetHapticsByValue(Intensity, Frequency, Hand);
	}
	else
	{
		CachedPC->StopHapticEffect(Hand);
	}

	
}

#pragma region Inputs
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
#pragma region Triggers
void ARobot::GraspLeft(const FInputActionValue& Value){
	left.grab = Value.Get<float>();
};
void ARobot::GraspReleaseLeft(const FInputActionValue& Value){
	left.grab = 0;
};
void ARobot::GraspRight(const FInputActionValue& Value){
	right.grab =  Value.Get<float>();
	std::cout<< right.grab<< std::endl;
	
};

void ARobot::GraspReleaseRight(const FInputActionValue& Value){
	right.grab = 0;
	std::cout<< right.grab<< std::endl;
};
void ARobot::TriggerLeft(const FInputActionValue& Value){
	left.trigger = Value.Get<float>();
};
void ARobot::TriggerReleaseLeft(const FInputActionValue& Value){
	left.trigger = 0;
};
void ARobot::TriggerRight(const FInputActionValue& Value){
	right.trigger = Value.Get<float>();
};
void ARobot::TriggerReleaseRight(const FInputActionValue& Value){
	right.trigger = 0;
};
#pragma endregion
#pragma region Button
void ARobot::PushA(const FInputActionValue& Value){
	right.aButton= true;
};
void ARobot::ReleaseA(const FInputActionValue& Value){
	right.aButton= false;
};
void ARobot::PushB(const FInputActionValue& Value){
	right.bButton= true;
};
void ARobot::ReleaseB(const FInputActionValue& Value){
	right.bButton= false;
};
void ARobot::PushX(const FInputActionValue& Value){
	left.aButton= true;
};
void ARobot::ReleaseX(const FInputActionValue& Value){
	left.aButton= false;
};
void ARobot::PushY(const FInputActionValue& Value){
	left.bButton= true;
};
void ARobot::ReleaseY(const FInputActionValue& Value){
	left.bButton= false;
};
#pragma endregion
#pragma region ThumbStick
void ARobot::ThumbStickLeft(const FInputActionValue& Value)
{
    FVector2D StickValue = Value.Get<FVector2D>();
	left.x = StickValue.X;
	left.y = StickValue.Y;
}
void ARobot::ThumbStickReleaseLeft(const FInputActionValue& Value)
{
	left.x = 0;
    left.y = 0;
}
void ARobot::ThumbStickRight(const FInputActionValue& Value)
{
    FVector2D StickValue = Value.Get<FVector2D>();
	right.x = StickValue.X;
	right.y = StickValue.Y;
}
void ARobot::ThumbStickReleaseRight(const FInputActionValue& Value)
{
	right.x = 0;
	right.y = 0;
}
#pragma endregion
#pragma endregion


// Called every frame
void ARobot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if (!GEngine or !GetWorld() or !GetWorld()->IsGameWorld())
		return ;
	
	FVector HMDPos = VRCamera->GetComponentLocation();
	FQuat HMDQuat = VRCamera->GetComponentQuat();

	FVector LeftPos = LeftController->GetComponentLocation();
	FQuat LeftQuat = LeftController->GetComponentQuat();

	FVector RightPos = RightController->GetComponentLocation();
	FQuat RightQuat = RightController->GetComponentQuat();

	middleware.sendData(RobotMiddleware::Pose{HMDPos.X, HMDPos.Y, HMDPos.Z, HMDQuat.X, HMDQuat.Y, HMDQuat.Z, HMDQuat.W},
		RobotMiddleware::Pose{LeftPos.X, LeftPos.Y, LeftPos.Z, LeftQuat.X, LeftQuat.Y, LeftQuat.Z, LeftQuat.W}, left,
		RobotMiddleware::Pose{RightPos.X, RightPos.Y, RightPos.Z, RightQuat.X, RightQuat.Y, RightQuat.Z, RightQuat.W}, right
	);

	RobotMiddleware::Haptic leftHaptic, rightHaptic;
	if (middleware.receiveHaptics(leftHaptic, rightHaptic))
	{
		TriggerHapticFeedback(EControllerHand::Left, leftHaptic.intensity, leftHaptic.frequency);
		TriggerHapticFeedback(EControllerHand::Right, rightHaptic.intensity, rightHaptic.frequency);
	}


	LeftGriper->SetWorldRotation(LeftQuat);
	LeftGriper->SetWorldLocation(LeftPos);

	RightGriper->SetWorldRotation(RightQuat);
	RightGriper->SetWorldLocation(RightPos);

	// if (FPSQueue.size() == maxSize) {
	// 	FPSQueue.pop_front();
	// }
	// FPSQueue.push_back(1/DeltaTime);
	//

	#pragma region Debug
	// auto VecToStr2 = [](const FVector& V) {
	// 	return FString::Printf(TEXT("X=%.2f, Y=%.2f, Z=%.2f"), V.X, V.Y, V.Z);
	// };
	// auto RotToStr2 = [](const FRotator& V) {
	// 	return FString::Printf(TEXT("X=%.2f, Y=%.2f, Z=%.2f"), V.Pitch, V.Yaw, V.Roll);
	// };
	// auto QuatToStr2 = [](const FQuat& Q) {
	// 	return FString::Printf(TEXT("X=%.5f, Y=%.5f, Z=%.5f, W=%.5f"), Q.X, Q.Y, Q.Z, Q.W);
	// };
	//
	// GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Black,
	// 	FString::Printf(TEXT("HMD: %s\n%s\n%s"), *VecToStr2(HMDPos), *RotToStr2(HMDRot), *QuatToStr2(HMDQuat)), true, FVector2D(2.5, 2.5));
	//
	// GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Red,
	// 	FString::Printf(TEXT("Left: %s\n%s\n%s"), *VecToStr2(LeftPos), *RotToStr2(LeftRot), *QuatToStr2(LeftQuat)), true, FVector2D(2.5, 2.5));
	//
	// GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Blue,
	// 	FString::Printf(TEXT("Right: %s\n%s\n%s"), *VecToStr2(RightPos), *RotToStr2(RightRot), *QuatToStr2(RightQuat)), true, FVector2D(2.5, 2.5));

	// GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Orange,
	// 	FString::Printf(TEXT("HZ: %.3f"), 1/DeltaTime), true, FVector2D(3, 3));
	//
	// 	// Left controller
	// DrawDebugCoordinateSystem(
	// 	GetWorld(),
	// 	LeftPos,                   // Origen
	// 	LeftRot,        // Orientación
	// 	10.0f,                     // Longitud de los ejes
	// 	false,                     // Persistente
	// 	-1.f,                      // Tiempo de vida (segundos)
	// 	0,                         // DepthPriority
	// 	1.f                       // Grosor
	// );
	//
	// // Right controller
	// DrawDebugCoordinateSystem(
	// 	GetWorld(),
	// 	RightPos,
	// 	RightRot,
	// 	10.0f,
	// 	false,
	// 	-1.f,
	// 	0,
	// 	1.f
	// );
#pragma endregion

}

