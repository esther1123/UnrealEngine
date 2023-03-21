// Fill out your copyright notice in the Description page of Project Settings.


#include "DayOneHUD.h"

#include "Blueprint/UserWidget.h"

ADayOneHUD::ADayOneHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> DayOneWidgetObject(TEXT("/Game/DayOne/UI/WBP_DayOneWidget"));
	DayOneWidgetClass = DayOneWidgetObject.Class;
}

void ADayOneHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr) {
		PlayerController->bShowMouseCursor = false;
		PlayerController->SetInputMode(FInputModeGameAndUI());
	}

	if (DayOneWidgetClass != nullptr) {
		UUserWidget* DayOneWidget = CreateWidget<UUserWidget>(GetWorld(), DayOneWidgetClass);
		if (DayOneWidget != nullptr) {
			DayOneWidget->AddToViewport();
		}
	}
}
