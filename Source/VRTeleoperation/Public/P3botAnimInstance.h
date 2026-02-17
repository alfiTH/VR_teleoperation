// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RobotMiddleware.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#include "P3botAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class VRTELEOPERATION_API UP3botAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	void NativeInitializeAnimation();
	void NativeUpdateAnimation(float DeltaSeconds);

	
public:
	// ---- Left Arm ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|LeftArm")
	float LeftQ0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|LeftArm")
	float LeftQ1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|LeftArm")
	float LeftQ2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|LeftArm")
	float LeftQ3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|LeftArm")
	float LeftQ4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|LeftArm")
	float LeftQ5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|LeftArm")
	float LeftQ6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|LeftArm")
	float LeftQ7;

	// ---- Right Arm ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|RightArm")
	float RightQ0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|RightArm")
	float RightQ1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|RightArm")
	float RightQ2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|RightArm")
	float RightQ3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|RightArm")
	float RightQ4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|RightArm")
	float RightQ5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|RightArm")
	float RightQ6;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot|RightArm")
	float RightQ7;

	RobotMiddleware& middleware = RobotMiddleware::getInstance();
	
};
