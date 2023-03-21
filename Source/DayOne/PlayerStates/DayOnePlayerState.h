// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DayOnePlayerState.generated.h"

/**
 * 
 */
UCLASS()
class DAYONE_API ADayOnePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated)
	FString PlayerGameId;
	UPROPERTY(Replicated)
	FString TeamName;

protected:
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
};
