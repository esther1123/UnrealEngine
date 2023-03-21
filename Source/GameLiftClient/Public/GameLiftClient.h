#pragma once

#include "CoreMinimal.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "WebBrowser.h"
#include "GameLiftClient.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGameLiftClient, Log, All);

typedef TMap<FString, float> LatencyMap;
typedef TDoubleLinkedList<float> LatencyList;
typedef TMap<FString, TSharedPtr<LatencyList> > LatencyListMap;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoginResponse, FString /*AuthzCode*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnExchangeCodeToTokensResponse, FString /*AccessToken*/, FString /*RefreshToken*/, int /*ExpiresIn*/);
DECLARE_DELEGATE_TwoParams(FOnRefreshTokensResponse, FString /*AccessToken*/, bool /*bIsSuccessful*/);
DECLARE_DELEGATE(FOnRevokeTokensResponse);
DECLARE_DELEGATE_ThreeParams(FOnGetPlayerDataResponse, FString /*PlayerId*/, int /*Wins*/, int /*Losses*/);
DECLARE_DELEGATE_OneParam(FOnStartMatchmakingResponse, FString /*TicketId*/);
DECLARE_DELEGATE(FOnStopMatchmakingResponse);
DECLARE_DELEGATE_FiveParams(FOnPollMatchmakingResponse, FString /*TicketType*/, FString /*PlayerId*/, FString /*PlayerSessionId*/, FString /*IpAddress*/, FString /*Port*/);
DECLARE_DELEGATE_OneParam(FOnTestLatencyResponse, LatencyMap /*AverageLatencyMap*/);

UCLASS(Config="GameLiftClient")
class GAMELIFTCLIENT_API UGameLiftClient : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Config)
	FString LoginUrl;
	UPROPERTY(Config)
	FString LoginCallbackUrl;
	UPROPERTY(Config)
	FString ApiKey;
	UPROPERTY(Config)
	FString InvokeUrl;
	UPROPERTY(Config)
	TArray<FString> RegionCodes;
	
	UGameLiftClient();

	virtual void PostInitProperties() override;

	// Check if token is valid
	bool IsTokenValid() const;

	// ShowLoginUI
	FOnLoginResponse& ShowLoginUI(UWebBrowser& WebBrowser);
	UFUNCTION()
	void OnLoginUrlChanged(const FText& BrowserUrl);
	FOnLoginResponse OnLoginResponse;

	// ExchangeCodeToTokens
	FOnExchangeCodeToTokensResponse& ExchangeCodeToTokens(const FString& AuthzCode);
	void OnExchangeCodeToTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	FOnExchangeCodeToTokensResponse OnExchangeCodeToTokensResponse;

	// RefreshTokens
	FOnRefreshTokensResponse& RefreshTokens();
	void OnRefreshTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	FOnRefreshTokensResponse OnRefreshTokensResponse;

	// RevokeTokens
	FOnRevokeTokensResponse& RevokeTokens();
	void OnRevokeTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	FOnRevokeTokensResponse OnRevokeTokensResponse;
	
	// GetPlayerData
	FOnGetPlayerDataResponse& GetPlayerData();
	void OnGetPlayerDataResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	FOnGetPlayerDataResponse OnGetPlayerDataResponse;

	// StartMatchmaking
	FOnStartMatchmakingResponse& StartMatchmaking();
	void OnStartMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	FOnStartMatchmakingResponse OnStartMatchmakingResponse;

	// StopMatchmaking
	FOnStopMatchmakingResponse& StopMatchmaking();
	void OnStopMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	FOnStopMatchmakingResponse OnStopMatchmakingResponse;

	// PollMatchmaking
	FOnPollMatchmakingResponse& PollMatchmaking();
	void OnPollMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	FOnPollMatchmakingResponse OnPollMatchmakingResponse;

	// TestLatency
	FOnTestLatencyResponse& TestLatency();
	void OnTestLatencyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	FOnTestLatencyResponse OnTestLatencyResponse;
	
//private:
	class FHttpModule* HttpModule;
	
	FString PlayerId;
	
	FString TicketId;
	
	FString AccessToken;
	FString RefreshToken;
	int TokenExpiresIn;
	int64 TokenTTL;

	// Clean every function TestLatency call.
	LatencyMap LatencyMapCache;
	LatencyListMap LatencyListPerRegion;
	LatencyMap AverageLatencyPerRegion;
};