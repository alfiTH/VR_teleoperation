// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "InputAction.h"

#include "GameFramework/PlayerController.h"
#include "Haptics/HapticFeedbackEffect_Base.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Engine/Engine.h"
#include "RobotMiddleware.h"
#include "P3Bot.h"
#include <atomic>
#include <iostream>
#include <deque>
#include <fstream>

#include "DrawDebugHelpers.h"
#include "Expert.generated.h"


UCLASS()
class VRTELEOPERATION_API AExpert : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AExpert();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void TriggerHapticFeedback(EControllerHand Hand, float Intensity, float Frequency);
	void SetupPoseComponent();

	
	void GraspLeft(const FInputActionValue& Value);
	void GraspReleaseLeft(const FInputActionValue& Value);
	void GraspRight(const FInputActionValue& Value);
	void GraspReleaseRight(const FInputActionValue& Value);
	void TriggerLeft(const FInputActionValue& Value);
	void TriggerReleaseLeft(const FInputActionValue& Value);
	void TriggerRight(const FInputActionValue& Value);
	void TriggerReleaseRight(const FInputActionValue& Value);
	void PushA(const FInputActionValue& Value);
	void ReleaseA(const FInputActionValue& Value);
	void PushB(const FInputActionValue& Value);
	void ReleaseB(const FInputActionValue& Value);
	void PushX(const FInputActionValue& Value);
	void ReleaseX(const FInputActionValue& Value);
	void PushY(const FInputActionValue& Value);
	void ReleaseY(const FInputActionValue& Value);
	void ThumbStickLeft(const FInputActionValue& Value);
	void ThumbStickReleaseLeft(const FInputActionValue& Value);
	void ThumbStickRight(const FInputActionValue& Value);
	void ThumbStickReleaseRight(const FInputActionValue& Value);
	void PushThumbStickLeft(const FInputActionValue& Value);
	void ReleaseThumbStickLeft(const FInputActionValue& Value);
	void PushThumbStickRight(const FInputActionValue& Value);
	void ReleaseThumbStickRight(const FInputActionValue& Value);


    UPROPERTY(VisibleAnywhere)
    UCameraComponent* VRCamera;

    UPROPERTY(VisibleAnywhere)
    UMotionControllerComponent* LeftController;

    UPROPERTY(VisibleAnywhere)
    UMotionControllerComponent* RightController;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* LeftGriper;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* RightGriper;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* MI_Griper;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputMappingContext* IMC_Robot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_Grasp_Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_Grasp_Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_IndexCurl_Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_IndexCurl_Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_Thumbstick_Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_Thumbstick_Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_Thumbstick_Press_Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	UInputAction* IA_Hand_Thumbstick_Press_Right;

	// Radio máximo (en cm, unidades de Unreal) entre la persona y el P3Bot para permitir el followRobot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Safety")
	float MaxFollowDistance = 100.0f;

private:
	RobotMiddleware& middleware = RobotMiddleware::getInstance();
	TWeakObjectPtr<APlayerController> CachedPC;
	RobotMiddleware::Controller left, right;
	std::atomic<bool> controllerChanged = false;
	std::deque<float> FPSQueue;
	const size_t maxSize = 2000;
	std::atomic<bool> followRobot = false;
	std::atomic<bool> stopRobot = false;
	bool bOutOfFollowRangeWarned = false;

	std::atomic<bool> continuousOdometerReset = false;
	float LastThumbstickLeftPressTime = -1.0f;

	AP3Bot* P3Bot = nullptr;

public:

};
