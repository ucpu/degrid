#include <exception>

#include "game.h"
#include "screens/screens.h"

#include <cage-core/config.h>
#include <cage-core/assetManager.h>

#include <cage-engine/engineProfiling.h>
#include <cage-engine/fullscreenSwitcher.h>
#include <cage-engine/highPerformanceGpuHint.h>

configUint32 confLanguage("degrid.language.language", 0);

void reloadLanguage(uint32 index);

namespace
{
	uint32 loadedLanguageHash;
	uint32 currentLanguageHash;

	bool windowClose()
	{
		engineStop();
		return true;
	}

	bool keyRelease(uint32 key, uint32, modifiersFlags modifiers)
	{
		if (modifiers != modifiersFlags::None)
			return false;

		static configBool secondaryCamera("degrid.secondary-camera.enabled", false);

		CAGE_LOG_DEBUG(severityEnum::Info, "keyboard", stringizer() + "key: " + key);

		switch (key)
		{
		case 298: // F9
			secondaryCamera = !(bool)secondaryCamera;
			return true;
		}

		return false;
	}

	void assetsUpdate()
	{
		if (currentLanguageHash != loadedLanguageHash)
		{
			if (loadedLanguageHash)
				assets()->remove(loadedLanguageHash);
			loadedLanguageHash = currentLanguageHash;
			if (loadedLanguageHash)
				assets()->add(loadedLanguageHash);
		}
	}

	void frameCounter()
	{
		statistics.frameIteration++;
	}

	windowEventListeners listeners;
}

void reloadLanguage(uint32 index)
{
	static const uint32 languages[] = {
		hashString("degrid/languages/english.textpack"),
		hashString("degrid/languages/czech.textpack")
	};
	if (index < sizeof(languages) / sizeof(languages[0]))
		currentLanguageHash = languages[index];
	else
		currentLanguageHash = 0;
}

int main(int argc, const char *args[])
{
	try
	{
		configSetBool("cage.config.autoSave", true);
		controlThread().timePerTick = 1000000 / 30;
		engineInitialize(engineCreateConfig());

		listeners.attachAll(window(), 1000);
		listeners.windowClose.bind<&windowClose>();
		listeners.keyRelease.bind<&keyRelease>();
		eventListener<void()> assetsUpdateListener;
		assetsUpdateListener.bind<&assetsUpdate>();
		assetsUpdateListener.attach(controlThread().assets);
		eventListener<void()> frameCounterListener;
		frameCounterListener.bind<&frameCounter>();
		frameCounterListener.attach(graphicsPrepareThread().prepare);

		window()->title("Degrid");
		reloadLanguage(confLanguage);
		assets()->add(hashString("degrid/degrid.pack"));

		{
			holder<fullscreenSwitcher> fullscreen = newFullscreenSwitcher({});
			holder<engineProfiling> engineProfiling = newEngineProfiling();
			engineProfiling->profilingScope = engineProfilingScopeEnum::None;
			engineProfiling->screenPosition = vec2(0.5);

			engineStart();
		}

		assets()->remove(hashString("degrid/degrid.pack"));
		if (loadedLanguageHash)
			assets()->remove(loadedLanguageHash);

		engineFinalize();
		return 0;
	}
	catch (const cage::exception &e)
	{
		CAGE_LOG(severityEnum::Note, "exception", e.message);
		CAGE_LOG(severityEnum::Error, "degrid", "caught cage exception in main");
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(severityEnum::Note, "exception", e.what());
		CAGE_LOG(severityEnum::Error, "degrid", "caught std exception in main");
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "degrid", "caught unknown exception in main");
	}
	return 1;
}
