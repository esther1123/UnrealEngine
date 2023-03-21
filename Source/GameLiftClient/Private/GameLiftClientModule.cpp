#include "GameLiftClientModule.h"

IMPLEMENT_MODULE(FGameLiftClientModule, GameLiftClient);

FGameLiftClientModule* FGameLiftClientModule::Singleton = nullptr;

void FGameLiftClientModule::StartupModule()
{
	Singleton = this;
	GameLiftClient = NewObject<UGameLiftClient>();
	GameLiftClient->AddToRoot();
}

void FGameLiftClientModule::ShutdownModule()
{
	GameLiftClient->RemoveFromRoot();
	Singleton = nullptr;
}

FGameLiftClientModule& FGameLiftClientModule::Get()
{
	if (Singleton == nullptr)
	{
		check(IsInGameThread());
		FModuleManager::LoadModuleChecked<FGameLiftClientModule>("GameLiftClient");
	}
	check(Singleton != nullptr);
	return *Singleton;
}
