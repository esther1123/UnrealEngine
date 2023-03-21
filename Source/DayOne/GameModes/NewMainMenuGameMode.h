// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NewMainMenuGameMode.generated.h"

/**
 * 
 */
UCLASS()
class DAYONE_API ANewMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANewMainMenuGameMode();

	virtual void BeginPlay() override;
};
