// Fill out your copyright notice in the Description page of Project Settings.


#include "P3botAnimInstance.h"

void UP3botAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	// Aquí inicializas referencias, etc.
}


void UP3botAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// EJEMPLO: aquí podrías leer tu middleware
	// Por ejemplo, si tienes una función que devuelve las últimas poses

	LeftQ0 -= 1;
}