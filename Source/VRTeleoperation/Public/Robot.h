// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "InputAction.h" 

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Engine/Engine.h"
#include "RobotMiddleware.h"

#include "DrawDebugHelpers.h"
#include "Robot.generated.h"


UCLASS()
class VRTELEOPERATION_API ARobot : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ARobot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void SetupPoseComponent();
	void GraspLeft(const FInputActionValue& Value);
	void GraspRight(const FInputActionValue& Value);
	void TriggerLeft(const FInputActionValue& Value);
	void TriggerRight(const FInputActionValue& Value);
	void OnTriggerPressed(const FInputActionValue& Value);

    UPROPERTY(VisibleAnywhere)
    UCameraComponent* VRCamera;

    UPROPERTY(VisibleAnywhere)
    UMotionControllerComponent* LeftController;

    UPROPERTY(VisibleAnywhere)
    UMotionControllerComponent* RightController;

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

	


private:
	RobotMiddleware middleware;
	RobotMiddleware::Controller left, right;
	bool controllerChanged = false;

};
