// Fill out your copyright notice in the Description page of Project Settings.


#include "NewMainMenuHUD.h"
#include "Blueprint/UserWidget.h"

ANewMainMenuHUD::ANewMainMenuHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> MainMenuBGObject(TEXT("/Game/DayOne/UI/MainMenu/WBP_MainMenuBG"));
	NewMainMenuBGClass = MainMenuBGObject.Class;

	static ConstructorHelpers::FClassFinder<UUserWidget> MainMenuBtnObject(TEXT("/Game/DayOne/UI/MainMenu/WBP_MainMenuBtn"));
	NewMainMenuBtnClass = MainMenuBtnObject.Class;
}

void ANewMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr) {
		PlayerController->bShowMouseCursor = true;
	}

	if (NewMainMenuBGClass != nullptr && NewMainMenuBtnClass != nullptr)
	{
		UUserWidget* MainMenuBGWidget = CreateWidget<UUserWidget>(GetWorld(), NewMainMenuBGClass);
		if (MainMenuBGWidget != nullptr)
		{
			MainMenuBGWidget->AddToViewport();
		}
		UUserWidget* MainMenuBtnWidget = CreateWidget<UUserWidget>(GetWorld(), NewMainMenuBtnClass);
		if (MainMenuBtnWidget != nullptr)
		{
			MainMenuBtnWidget->AddToViewport();
		}
	}
}
