#include <exception>

#include "game.h"
#include "screens/screens.h"

#include <cage-core/config.h>
#include <cage-core/assetManager.h>

#include <cage-engine/engineProfiling.h>
#include <cage-engine/fullscreenSwitcher.h>
#include <cage-engine/highPerformanceGpuHint.h>

ConfigUint32 confLanguage("degrid/language/language", 0);

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

	bool keyRelease(uint32 key, uint32, ModifiersFlags modifiers)
	{
		if (modifiers != ModifiersFlags::None)
			return false;

		static ConfigBool secondaryCamera("degrid/secondaryCamera/enabled", false);

		CAGE_LOG_DEBUG(SeverityEnum::Info, "keyboard", stringizer() + "key: " + key);

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

	WindowEventListeners listeners;
}

void reloadLanguage(uint32 index)
{
	static const uint32 languages[] = {
		HashString("degrid/languages/english.textpack"),
		HashString("degrid/languages/czech.textpack")
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
		configSetBool("cage/config/autoSave", true);
		controlThread().timePerTick = 1000000 / 30;
		engineInitialize(EngineCreateConfig());

		listeners.attachAll(window(), 1000);
		listeners.windowClose.bind<&windowClose>();
		listeners.keyRelease.bind<&keyRelease>();
		EventListener<void()> assetsUpdateListener;
		assetsUpdateListener.bind<&assetsUpdate>();
		assetsUpdateListener.attach(controlThread().assets);
		EventListener<void()> frameCounterListener;
		frameCounterListener.bind<&frameCounter>();
		frameCounterListener.attach(graphicsPrepareThread().prepare);

		window()->title("Degrid");
		reloadLanguage(confLanguage);
		assets()->add(HashString("degrid/degrid.pack"));

		{
			Holder<FullscreenSwitcher> fullscreen = newFullscreenSwitcher({});
			Holder<EngineProfiling> EngineProfiling = newEngineProfiling();
			EngineProfiling->profilingScope = EngineProfilingScopeEnum::None;
			EngineProfiling->screenPosition = vec2(0.5);

			engineStart();
		}

		assets()->remove(HashString("degrid/degrid.pack"));
		if (loadedLanguageHash)
			assets()->remove(loadedLanguageHash);

		engineFinalize();
		return 0;
	}
	catch (const cage::Exception &e)
	{
		CAGE_LOG(SeverityEnum::Note, "exception", e.message);
		CAGE_LOG(SeverityEnum::Error, "degrid", "caught cage exception in main");
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(SeverityEnum::Note, "exception", e.what());
		CAGE_LOG(SeverityEnum::Error, "degrid", "caught std exception in main");
	}
	catch (...)
	{
		CAGE_LOG(SeverityEnum::Error, "degrid", "caught unknown exception in main");
	}
	return 1;
}
