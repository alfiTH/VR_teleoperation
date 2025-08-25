// Fill out your copyright notice in the Description page of Project Settings.


#include "MyVRGameMode.h"
#include "Robot.h"   // tu Pawn

AMyVRGameMode::AMyVRGameMode()
{
    // Forzar a que el Pawn por defecto sea tu ARobot
    DefaultPawnClass = ARobot::StaticClass();
}