#include <exception>

#include "game.h"
#include "screens/screens.h"

#include <cage-core/config.h>
#include <cage-core/assets.h>

#include <cage-client/engineProfiling.h>
#include <cage-client/highPerformanceGpuHint.h>

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

	void setWindowFullscreen(bool fullscreen)
	{
		static configUint32 ww("degrid.window.width", 1280);
		static configUint32 wh("degrid.window.height", 720);
		static configUint32 fw("degrid.fullscreen.width", 0);
		static configUint32 fh("degrid.fullscreen.height", 0);
		if (fullscreen)
		{
			try
			{
				detail::overrideBreakpoint ob;
				window()->modeSetFullscreen(pointStruct(fw, fh));
			}
			catch (...)
			{
				setWindowFullscreen(false);
			}
		}
		else
		{
			window()->modeSetWindowed(windowFlags::Border | windowFlags::Resizeable);
			window()->windowedSize(pointStruct(ww, wh));
		}
	}

	bool keyRelease(uint32 key, uint32, modifiersFlags modifiers)
	{
		if (modifiers != modifiersFlags::None)
			return false;

		static configBool secondaryCamera("degrid.secondary-camera.enabled", false);

		CAGE_LOG_DEBUG(severityEnum::Info, "keyboard", string() + "key: " + key);

		switch (key)
		{
		case 298: // F9
			secondaryCamera = !(bool)secondaryCamera;
			return true;
		case 300: // F11
			setWindowFullscreen(!window()->isFullscreen());
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
		controlThread().timePerTick = 1000000 / 30;

		engineInitialize(engineCreateConfig());

		eventListener<void()> assetsUpdateListener;
		assetsUpdateListener.bind<&assetsUpdate>();
		assetsUpdateListener.attach(controlThread().assets);
		eventListener<void()> frameCounterListener;
		frameCounterListener.bind<&frameCounter>();
		frameCounterListener.attach(graphicsPrepareThread().prepare);
		eventListener<bool()> windowCloseListener;
		windowCloseListener.bind<&windowClose>();
		windowCloseListener.attach(window()->events.windowClose);
		eventListener<bool(uint32 key, uint32, modifiersFlags modifiers)> keyReleaseListener;
		keyReleaseListener.bind<&keyRelease>();
		keyReleaseListener.attach(window()->events.keyRelease);

		window()->title("Degrid");
		setWindowFullscreen(configGetBool("degrid.fullscreen.enabled"));
		reloadLanguage(confLanguage);
		assets()->add(hashString("degrid/degrid.pack"));

		{
			holder<engineProfilingClass> engineProfiling = newEngineProfiling();
			engineProfiling->profilingScope = engineProfilingScopeEnum::None;
			engineProfiling->keyToggleFullscreen = 0;
			engineProfiling->screenPosition = vec2(0.5, 0.5);

			engineStart();
		}

		assets()->remove(hashString("degrid/degrid.pack"));
		if (loadedLanguageHash)
			assets()->remove(loadedLanguageHash);

		engineFinalize();

		try
		{
			configSaveIni("degrid.ini", "degrid");
		}
		catch (...)
		{
			CAGE_LOG(severityEnum::Warning, "degrid", "failed to save game configuration");
		}

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
