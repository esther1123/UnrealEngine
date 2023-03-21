// Fill out your copyright notice in the Description page of Project Settings.


#include "DayOneWidget.h"

#include "GameLiftClientModule.h"
#include "Components/TextBlock.h"
#include "DayOne/DayOne.h"
#include "DayOne/GameInstance/DayOneGameInstance.h"
#include "DayOne/GameStates/DayOneGameState.h"
#include "DayOne/PlayerStates/DayOnePlayerState.h"

void UDayOneWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerState* OwningPlayerState = GetOwningPlayerState();
	if (OwningPlayerState != nullptr)
	{
		ADayOnePlayerState* DayOnePlayerState = Cast<ADayOnePlayerState>(OwningPlayerState);
		if (DayOnePlayerState != nullptr)
		{
			TextBlock_PlayerId->SetText(FText::FromString("Player ID: " + DayOnePlayerState->PlayerGameId));
			TextBlock_TeamName->SetText(FText::FromString("Team Name: " + DayOnePlayerState->TeamName));
		}
	}

	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState != nullptr)
	{
		ADayOneGameState* DayOneGameState = Cast<ADayOneGameState>(GameState);
		if (DayOneGameState != nullptr)
		{
			TextBlock_GameOverCountDown->SetText(FText::FromString(FString::FromInt(DayOneGameState->GameOverCountDown)));
			TextBlock_GameMessage->SetText(FText::FromString(DayOneGameState->GameMessage));
		}
	}
}
