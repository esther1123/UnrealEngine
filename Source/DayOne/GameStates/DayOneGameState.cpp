// Fill out your copyright notice in the Description page of Project Settings.


#include "DayOneGameState.h"

#include "Net/UnrealNetwork.h"

void ADayOneGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, GameMessage);
	DOREPLIFETIME(ThisClass, GameOverCountDown);
}
