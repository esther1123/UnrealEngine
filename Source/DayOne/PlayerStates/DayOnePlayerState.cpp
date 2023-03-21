// Fill out your copyright notice in the Description page of Project Settings.


#include "DayOnePlayerState.h"

#include "Net/UnrealNetwork.h"

void ADayOnePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PlayerGameId);
	DOREPLIFETIME(ThisClass, TeamName);
}
