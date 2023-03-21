// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DayOne/GameInstance/GameLiftGameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameLiftGameMode.generated.h"

UENUM()
enum class EGameSessionState : uint8
{
	// Game server is ready and waiting for GameLift service OnGameSessionStart callback
	EGST_Waiting,
	// Has already received OnGameSessionStart callback
	EGST_Running,
	// Has already received OnProcessTermination callback
	EGST_Terminating,
	// Players have already finished this game session
	EGST_Ending
};

/**
 * 
 */
UCLASS()
class DAYONE_API AGameLiftGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGameLiftGameMode();

protected:
	virtual void BeginPlay() override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void Logout(AController* Exiting) override;

private:
	EGameSessionState GameSessionState;
	// Max game time
	int RemainingGameTime;
	
	// Timer
	FTimerHandle GuardGameSessionHandle;
	UFUNCTION()
	void GuardGameSession();
	
	FTimerHandle CountdownToGameOverHandle;
	UFUNCTION()
	void CountdownToGameOver();
	
	FTimerHandle ProcessTerminationHandle;
	UFUNCTION()
	void ProcessTermination();
	
	FTimerHandle EndGameHandle;
	UFUNCTION()
	void EndGame();

	// Callback
	FStartGameSessionState StartGameSessionState;
	void OnStartGameSession(const FStartGameSessionState& State);
	FProcessTerminateState ProcessTerminateState;
	void OnProcessTerminate(const FProcessTerminateState& State);
};
