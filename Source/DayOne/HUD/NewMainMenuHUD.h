// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NewMainMenuHUD.generated.h"

/**
 * 
 */
UCLASS()
class DAYONE_API ANewMainMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	ANewMainMenuHUD();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TSubclassOf<UUserWidget> NewMainMenuBGClass;

	UPROPERTY()
	TSubclassOf<UUserWidget> NewMainMenuBtnClass;
};
