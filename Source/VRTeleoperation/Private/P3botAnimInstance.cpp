#include "P3botAnimInstance.h"

void UP3botAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	middleware = &RobotMiddlewareSingleton::Get();

	if (!middleware->running)
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

void UP3botAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// EJEMPLO: aquí podrías leer tu middleware
	// Por ejemplo, si tienes una función que devuelve las últimas poses
	float left[8], right[8];
	//if (middleware->getRobotState(left, right))
		;
	LeftQ0 -= 1;
}