// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "MainMenuLevelScriptActor.generated.h"

/**
 * 
 */
UCLASS()
class DAYONE_API AMainMenuLevelScriptActor : public ALevelScriptActor
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
};
