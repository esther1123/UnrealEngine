#pragma once

#include "Modules/ModuleManager.h"
#include "GameLiftClient.h"

class FGameLiftClientModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	GAMELIFTCLIENT_API static FGameLiftClientModule& Get();

private:
	/** singleton for the module while loaded and available */
	static FGameLiftClientModule* Singleton;

public:
	UGameLiftClient* GameLiftClient;
};