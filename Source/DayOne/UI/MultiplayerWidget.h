// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MultiplayerWidget.generated.h"

/**
 * 
 */
UCLASS()
class DAYONE_API UMultiplayerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMultiplayerWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	// UI binding
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UWebBrowser* WebBrowser_Login;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* TextBlock_Latency;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* TextBlock_PlayerId;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* TextBlock_Score;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* TextBlock_MatchmakingEvent;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* LoginButton;
	UFUNCTION()
	void OnLoginButtonClicked();

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	class UButton* JoinButton;
	UFUNCTION()
	void OnJoinButtonClicked();

private:
	// Timer
	FTimerHandle UpdateLatencyUIHandle;
	UFUNCTION()
	void UpdateLatencyUI();
	
	FTimerHandle PollMatchmakingHandle;
	UFUNCTION()
	void PollMatchmaking();
	
	// Variables
	class FGameLiftClientModule* GLClientModule;
	bool bSearchingForGameSession;

	// Callbacks
	void OnGLLoginResponse(FString AuthzCode);
	void OnGLExchangeCodeToTokensResponse(FString AccessToken, FString RefreshToken, int ExpiresIn);
	void OnGLGetPlayerDataResponse(FString PlayerId, int Wins, int Losses);
	void OnGLStartMatchmakingResponse(FString TicketId);
	void OnGLStopMatchmakingResponse();
	void OnGLPollMatchmakingResponse(FString TicketType, FString PlayerId, FString PlayerSessionId, FString IpAddress, FString Port);
};
