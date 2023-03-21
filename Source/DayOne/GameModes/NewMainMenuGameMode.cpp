// Fill out your copyright notice in the Description page of Project Settings.


#include "NewMainMenuGameMode.h"
#include "DayOne/HUD/NewMainMenuHUD.h"

ANewMainMenuGameMode::ANewMainMenuGameMode()
{
	HUDClass = ANewMainMenuHUD::StaticClass();
}

void ANewMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr)
	{
		PlayerController->SetInputMode(FInputModeUIOnly());
	}
}
