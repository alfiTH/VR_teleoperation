// Fill out your copyright notice in the Description page of Project Settings.


#include "PointCloudPLYActor.h"

// Sets default values
APointCloudPLYActor::APointCloudPLYActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APointCloudPLYActor::BeginPlay()
{
	Super::BeginPlay();

	if (bLoadOnBeginPlay) DrawPoints();
}

void APointCloudPLYActor::LoadPLY()
{
	Positions.Empty();
	Colors.Empty();

	FString FullPath = FPaths::ConvertRelativePathToFull(PLYFile.FilePath);
	FString Error;

	if (!ParseASCIIPLY(FullPath, Positions, Colors, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("LoadPLY failed: %s"), *Error);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("LoadPLY: Loaded %d points from %s"), Positions.Num(), *FullPath);
}

void APointCloudPLYActor::DrawPoints()
{
	// Dibujar algunos puntos
	int32 ToDraw = FMath::Min(MaxDebugDrawPoints, Positions.Num());
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadPLY: No world"));
		return;
	}
		
	for (int32 i = 0; i < ToDraw; i++)
	{
		FVector WorldPosition = Positions[i];
		FColor Color = (Colors.IsValidIndex(i)) ? Colors[i].ToFColor(true) : FColor::White;
		DrawDebugPoint(World, WorldPosition, DebugPointSize, Color, false, 4.0f);
	}
}

bool APointCloudPLYActor::ParseASCIIPLY(const FString& FullPath, TArray<FVector>& OutPosition, TArray<FLinearColor>& OutColors, FString& OutError)
{
	OutPosition.Reset();
	OutColors.Reset();

	#pragma region Errors
	if (FullPath.IsEmpty())
	{
		OutError = TEXT("No path provided");
		return false;
	}

	if (!FPaths::FileExists(FullPath))
	{
		OutError = FString::Printf(TEXT("File not found: %s"), *FullPath);
		return false;
	}

	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *FullPath))
	{
		OutError =  TEXT("Failed to load file");
		return false;
	}
	#pragma endregion Errors

	TArray<FString> Lines;
	FileContents.ParseIntoArrayLines(Lines);

	int32 NumVertices = 0;
	int32 HeaderEndLine = INDEX_NONE;
	TArray<FString> PropertyNames;

	for (int32 i = 0; i < Lines.Num(); i++)
	{
		FString Line = Lines[i].TrimStartAndEnd();
		if (Line.StartsWith(TEXT("element vertex")))
		{
			TArray<FString> Tokens;
			Line.ParseIntoArrayWS(Tokens);
			if (Tokens.Num() >= 3)
				NumVertices = FCString::Atoi(*Tokens[2]);
		}
		else if (Line.StartsWith(TEXT("property")))
		{
			TArray<FString> Tokens;
			Line.ParseIntoArrayWS(Tokens);
			if (Tokens.Num() >= 3)
				PropertyNames.Add(Tokens.Last().ToLower());
		}
		else if (Line.StartsWith(TEXT("end_header")))
		{
			HeaderEndLine = i;
			break;
		}
	}

	if (HeaderEndLine == INDEX_NONE)
	{
		OutError = TEXT("PLY header not terminated with end_header");
		return false;
	}

	if (NumVertices == 0)
        UE_LOG(LogTemp, Warning, TEXT("ParseASCIIPLY: NumVertices found = %d (will try to parse remaining lines)"), NumVertices);

	// Parse vertex data
	int32 LineIndex = HeaderEndLine + 1;
	int32 Parsed = 0;
	while (LineIndex < Lines.Num() && (NumVertices <= 0 || Parsed < NumVertices))
	{
		FString L = Lines[LineIndex++].TrimStartAndEnd();
		if (L.IsEmpty()) continue;

		TArray<FString> Tokens;
		L.ParseIntoArrayWS(Tokens);
		if (Tokens.Num() < PropertyNames.Num())
		{
			// Mal formado: saltar
			continue;
		}

		float x = 0.f, y = 0.f, z = 0.f;
		float r = 1.f, g = 1.f, b = 1.f, a = 1.f;
		bool hasColor = false;
	
		for (int32 p = 0; p < PropertyNames.Num() && p < Tokens.Num(); ++p)
		{
			FString Prop = PropertyNames[p];
			const FString& Tok = Tokens[p];

			if (Prop == TEXT("x"))
			{
				x = FCString::Atof(*Tok);
			}
			else if (Prop == TEXT("y"))
			{
				y = FCString::Atof(*Tok);
			}
			else if (Prop == TEXT("z"))
			{
				z = FCString::Atof(*Tok);
			}
			else if (Prop == TEXT("red") || Prop == TEXT("r"))
			{
				r = FCString::Atof(*Tok);
				hasColor = true;
			}
			else if (Prop == TEXT("green") || Prop == TEXT("g"))
			{
				g = FCString::Atof(*Tok);
			}
			else if (Prop == TEXT("blue") || Prop == TEXT("b"))
			{
				b = FCString::Atof(*Tok);
			}
		}

		// ajustar rango color (si vienen como 0..255)
		if (hasColor)
		{
			if (r > 1.0f || g > 1.0f || b > 1.0f)
			{
				r = FMath::Clamp(r / 255.0f, 0.0f, 1.0f);
				g = FMath::Clamp(g / 255.0f, 0.0f, 1.0f);
				b = FMath::Clamp(b / 255.0f, 0.0f, 1.0f);
			}
		}

		OutPosition.Add(FVector(x, y, z)*10);
		OutColors.Add(FLinearColor(r, g, b, a));

		++Parsed;
	}

	// Si no se encontró número en header, Parsed es lo que realmente se parseó
	if (NumVertices > 0 && Parsed != NumVertices)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseASCIIPLY: header said %d vertices but parsed %d (continuing)"), NumVertices, Parsed);
	}

	return true;
}


