// Fill out your copyright notice in the Description page of Project Settings.
#include "Expert.h"
#include <iostream>

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
AExpert::AExpert()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    SetupPoseComponent();
}

// Called when the game starts or when spawned
void AExpert::BeginPlay()
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
			UE_LOG(LogTemp, Warning, TEXT("AExpert::BeginPlay -> Pawn dont has PlayerController"));
		}
		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AP3Bot::StaticClass(), FoundActors);
		UE_LOG(LogTemp, Warning, TEXT("FoundActors.Num(): %d"), FoundActors.Num());
		if (FoundActors.Num() > 0)
			P3Bot = Cast<AP3Bot>(FoundActors[0]);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Editor mode"));
	}
}
void AExpert::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// exportFPSQueueToCSV(FPSQueue, "/home/robolab/fps_data.csv", "FPS");
}

void AExpert::SetupPoseComponent()
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

void AExpert::TriggerHapticFeedback(
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
void AExpert::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
		// You can bind to any of the trigger events here by changing the "ETriggerEvent" enum value
		Input->BindAction(IA_Hand_Grasp_Left, ETriggerEvent::Triggered, this, &AExpert::GraspLeft);
		Input->BindAction(IA_Hand_Grasp_Left, ETriggerEvent::Completed, this, &AExpert::GraspReleaseLeft);
		Input->BindAction(IA_Hand_Grasp_Right, ETriggerEvent::Triggered, this, &AExpert::GraspRight);
		Input->BindAction(IA_Hand_Grasp_Right, ETriggerEvent::Completed, this, &AExpert::GraspReleaseRight);
		Input->BindAction(IA_Hand_IndexCurl_Left, ETriggerEvent::Triggered, this, &AExpert::TriggerLeft);
		Input->BindAction(IA_Hand_IndexCurl_Left, ETriggerEvent::Completed, this, &AExpert::TriggerReleaseLeft);
		Input->BindAction(IA_Hand_IndexCurl_Right, ETriggerEvent::Triggered, this, &AExpert::TriggerRight);
		Input->BindAction(IA_Hand_IndexCurl_Right, ETriggerEvent::Completed, this, &AExpert::TriggerReleaseRight);
		Input->BindAction(IA_Hand_A, ETriggerEvent::Started, this, &AExpert::PushA);
		Input->BindAction(IA_Hand_A, ETriggerEvent::Completed, this, &AExpert::ReleaseA);
		Input->BindAction(IA_Hand_B, ETriggerEvent::Started, this, &AExpert::PushB);
		Input->BindAction(IA_Hand_B, ETriggerEvent::Completed, this, &AExpert::ReleaseB);
		Input->BindAction(IA_Hand_X, ETriggerEvent::Started, this, &AExpert::PushX);
		Input->BindAction(IA_Hand_X, ETriggerEvent::Completed, this, &AExpert::ReleaseX);
		Input->BindAction(IA_Hand_Y, ETriggerEvent::Started, this, &AExpert::PushY);
		Input->BindAction(IA_Hand_Y, ETriggerEvent::Completed, this, &AExpert::ReleaseY);
		Input->BindAction(IA_Hand_Thumbstick_Left, ETriggerEvent::Triggered, this, &AExpert::ThumbStickLeft);
		Input->BindAction(IA_Hand_Thumbstick_Left, ETriggerEvent::Completed, this, &AExpert::ThumbStickReleaseLeft);
		Input->BindAction(IA_Hand_Thumbstick_Right, ETriggerEvent::Triggered, this, &AExpert::ThumbStickRight);
		Input->BindAction(IA_Hand_Thumbstick_Right, ETriggerEvent::Completed, this, &AExpert::ThumbStickReleaseRight);
		Input->BindAction(IA_Hand_Thumbstick_Press_Left, ETriggerEvent::Started, this, &AExpert::PushThumbStickLeft);
		Input->BindAction(IA_Hand_Thumbstick_Press_Left, ETriggerEvent::Completed, this, &AExpert::ReleaseThumbStickLeft);
		Input->BindAction(IA_Hand_Thumbstick_Press_Right, ETriggerEvent::Started, this, &AExpert::PushThumbStickRight);
		Input->BindAction(IA_Hand_Thumbstick_Press_Right, ETriggerEvent::Completed, this, &AExpert::ReleaseThumbStickRight);

}
#pragma region Triggers
void AExpert::GraspLeft(const FInputActionValue& Value){
	left.grab = Value.Get<float>();
};
void AExpert::GraspReleaseLeft(const FInputActionValue& Value){
	left.grab = 0;
};
void AExpert::GraspRight(const FInputActionValue& Value){
	right.grab =  Value.Get<float>();
};
void AExpert::GraspReleaseRight(const FInputActionValue& Value){
	right.grab = 0;
};
void AExpert::TriggerLeft(const FInputActionValue& Value){
	left.trigger = Value.Get<float>();
};
void AExpert::TriggerReleaseLeft(const FInputActionValue& Value){
	left.trigger = 0;
};
void AExpert::TriggerRight(const FInputActionValue& Value){
	right.trigger = Value.Get<float>();
};
void AExpert::TriggerReleaseRight(const FInputActionValue& Value){
	right.trigger = 0;
};
#pragma endregion
#pragma region Button
void AExpert::PushA(const FInputActionValue& Value){
	right.aButton= true;
};
void AExpert::ReleaseA(const FInputActionValue& Value){
	right.aButton= false;
};
void AExpert::PushB(const FInputActionValue& Value){
	right.bButton= true;
};
void AExpert::ReleaseB(const FInputActionValue& Value){
	right.bButton= false;
};
void AExpert::PushX(const FInputActionValue& Value){
	left.aButton= true;
};
void AExpert::ReleaseX(const FInputActionValue& Value){
	left.aButton= false;
};
void AExpert::PushY(const FInputActionValue& Value){
	left.bButton= true;
};
void AExpert::ReleaseY(const FInputActionValue& Value){
	left.bButton= false;
};
#pragma endregion
#pragma region ThumbStick
void AExpert::ThumbStickLeft(const FInputActionValue& Value)
{
    FVector2D StickValue = Value.Get<FVector2D>();
	left.x = StickValue.X;
	left.y = StickValue.Y;
}
void AExpert::ThumbStickReleaseLeft(const FInputActionValue& Value)
{
	left.x = 0;
    left.y = 0;
}
void AExpert::ThumbStickRight(const FInputActionValue& Value)
{
    FVector2D StickValue = Value.Get<FVector2D>();
	right.x = StickValue.X;
	right.y = StickValue.Y;
}
void AExpert::ThumbStickReleaseRight(const FInputActionValue& Value)
{
	right.x = 0;
	right.y = 0;
}
void AExpert::PushThumbStickLeft(const FInputActionValue& Value){
	left.thumbstickButton = true;
};
void AExpert::ReleaseThumbStickLeft(const FInputActionValue& Value){
	left.thumbstickButton = false;
};
void AExpert::PushThumbStickRight(const FInputActionValue& Value){
	right.thumbstickButton = true;
	followRobot = !followRobot;
	middleware.stopBase();
};
void AExpert::ReleaseThumbStickRight(const FInputActionValue& Value){
	right.thumbstickButton = false;
};
#pragma endregion
#pragma endregion


// Called every frame
void AExpert::Tick(float DeltaTime)
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

	LeftPos = left.bButton ? HMDPos + (LeftPos - HMDPos) * 1.9f : LeftPos;
	RightPos = right.bButton ? HMDPos + (RightPos - HMDPos) * 1.9f : RightPos;

	LeftGriper->SetWorldRotation(LeftQuat);
	LeftGriper->SetWorldLocation(LeftPos);

	RightGriper->SetWorldRotation(RightQuat);
	RightGriper->SetWorldLocation(RightPos);

	if (P3Bot)
	{
		FTransform BotTransform = P3Bot->GetActorTransform();
		FQuat BotQuat = P3Bot->GetActorQuat();

		HMDPos = BotTransform.InverseTransformPosition(HMDPos);
		HMDQuat = BotQuat.Inverse()*HMDQuat;

		LeftPos = BotTransform.InverseTransformPosition(LeftPos);
		LeftQuat = BotQuat.Inverse()*LeftQuat;

		RightPos = BotTransform.InverseTransformPosition(RightPos);
		RightQuat = BotQuat.Inverse()*RightQuat;
		// UE_LOG(LogTemp, Warning, TEXT("HMDPos -> (%f, %f, %f), LeftPos -> (%f, %f, %f), RightPos -> (%f, %f, %f)"), HMDPos.X, HMDPos.Y, HMDPos.Z, LeftPos.X, LeftPos.Y, LeftPos.Z, RightPos.X, RightPos.Y, RightPos.Z);
	}

	middleware.sendData(RobotMiddleware::Pose{HMDPos.X, HMDPos.Y, HMDPos.Z, HMDQuat.X, HMDQuat.Y, HMDQuat.Z, HMDQuat.W},
		RobotMiddleware::Pose{LeftPos.X, LeftPos.Y, LeftPos.Z, LeftQuat.X, LeftQuat.Y, LeftQuat.Z, LeftQuat.W}, left,
		RobotMiddleware::Pose{RightPos.X, RightPos.Y, RightPos.Z, RightQuat.X, RightQuat.Y, RightQuat.Z, RightQuat.W}, right
	);

	if (followRobot){
		middleware.setBasePose(RobotMiddleware::Pose{HMDPos.X, HMDPos.Y, HMDPos.Z, HMDQuat.X, HMDQuat.Y, HMDQuat.Z, HMDQuat.W});	
	}
	else{
		if (left.thumbstickButton)
		{
			middleware.resetOdometer();
		}
		if (left.x == 0 and left.y == 0 and right.x == 0 )
		{
			if(stopRobot == false)
				middleware.stopBase();
			stopRobot = true;
		}
		else
		{
			stopRobot = false;
		}
		if (!stopRobot)
			middleware.setSpeedBase(left.x*500, left.y*500, -right.x*1.5);
	}

	RobotMiddleware::Haptic leftHaptic, rightHaptic;
	if (middleware.receiveHaptics(leftHaptic, rightHaptic))
	{
		TriggerHapticFeedback(EControllerHand::Left, leftHaptic.intensity, leftHaptic.frequency);
		TriggerHapticFeedback(EControllerHand::Right, rightHaptic.intensity, rightHaptic.frequency);
	}




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

