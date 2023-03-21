// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameLiftGameInstance.h"
#include "Engine/GameInstance.h"
#include "DayOneGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class DAYONE_API UDayOneGameInstance : public UGameLiftGameInstance
{
	GENERATED_BODY()

public:
	UDayOneGameInstance();
	
	void SetRefreshTokensTimer();

protected:
	virtual void Init() override;
	virtual void Shutdown() override;

private:
	class FGameLiftClientModule* GLClientModule;

	// Timer
	FTimerHandle RefreshTokensHandle;
	UFUNCTION()
	void RefreshTokens();
	
	FTimerHandle TestLatencyHandle;
	UFUNCTION()
	void TestLatency();
	
	// Callback
	void OnGLRefreshTokensResponse(FString AccessToken, bool bIsSuccessful);
};
