#include "P3botAnimInstance.h"

void UP3botAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		if (!middleware.isRunning())
		{
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
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("Editor mode"));
	}
}

void UP3botAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!GEngine or !GetWorld() or !GetWorld()->IsGameWorld())
		return ;
	
	if (!middleware.isRunning()) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Middleware not initialized in UP3botAnimInstance"));
		return;
	}

	float left[8] = {0.0f};
	float right[8] = {0.0f};
	
	// Leer el estado del robot desde la memoria (no bloqueante)
	if (middleware.getRobotState(left, right))
	{
		LeftQ0 = -left[0];
		LeftQ1 = -left[1];
		LeftQ2 = -left[2];
		LeftQ3 = -left[3];
		LeftQ4 = -left[4];
		LeftQ5 = -left[5];
		LeftQ6 = -left[6];
		LeftQ7 = left[7];

		RightQ0 = -right[0];
		RightQ1 = -right[1];
		RightQ2 = -right[2];
		RightQ3 = -right[3];
		RightQ4 = -right[4];
		RightQ5 = -right[5];
		RightQ6 = -right[6];
		RightQ7 = right[7];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to get robot state from middleware"));
	}
	
}