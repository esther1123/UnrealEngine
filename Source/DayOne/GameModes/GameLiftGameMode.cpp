// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftGameMode.h"
#include "GameLiftServerSDK.h"
#include "DayOne/DayOne.h"
#include "DayOne/GameInstance/DayOneGameInstance.h"
#include "DayOne/GameStates/DayOneGameState.h"
#include "DayOne/HUD/DayOneHUD.h"
#include "DayOne/PlayerStates/DayOnePlayerState.h"
#include "Kismet/GameplayStatics.h"

AGameLiftGameMode::AGameLiftGameMode()
{
	HUDClass = ADayOneHUD::StaticClass();
	GameStateClass = ADayOneGameState::StaticClass();
	PlayerStateClass = ADayOnePlayerState::StaticClass();

	GameSessionState = EGameSessionState::EGST_Waiting;
	// Max 90 seconds game time
	RemainingGameTime = 90;
}

void AGameLiftGameMode::BeginPlay()
{
	Super::BeginPlay();

#if WITH_GAMELIFT
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr)
	{
		UDayOneGameInstance* DayOneGameInstance = Cast<UDayOneGameInstance>(GameInstance);
		if (DayOneGameInstance != nullptr)
		{
			DayOneGameInstance->StartGameSessionState.OnStartGameSession.BindUObject(this, &ThisClass::OnStartGameSession);
			DayOneGameInstance->ProcessTerminateState.OnProcessTerminate.BindUObject(this, &ThisClass::OnProcessTerminate);
		}
	}
	UE_LOG(LogDayOne, Warning, TEXT("SetTimer->GuardGameSession"));
	GetWorldTimerManager().SetTimer(GuardGameSessionHandle, this, &ThisClass::GuardGameSession, 1.0f, true, 1.0f);
#endif
}

FString AGameLiftGameMode::InitNewPlayer(APlayerController* NewPlayerController,
	                                     const FUniqueNetIdRepl& UniqueId,
	                                     const FString& Options,
	                                     const FString& Portal)
{
#if WITH_GAMELIFT
	const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
	const FString& PlayerId = UGameplayStatics::ParseOption(Options, "PlayerId");

	if (NewPlayerController != nullptr)
	{
		APlayerState* PlayerState = NewPlayerController->PlayerState;
		if (PlayerState != nullptr)
		{
			ADayOnePlayerState* DayOnePlayerState = Cast<ADayOnePlayerState>(PlayerState);
			if (DayOnePlayerState != nullptr)
			{
				DayOnePlayerState->PlayerGameId = PlayerId;

				FGameLiftPlayer* GameLiftPlayer = StartGameSessionState.ReservedPlayers.Find(PlayerId);
				if (GameLiftPlayer != nullptr)
				{
					GameLiftPlayer->PlayerSessionId = PlayerSessionId;
					DayOnePlayerState->TeamName = GameLiftPlayer->TeamName;
				}
			}
		}
	}
#endif
	
	return Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
}

void AGameLiftGameMode::PreLogin(const FString& Options,
	                             const FString& Address,
	                             const FUniqueNetIdRepl& UniqueId,
	                             FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

#if WITH_GAMELIFT
	if (Options.Len() > 0)
	{
		const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
		const FString& PlayerId = UGameplayStatics::ParseOption(Options, "PlayerId");

		if (PlayerSessionId.Len() > 0 && PlayerId.Len() > 0)
		{
			Aws::GameLift::Server::Model::DescribePlayerSessionsRequest DescribePlayerSessionsRequest;
			DescribePlayerSessionsRequest.SetPlayerSessionId(TCHAR_TO_ANSI(*PlayerSessionId));

			auto DescribePlayerSessionsOutcome = Aws::GameLift::Server::DescribePlayerSessions(DescribePlayerSessionsRequest);
			if (DescribePlayerSessionsOutcome.IsSuccess())
			{
				auto DescribePlayerSessionsResult = DescribePlayerSessionsOutcome.GetResult();
				int Count = 1;
				auto PlayerSessions = DescribePlayerSessionsResult.GetPlayerSessions(Count);
				if (PlayerSessions != nullptr)
				{
					auto PlayerSession = PlayerSessions[0];
					FString ExpectedPlayerId = PlayerSession.GetPlayerId();
					auto PlayerStatus = PlayerSession.GetStatus();

					UE_LOG(LogDayOne, Warning, TEXT("Expected PlayerId: %s, Actual PlayerId: %s, Player Status: %d"), *ExpectedPlayerId, *PlayerId, (int)PlayerStatus);

					if (ExpectedPlayerId.Equals(PlayerId) && PlayerStatus == Aws::GameLift::Server::Model::PlayerSessionStatus::RESERVED)
					{
						auto AcceptPlayerSessionOutcome = Aws::GameLift::Server::AcceptPlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));
						if (!AcceptPlayerSessionOutcome.IsSuccess())
						{
							ErrorMessage = FString::Printf(TEXT("PlayerSessionId: %s, AcceptPlayerSession Failed: %s"), *PlayerSessionId, ANSI_TO_TCHAR(AcceptPlayerSessionOutcome.GetError().GetErrorMessage()));
						}
					}
					else
					{
						if (!ExpectedPlayerId.Equals(PlayerId))
						{
							ErrorMessage = FString::Printf(TEXT("Expected Player ID: %s and Player ID: %s doesn't match"), *ExpectedPlayerId, *PlayerId);
						}
						else if (PlayerStatus != Aws::GameLift::Server::Model::PlayerSessionStatus::RESERVED)
						{
							ErrorMessage = FString::Printf(TEXT("PlayerStatus isn't RESERVED"));
						}
						else
						{
							ErrorMessage = "Unknown Unauthorized error";
						}
					}
				}
				else
				{
					ErrorMessage = "Failed to get PlayerSession";
				}
			}
			else
			{
				ErrorMessage = "Failed to Describe PlayerSession";
			}
		}
		else
		{
			ErrorMessage = "Invalid PlayerSessionId or PlayerId";
		}
	}
	else
	{
		ErrorMessage = "Invalid Options";
	}
#endif
}

void AGameLiftGameMode::Logout(AController* Exiting)
{
#if WITH_GAMELIFT
	if (Exiting != nullptr)
	{
		APlayerState* PlayerState = Exiting->PlayerState;
		if (PlayerState != nullptr)
		{
			ADayOnePlayerState* DayOnePlayerState = Cast<ADayOnePlayerState>(PlayerState);
			if (DayOnePlayerState != nullptr)
			{
				const FGameLiftPlayer* GameLiftPlayer = StartGameSessionState.ReservedPlayers.Find(DayOnePlayerState->PlayerGameId);
				if (GameLiftPlayer != nullptr)
				{
					Aws::GameLift::Server::RemovePlayerSession(TCHAR_TO_ANSI(*GameLiftPlayer->PlayerSessionId));
				}
			}
		}
	}
#endif
	
	Super::Logout(Exiting);
}

void AGameLiftGameMode::GuardGameSession()
{
	if (GameSessionState == EGameSessionState::EGST_Waiting)
	{
		if (StartGameSessionState.bIsSuccess)
		{
			UE_LOG(LogDayOne, Warning, TEXT("Game session is running"))
			GameSessionState = EGameSessionState::EGST_Running;
			UE_LOG(LogDayOne, Warning, TEXT("SetTimer->CountdownToGameOver"));
			GetWorldTimerManager().SetTimer(CountdownToGameOverHandle, this, &ThisClass::CountdownToGameOver, 1.0f, true, 1.0f);
		}
	}
	else if (GameSessionState == EGameSessionState::EGST_Running)
	{
		if (ProcessTerminateState.bIsTerminating)
		{
			UE_LOG(LogDayOne, Warning, TEXT("Game session is force terminating"))
			GameSessionState = EGameSessionState::EGST_Terminating;
			UE_LOG(LogDayOne, Warning, TEXT("SetTimer->ProcessTermination"));
			GetWorldTimerManager().SetTimer(ProcessTerminationHandle, this, &ThisClass::ProcessTermination, 10.0f, true, 1.0f);
		}
	}
}

void AGameLiftGameMode::CountdownToGameOver()
{
	RemainingGameTime--;
	
	if (GameState != nullptr)
	{
		ADayOneGameState* DayOneGameState = Cast<ADayOneGameState>(GameState);
		if (DayOneGameState != nullptr)
		{
			if (RemainingGameTime >= 0)
			{
				DayOneGameState->GameOverCountDown = RemainingGameTime;
			}
			else
			{
				DayOneGameState->GameMessage = "Game Over";
				// We must clear the CountdownToGameOver timer here. Because this timer is faster than EndGame timer.
				GetWorldTimerManager().ClearTimer(CountdownToGameOverHandle);
				if (GameSessionState == EGameSessionState::EGST_Terminating)
				{
					GetWorldTimerManager().ClearTimer(ProcessTerminationHandle);
				}
				GameSessionState = EGameSessionState::EGST_Ending;
				UE_LOG(LogDayOne, Warning, TEXT("SetTimer->EndGame in CountdownToGameOver"));
				GetWorldTimerManager().SetTimer(EndGameHandle, this, &ThisClass::EndGame, 1.0f, false, 5.0f);
			}
		}
	}
}

void AGameLiftGameMode::ProcessTermination()
{
	// Send notification to player
	FString GameInterruptionMessage;
	long TimeLeft = (long)(ProcessTerminateState.TerminationTime - FDateTime::Now().ToUnixTimestamp());
	// If GameLift service will terminate game server in 15s,
	// we'd better notify players the server is shutting down now.
	if (TimeLeft < 15L)
	{
		GameInterruptionMessage = "Game server is shutting down";
		// EndGame timer is faster than ProcessTermination timer,
		// we can clear ProcessTermination timer in EndGame.
		GetWorldTimerManager().ClearTimer(ProcessTerminationHandle);
		UE_LOG(LogDayOne, Warning, TEXT("SetTimer->EndGame in ProcessTermination"));
		GetWorldTimerManager().SetTimer(EndGameHandle, this, &ThisClass::EndGame, 1.0f, false, 5.0f);
	}
	else
	{
		GameInterruptionMessage = FString::Printf(TEXT("Game Server scheduled to terminate in %ld seconds"), TimeLeft);
	}
	
	ADayOneGameState* DayOneGameState = Cast<ADayOneGameState>(GetWorld()->GetGameState());
	if (DayOneGameState != nullptr)
	{
		DayOneGameState->GameMessage = GameInterruptionMessage;
	}
}

void AGameLiftGameMode::EndGame()
{
	UE_LOG(LogDayOne, Warning, TEXT("EndGame"));
	
	GetWorldTimerManager().ClearTimer(GuardGameSessionHandle);
	GetWorldTimerManager().ClearTimer(CountdownToGameOverHandle);
	GetWorldTimerManager().ClearTimer(ProcessTerminationHandle);
	GetWorldTimerManager().ClearTimer(EndGameHandle);

#if WITH_GAMELIFT
	Aws::GameLift::Server::ProcessEnding();
#endif
	FGenericPlatformMisc::RequestExit(false);
}

void AGameLiftGameMode::OnStartGameSession(const FStartGameSessionState& State)
{
	// !NOTE!: Never call GetWorldTimerManager().SetTimer here.
	// !This functions was called from Non-Game thread!
	StartGameSessionState = State;
}

void AGameLiftGameMode::OnProcessTerminate(const FProcessTerminateState& State)
{
	// !NOTE!: Never call GetWorldTimerManager().SetTimer here.
	// !This functions was called from Non-Game thread!
	ProcessTerminateState = State;
}
