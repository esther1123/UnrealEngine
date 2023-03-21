// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuLevelScriptActor.h"

#include "Blueprint/UserWidget.h"
#include "DayOne/DayOne.h"
#include "Kismet/GameplayStatics.h"

void AMainMenuLevelScriptActor::BeginPlay()
{
	Super::BeginPlay();

	if (!GetGameInstance()->IsDedicatedServerInstance())
	{
		UE_LOG(LogDayOne, Warning, TEXT("AMainMenuLevelScriptActor::BeginPlay"));

		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
		if (PlayerController)
		{
			FInputModeUIOnly UIOnlyInput;
			PlayerController->SetInputMode(UIOnlyInput);
			PlayerController->SetShowMouseCursor(true);
		}
	}
}
