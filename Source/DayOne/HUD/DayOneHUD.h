// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DayOneHUD.generated.h"

/**
 * 
 */
UCLASS()
class DAYONE_API ADayOneHUD : public AHUD
{
	GENERATED_BODY()

public:
	ADayOneHUD();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TSubclassOf<UUserWidget> DayOneWidgetClass;
};
