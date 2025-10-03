// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "RobotMiddlewareSingleton.h"
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot")
	float LeftQ0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot")
	float LeftQ1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot")
	float LeftQ2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot")
	float LeftQ3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot")
	float LeftQ4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot")
	float LeftQ5;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="P3Bot")
	float LeftQ6;
	

	RobotMiddleware* middleware;
	
};
