#include "GameLiftClient.h"

DEFINE_LOG_CATEGORY(LogGameLiftClient);

UGameLiftClient::UGameLiftClient()
{
	PlayerId = "";

	TicketId = "";
	
	AccessToken = "";
	RefreshToken = "";
	TokenExpiresIn = 0;
	TokenTTL = 0;
	
	HttpModule = &FHttpModule::Get();
}

void UGameLiftClient::PostInitProperties()
{
	UObject::PostInitProperties();
	
	UE_LOG(LogGameLiftClient, Warning, TEXT("LoginUrl: %s"), *LoginUrl);
	UE_LOG(LogGameLiftClient, Warning, TEXT("LoginCallbackUrl: %s"), *LoginCallbackUrl);
	UE_LOG(LogGameLiftClient, Warning, TEXT("ApiKey: %s"), *ApiKey);
	UE_LOG(LogGameLiftClient, Warning, TEXT("InvokeUrl: %s"), *InvokeUrl);
	for (FString Region : RegionCodes)
	{
		UE_LOG(LogGameLiftClient, Warning, TEXT("Region: %s"), *Region);

		AverageLatencyPerRegion.Add(Region, 0.0f);
		LatencyListPerRegion.Add(Region, MakeShareable(new LatencyList));
	}
}

bool UGameLiftClient::IsTokenValid() const
{
	if (AccessToken.Len() > 0 && TokenTTL > FDateTime::UtcNow().ToUnixTimestamp())
	{
		return true;
	}
	return false;
}

FOnLoginResponse& UGameLiftClient::ShowLoginUI(UWebBrowser& WebBrowser)
{
	// Load Cognito Hosted UI.
	if (!WebBrowser.IsVisible())
	{
		WebBrowser.SetVisibility(ESlateVisibility::Visible);
	}
	WebBrowser.LoadURL(LoginUrl);
	WebBrowser.OnUrlChanged.AddUniqueDynamic(this, &ThisClass::OnLoginUrlChanged);

	return OnLoginResponse;
}

void UGameLiftClient::OnLoginUrlChanged(const FText& BrowserUrl)
{
	FString Url;
	FString QueryParameters;

	UE_LOG(LogGameLiftClient, Warning, TEXT("BrowserUrl: %s"), *BrowserUrl.ToString())

	// Only process Cognito Hosted UI login callback URL.
	if (BrowserUrl.ToString().Split("?", &Url, &QueryParameters)) {
		if (Url.Equals(LoginCallbackUrl)) {
			FString ParameterName;
			FString ParameterValue;

			// Extract Cognito login code.
			if (QueryParameters.Split("=", &ParameterName, &ParameterValue)) {
				if (ParameterName.Equals("code")) {
					// Remove the end character '#'
					if (ParameterValue.EndsWith("#"))
					{
						ParameterValue.MidInline(0, ParameterValue.Len() - 1);
					}
					if (OnLoginResponse.IsBound())
					{
						OnLoginResponse.Broadcast(ParameterValue);
					}
				}
			}
		}
	}
}

FOnExchangeCodeToTokensResponse& UGameLiftClient::ExchangeCodeToTokens(const FString& AuthzCode)
{
	const FString InvokeResource("/tokens/exchange");
	const FString InvokeMethod("POST");
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("authzCode", AuthzCode);

	FString InvokeBody;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&InvokeBody);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter))
	{
		TSharedRef<IHttpRequest> ExchangeCodeToTokensRequest = HttpModule->CreateRequest();
		ExchangeCodeToTokensRequest->SetURL(InvokeUrl + InvokeResource);
		ExchangeCodeToTokensRequest->SetVerb(InvokeMethod);
		ExchangeCodeToTokensRequest->SetHeader("x-api-key", ApiKey);
		ExchangeCodeToTokensRequest->SetHeader("Content-Type", "application/json");
		ExchangeCodeToTokensRequest->SetContentAsString(InvokeBody);
		ExchangeCodeToTokensRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnExchangeCodeToTokensResponseReceived);
		ExchangeCodeToTokensRequest->ProcessRequest();
	}
	else
	{
		UE_LOG(LogGameLiftClient, Warning, TEXT("Failed to serialize json object to string"));
	}

	return OnExchangeCodeToTokensResponse;
}

void UGameLiftClient::OnExchangeCodeToTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	UE_LOG(LogGameLiftClient, Warning, TEXT("GameLiftClient::OnExchangeCodeToTokensResponseReceived, ResponseCode: %d"), Response->GetResponseCode());
	
	if (bConnectedSuccessfully)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject)) {
			if (JsonObject->HasField("accessToken") && JsonObject->HasField("refreshToken") && JsonObject->HasField("expiresIn")) {
				AccessToken = JsonObject->GetStringField("accessToken");
				RefreshToken = JsonObject->GetStringField("refreshToken");
				TokenExpiresIn = JsonObject->GetIntegerField("expiresIn");
				TokenTTL = FDateTime::UtcNow().ToUnixTimestamp() + TokenExpiresIn;

				UE_LOG(LogGameLiftClient, Warning, TEXT("AccessToken: %s, RefreshToken: %s, ExpiresIn: %d"), *AccessToken, *RefreshToken, TokenExpiresIn);

				if (OnExchangeCodeToTokensResponse.IsBound())
				{
					OnExchangeCodeToTokensResponse.Broadcast(AccessToken, RefreshToken, TokenExpiresIn);
				}
			}
		}
		else
		{
			UE_LOG(LogGameLiftClient, Warning, TEXT("Failed to deserialize body string to json object"));
		}
	}
	else
	{
		UE_LOG(LogGameLiftClient, Warning, TEXT("ExchangeCodeToTokens request get failed response"));
	}
}

FOnRefreshTokensResponse& UGameLiftClient::RefreshTokens()
{
	const FString InvokeResource("/tokens/refresh");
	const FString InvokeMethod("POST");

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("refreshToken", RefreshToken);

	FString InvokeBody;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&InvokeBody);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter))
	{
		TSharedRef<IHttpRequest> RefreshTokensRequest = HttpModule->CreateRequest();
		RefreshTokensRequest->SetURL(InvokeUrl + InvokeResource);
		RefreshTokensRequest->SetVerb(InvokeMethod);
		RefreshTokensRequest->SetHeader("x-api-key", ApiKey);
		RefreshTokensRequest->SetHeader("Content-Type", "application/json");
		RefreshTokensRequest->SetHeader("Authorization", AccessToken);
		RefreshTokensRequest->SetContentAsString(InvokeBody);
		RefreshTokensRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnRefreshTokensResponseReceived);
		RefreshTokensRequest->ProcessRequest();
	}

	return OnRefreshTokensResponse;
}

void UGameLiftClient::OnRefreshTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	UE_LOG(LogGameLiftClient, Warning, TEXT("GameLiftClient::OnRefreshTokensResponseReceived, ResponseCode: %d"), Response->GetResponseCode());
	
	// Process error first
	if (!IsTokenValid())
	{
		UE_LOG(LogGameLiftClient, Warning, TEXT("Failed refresh cognito tokens. AccessToken has timedout."));
		OnRefreshTokensResponse.ExecuteIfBound("", false);
		return;
	}
	if (!bConnectedSuccessfully)
	{
		UE_LOG(LogGameLiftClient, Warning, TEXT("Failed refresh cognito tokens. Can't connect with server."));
		OnRefreshTokensResponse.ExecuteIfBound("", false);
		return;
	}
	if (Response->GetResponseCode() > 400)
	{
		UE_LOG(LogGameLiftClient, Warning, TEXT("Failed refresh cognito tokens. Response code: %d. Reason: %s."),
			Response->GetResponseCode(), *Response->GetContentAsString());
		OnRefreshTokensResponse.ExecuteIfBound("", false);
		return;
	}
	
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
	{
		if (JsonObject->HasField("accessToken"))
		{
			AccessToken = JsonObject->GetStringField("accessToken");
			// Refresh Token TimeToLive time
			TokenTTL = FDateTime::UtcNow().ToUnixTimestamp() + TokenExpiresIn;
			
			OnRefreshTokensResponse.ExecuteIfBound(AccessToken, true);
		}
		else
		{
			UE_LOG(LogGameLiftClient, Warning, TEXT("Failed refresh cognito tokens. No 'accessToken' field in response."));
			OnRefreshTokensResponse.ExecuteIfBound("", false);
		}
	}
}

FOnRevokeTokensResponse& UGameLiftClient::RevokeTokens()
{
	const FString InvokeResource("/tokens/revoke");
	const FString InvokeMethod("POST");
	
	TSharedRef<IHttpRequest> RevokeTokensRequest = HttpModule->CreateRequest();
	RevokeTokensRequest->SetURL(InvokeUrl + InvokeResource);
	RevokeTokensRequest->SetVerb(InvokeMethod);
	RevokeTokensRequest->SetHeader("x-api-key", ApiKey);
	RevokeTokensRequest->SetHeader("Authorization", AccessToken);
	RevokeTokensRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnRevokeTokensResponseReceived);
	RevokeTokensRequest->ProcessRequest();

	return OnRevokeTokensResponse;
}

void UGameLiftClient::OnRevokeTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	UE_LOG(LogGameLiftClient, Warning, TEXT("GameLiftClient::OnRevokeTokensResponseReceived, ResponseCode: %d"), Response->GetResponseCode());
	
	if (bConnectedSuccessfully)
	{
		AccessToken = "";
		RefreshToken = "";
		TokenExpiresIn = 0;
		TokenTTL = 0;
		OnRevokeTokensResponse.ExecuteIfBound();
	}
}

FOnGetPlayerDataResponse& UGameLiftClient::GetPlayerData()
{
	const FString InvokeResource("/players/self");
	const FString InvokeMethod("GET");
	
	TSharedRef<IHttpRequest> GetPlayerDataRequest = HttpModule->CreateRequest();
	GetPlayerDataRequest->SetURL(InvokeUrl + InvokeResource);
	GetPlayerDataRequest->SetVerb(InvokeMethod);
	GetPlayerDataRequest->SetHeader("x-api-key", ApiKey);
	GetPlayerDataRequest->SetHeader("Authorization", AccessToken);
	GetPlayerDataRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnGetPlayerDataResponseReceived);
	GetPlayerDataRequest->ProcessRequest();

	return OnGetPlayerDataResponse;
}

void UGameLiftClient::OnGetPlayerDataResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	UE_LOG(LogGameLiftClient, Warning, TEXT("GameLiftClient::OnGetPlayerDataResponseReceived, ResponseCode: %d"), Response->GetResponseCode());
	
	if (bConnectedSuccessfully) {
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject)) {
			if (JsonObject->HasField("playerId") && JsonObject->HasField("wins") && JsonObject->HasField("losses"))
			{
				PlayerId = JsonObject->GetStringField("playerId");
				int Wins = JsonObject->GetIntegerField("wins");
				int Losses = JsonObject->GetIntegerField("losses");

				OnGetPlayerDataResponse.ExecuteIfBound(PlayerId, Wins, Losses);
			}
		}
	}
}

FOnStartMatchmakingResponse& UGameLiftClient::StartMatchmaking()
{
	const FString InvokeResource("/matchmaking");
	const FString InvokeMethod("POST");
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	for (const TPair<FString, float>& Pair : AverageLatencyPerRegion)
	{
		JsonObject->SetNumberField(Pair.Key, FMath::RoundToInt(Pair.Value));
	}

	FString InvokeBody;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&InvokeBody);
	if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter))
	{
		TSharedRef<IHttpRequest> StartMatchmakingRequest = HttpModule->CreateRequest();
		StartMatchmakingRequest->SetURL(InvokeUrl + InvokeResource);
		StartMatchmakingRequest->SetVerb(InvokeMethod);
		StartMatchmakingRequest->SetHeader("x-api-key", ApiKey);
		StartMatchmakingRequest->SetHeader("Content-Type", "application/json");
		StartMatchmakingRequest->SetHeader("Authorization", AccessToken);
		StartMatchmakingRequest->SetContentAsString(InvokeBody);
		StartMatchmakingRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnStartMatchmakingResponseReceived);
		StartMatchmakingRequest->ProcessRequest();
	}

	return OnStartMatchmakingResponse;
}

void UGameLiftClient::OnStartMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	UE_LOG(LogGameLiftClient, Warning, TEXT("GameLiftClient::OnStartMatchmakingResponseReceived, ResponseCode: %d"), Response->GetResponseCode());
	
	if (bConnectedSuccessfully) {
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
		{
			if (JsonObject->HasField("ticketId")) {
				TicketId = JsonObject->GetStringField("ticketId");
				OnStartMatchmakingResponse.ExecuteIfBound(TicketId);
			}
		}
	}
}

FOnStopMatchmakingResponse& UGameLiftClient::StopMatchmaking()
{
	const FString InvokeResource("/matchmaking/" + TicketId);
	const FString InvokeMethod("DELETE");

	TSharedRef<IHttpRequest> StopMatchmakingRequest = HttpModule->CreateRequest();
	StopMatchmakingRequest->SetURL(InvokeUrl + InvokeResource);
	StopMatchmakingRequest->SetVerb(InvokeMethod);
	StopMatchmakingRequest->SetHeader("x-api-key", ApiKey);
	StopMatchmakingRequest->SetHeader("Authorization", AccessToken);
	StopMatchmakingRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnStopMatchmakingResponseReceived);
	StopMatchmakingRequest->ProcessRequest();

	return OnStopMatchmakingResponse;
}

void UGameLiftClient::OnStopMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	UE_LOG(LogGameLiftClient, Warning, TEXT("GameLiftClient::OnStopMatchmakingResponseReceived, ResponseCode: %d"), Response->GetResponseCode());
	
	if (bConnectedSuccessfully)
	{
		TicketId = "";
		OnStopMatchmakingResponse.ExecuteIfBound();
	}
}

FOnPollMatchmakingResponse& UGameLiftClient::PollMatchmaking()
{
	const FString InvokeResource("/matchmaking/" + TicketId);
	const FString InvokeMethod("GET");

	TSharedRef<IHttpRequest> PollMatchmakingRequest = HttpModule->CreateRequest();
	PollMatchmakingRequest->SetURL(InvokeUrl + InvokeResource);
	PollMatchmakingRequest->SetVerb(InvokeMethod);
	PollMatchmakingRequest->SetHeader("x-api-key", ApiKey);
	PollMatchmakingRequest->SetHeader("Authorization", AccessToken);
	PollMatchmakingRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnPollMatchmakingResponseReceived);
	PollMatchmakingRequest->ProcessRequest();

	return OnPollMatchmakingResponse;
}

void UGameLiftClient::OnPollMatchmakingResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	UE_LOG(LogGameLiftClient, Warning, TEXT("GameLiftClient::OnPollMatchmakingResponseReceived, ResponseCode: %d"), Response->GetResponseCode());
	
	if (bConnectedSuccessfully)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(JsonReader, JsonObject))
		{
			if (JsonObject->HasField("ticketId") && JsonObject->HasField("ticketType"))
			{
				// TSharedPtr<FJsonObject> Ticket = JsonObject->GetObjectField("ticket");
				// FString TicketId = JsonObject->GetStringField("ticketId");
				FString TicketType = JsonObject->GetStringField("ticketType");

				TicketId = "";

				if (TicketType.Equals("MatchmakingSucceeded"))
				{
					if (JsonObject->HasField("players") && JsonObject->HasField("gameSessionInfo"))
					{
						FString PlayerSessionId;
						TArray<TSharedPtr<FJsonValue>> Players = JsonObject->GetArrayField("players");
						for (TSharedPtr<FJsonValue> player : Players)
						{
							if (player->AsObject()->GetStringField("playerId") == PlayerId)
							{
								PlayerSessionId = player->AsObject()->GetStringField("playerSessionId");
							}
						}
						
						TSharedPtr<FJsonObject> GameSessionInfo = JsonObject->GetObjectField("gameSessionInfo");
						FString IpAddress = GameSessionInfo->GetStringField("ipAddress");
						FString Port = GameSessionInfo->GetStringField("port");

						OnPollMatchmakingResponse.ExecuteIfBound(TicketType, PlayerId, PlayerSessionId, IpAddress, Port);
					}
				}
				else
				{
					OnPollMatchmakingResponse.ExecuteIfBound(TicketType, "", "", "", "");
				}
			}
		}
	}
}

FOnTestLatencyResponse& UGameLiftClient::TestLatency()
{
	LatencyMapCache.Empty();
	
	for (FString RegionCode : RegionCodes)
	{
		TSharedRef<IHttpRequest> TestLatencyRequest = HttpModule->CreateRequest();
		TestLatencyRequest->SetURL("https://gamelift." + RegionCode + ".amazonaws.com");
		TestLatencyRequest->SetVerb("GET");
		TestLatencyRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnTestLatencyResponseReceived);
		TestLatencyRequest->ProcessRequest();
	}

	return OnTestLatencyResponse;
}

void UGameLiftClient::OnTestLatencyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully)
	{
		// Latency in milliseconds
		float ResponseTime = Request->GetElapsedTime() * 1000;
		for (FString RegionCode : RegionCodes)
		{
			if (Request->GetURL().Contains(RegionCode))
			{
				LatencyMapCache.Add(RegionCode, ResponseTime);
				if (LatencyMapCache.Num() == RegionCodes.Num())
				{
					// begin
					for (const TPair<FString, float>& Pair : LatencyMapCache)
					{
						TSharedPtr<LatencyList> PlayerLatencyList = *LatencyListPerRegion.Find(Pair.Key);

						if (PlayerLatencyList->Num() >= 5) {
							PlayerLatencyList->RemoveNode(PlayerLatencyList->GetHead());
						}
						PlayerLatencyList->AddTail(Pair.Value);

						//
						float TotalPlayerLatency = 0.0f;
						for (float PlayerLatency : *PlayerLatencyList) {
							TotalPlayerLatency += PlayerLatency;
						}
						float AveragePlayerLatency = TotalPlayerLatency / PlayerLatencyList->Num();
						AverageLatencyPerRegion[Pair.Key] = AveragePlayerLatency;
					}
					// end
					
					OnTestLatencyResponse.ExecuteIfBound(AverageLatencyPerRegion);
				}
				break;
			}
		}
	}
}
