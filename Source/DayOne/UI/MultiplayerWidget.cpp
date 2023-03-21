// Fill out your copyright notice in the Description page of Project Settings.

#include "MultiplayerWidget.h"
#include "IWebBrowserCookieManager.h"
#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "GameLiftClientModule.h"
#include "DayOne/GameInstance/DayOneGameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "DayOne/DayOne.h"
#include "Kismet/GameplayStatics.h"

UMultiplayerWidget::UMultiplayerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GLClientModule = &FGameLiftClientModule::Get();
	bSearchingForGameSession = false;
}

void UMultiplayerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bIsFocusable = true;

	// Bind button callback
	LoginButton->OnClicked.AddDynamic(this, &ThisClass::OnLoginButtonClicked);
	JoinButton->OnClicked.AddDynamic(this, &ThisClass::OnJoinButtonClicked);

	// Setup timer to update latency UI text
	UE_LOG(LogDayOne, Warning, TEXT("SetTimer->UpdateLatencyUI"));
	GetWorld()->GetTimerManager().SetTimer(UpdateLatencyUIHandle, this, &ThisClass::UpdateLatencyUI, 1.0f, true, 1.0f);

	// Check if Cognito token is valid.
	// If token still valid we don't need to show login Ui to player. Just update player info to display.
	if (GLClientModule->GameLiftClient->IsTokenValid())
	{
		LoginButton->SetIsEnabled(false);
		JoinButton->SetIsEnabled(true);
		GLClientModule->GameLiftClient->GetPlayerData().BindUObject(this, &ThisClass::OnGLGetPlayerDataResponse);
	}
	else
	{
		JoinButton->SetIsEnabled(false);
	}
}

void UMultiplayerWidget::NativeDestruct()
{
	if (bSearchingForGameSession)
	{
		GLClientModule->GameLiftClient->StopMatchmaking();
	}
	
	GetWorld()->GetTimerManager().ClearTimer(PollMatchmakingHandle);
	GetWorld()->GetTimerManager().ClearTimer(UpdateLatencyUIHandle);
	
	Super::NativeDestruct();
}

void UMultiplayerWidget::UpdateLatencyUI()
{
	FString PingString("Latency\n");
	for (const TPair<FString, float>& Pair : GLClientModule->GameLiftClient->AverageLatencyPerRegion)
	{
		FString RegionPingString = FString::Printf(TEXT("%s: %dms\n"), *Pair.Key,  FMath::RoundToInt(Pair.Value));
		PingString += RegionPingString;
	}
	TextBlock_Latency->SetText(FText::FromString(PingString));
}

void UMultiplayerWidget::PollMatchmaking()
{
	if (bSearchingForGameSession)
	{
		GLClientModule->GameLiftClient->PollMatchmaking().BindUObject(this, &ThisClass::OnGLPollMatchmakingResponse);
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(PollMatchmakingHandle);
	}
}

void UMultiplayerWidget::OnLoginButtonClicked()
{
	LoginButton->SetIsEnabled(false);
	
	// Clean the cookies.
	IWebBrowserSingleton* WebBrowserSingleton = IWebBrowserModule::Get().GetSingleton();
	if (WebBrowserSingleton != nullptr) {
		TOptional<FString> DefaultContext;
		TSharedPtr<IWebBrowserCookieManager> CookieManager = WebBrowserSingleton->GetCookieManager(DefaultContext);
		if (CookieManager.IsValid()) {
			CookieManager->DeleteCookies();
		}
	}
	// Show Cognito Hosted login UI to player.
	GLClientModule->GameLiftClient->ShowLoginUI(*WebBrowser_Login).AddUObject(this, &ThisClass::OnGLLoginResponse);
}

void UMultiplayerWidget::OnJoinButtonClicked()
{
	// Disable button to prevent player click it multi times.
	JoinButton->SetIsEnabled(false);

	// If we are already in searching process, just cancel it.
	if (bSearchingForGameSession)
	{
		bSearchingForGameSession = false;
		GetWorld()->GetTimerManager().ClearTimer(PollMatchmakingHandle);
		
		GLClientModule->GameLiftClient->StopMatchmaking().BindUObject(this, &ThisClass::OnGLStopMatchmakingResponse);
	}
	else // We are not in searching, start matchmaking
	{
		GLClientModule->GameLiftClient->StartMatchmaking().BindUObject(this, &ThisClass::OnGLStartMatchmakingResponse);
	}
}

void UMultiplayerWidget::OnGLLoginResponse(FString AuthzCode)
{
	UE_LOG(LogDayOne, Warning, TEXT("UMMSettingsWidget::OnGLLoginResponse AuthzCode: %s"), *AuthzCode);
	GLClientModule->GameLiftClient->ExchangeCodeToTokens(AuthzCode).AddUObject(this, &ThisClass::OnGLExchangeCodeToTokensResponse);
}

void UMultiplayerWidget::OnGLExchangeCodeToTokensResponse(FString AccessToken, FString RefreshToken, int ExpiresIn)
{
	UE_LOG(LogDayOne, Warning, TEXT("AccessToken: %s"), *AccessToken);
	UE_LOG(LogDayOne, Warning, TEXT("RefreshToken: %s"), *RefreshToken);
	UE_LOG(LogDayOne, Warning, TEXT("ExpiresIn: %d"), ExpiresIn);

	// Just for FIRE refresh token timer
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr)
	{
		UDayOneGameInstance* DayOneGameInstance = Cast<UDayOneGameInstance>(GameInstance);
		if (DayOneGameInstance != nullptr)
		{
			DayOneGameInstance->SetRefreshTokensTimer();
		}
	}

	GLClientModule->GameLiftClient->GetPlayerData().BindUObject(this, &ThisClass::OnGLGetPlayerDataResponse);
}

void UMultiplayerWidget::OnGLGetPlayerDataResponse(FString PlayerId, int Wins, int Losses)
{
	UE_LOG(LogDayOne, Warning, TEXT("PlayerId: %s, Wins: %d, Losses: %d"), *PlayerId, Wins, Losses);

	TextBlock_PlayerId->SetVisibility(ESlateVisibility::Visible);
	TextBlock_PlayerId->SetText(FText::FromString(FString::Printf(TEXT("Player ID: %s"), *PlayerId)));

	// Compute player score
	int Score = 100 * Wins - 30 * Losses;
	TextBlock_Score->SetVisibility(ESlateVisibility::Visible);
	TextBlock_Score->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), Score)));

	WebBrowser_Login->SetVisibility(ESlateVisibility::Hidden);
	JoinButton->SetIsEnabled(true);
}

void UMultiplayerWidget::OnGLStartMatchmakingResponse(FString TicketId)
{
	UE_LOG(LogDayOne, Warning, TEXT("TicketId: %s"), *TicketId);

	bSearchingForGameSession = true;
	UE_LOG(LogDayOne, Warning, TEXT("SetTimer->PollMatchmaking"));
	GetWorld()->GetTimerManager().SetTimer(PollMatchmakingHandle, this, &ThisClass::PollMatchmaking, 10.0f, true, 3.0f);
	
	UTextBlock* ButtonTextBlock = (UTextBlock*)JoinButton->GetChildAt(0);
	ButtonTextBlock->SetText(FText::FromString("Cancel"));
	TextBlock_MatchmakingEvent->SetText(FText::FromString("Searching for game session..."));

	JoinButton->SetIsEnabled(true);
}

void UMultiplayerWidget::OnGLStopMatchmakingResponse()
{
	UTextBlock* ButtonTextBlock = (UTextBlock*)JoinButton->GetChildAt(0);
	ButtonTextBlock->SetText(FText::FromString("Join"));
	TextBlock_MatchmakingEvent->SetText(FText::FromString(""));

	JoinButton->SetIsEnabled(true);
}

void UMultiplayerWidget::OnGLPollMatchmakingResponse(FString TicketType,
	                                                FString PlayerId, FString PlayerSessionId,
	                                                FString IpAddress, FString Port)
{
	UE_LOG(LogDayOne, Warning, TEXT("TicketType: %s, PlayerId: %s, PlayerSessionId: %s, IpAddress: %s, Port: %s"), *TicketType, *PlayerId, *PlayerSessionId, *IpAddress, *Port);

	if (bSearchingForGameSession)
	{
		GetWorld()->GetTimerManager().ClearTimer(PollMatchmakingHandle);
		bSearchingForGameSession = false;

		if (TicketType.Equals("MatchmakingSucceeded"))
		{
			JoinButton->SetIsEnabled(false);
			TextBlock_MatchmakingEvent->SetText(FText::FromString("Game session found. Connecting to game server..."));

			FString LevelName = IpAddress + ":" + Port;
			const FString& Options = "?PlayerSessionId=" + PlayerSessionId + "?PlayerId=" + PlayerId;
			UE_LOG(LogDayOne, Warning, TEXT("options: %s"), *Options);

			UGameplayStatics::OpenLevel(GetWorld(), FName(*LevelName), false, Options);
		}
		else
		{
			UTextBlock* ButtonTextBlock = (UTextBlock*)JoinButton->GetChildAt(0);
			ButtonTextBlock->SetText(FText::FromString("Join"));
			TextBlock_MatchmakingEvent->SetText(FText::FromString(TicketType));
		}
	}
}
