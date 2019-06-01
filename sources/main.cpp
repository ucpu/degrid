#include <exception>

#include "game.h"
#include "screens/screens.h"

#include <cage-core/config.h>
#include <cage-core/assetManager.h>

#include <cage-client/engineProfiling.h>
#include <cage-client/highPerformanceGpuHint.h>

configUint32 confLanguage("degrid.language.language", 0);

void reloadLanguage(uint32 index);

namespace
{
	uint32 loadedLanguageHash;
	uint32 currentLanguageHash;

	configUint32 confWindowLeft("degrid.window.left", 100);
	configUint32 confWindowTop("degrid.window.top", 100);
	configUint32 confWindowWidth("degrid.window.width", 0);
	configUint32 confWindowHeight("degrid.window.height", 0);
	configUint32 confFullscreenWidth("degrid.fullscreen.width", 0);
	configUint32 confFullscreenHeight("degrid.fullscreen.height", 0);
	configBool confFullscreenEnabled("degrid.fullscreen.enabled", true);

	bool windowClose()
	{
		engineStop();
		return true;
	}

	bool windowMove(const pointStruct &pos)
	{
		if (window()->isWindowed())
		{
			confWindowLeft = pos.x;
			confWindowTop = pos.y;
		}
		return false;
	}

	bool windowResize(const pointStruct &size)
	{
		if (window()->isWindowed())
		{
			confWindowWidth = size.x;
			confWindowHeight = size.y;
		}
		else if (window()->isMaximized())
		{
			confWindowWidth = 0;
			confWindowHeight = 0;
		}
		return false;
	}

	void setWindowFullscreen(bool fullscreen)
	{
		if (fullscreen)
		{
			try
			{
				detail::overrideBreakpoint ob;
				window()->setFullscreen(pointStruct(confFullscreenWidth, confFullscreenHeight));
				confFullscreenEnabled = true;
			}
			catch (...)
			{
				setWindowFullscreen(false);
			}
		}
		else
		{
			confFullscreenEnabled = false;
			if (confWindowWidth == 0)
				window()->setMaximized();
			else
			{
				window()->setWindowed();
				window()->windowedPosition(pointStruct(confWindowLeft, confWindowTop));
				window()->windowedSize(pointStruct(confWindowWidth, confWindowHeight));
			}
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
		controlThread().timePerTick = 1000000 / 30;

		engineInitialize(engineCreateConfig());

		listeners.attachAll(window(), 1000);
		listeners.windowClose.bind<&windowClose>();
		listeners.windowMove.bind<&windowMove>();
		listeners.windowResize.bind<&windowResize>();
		listeners.keyRelease.bind<&keyRelease>();
		eventListener<void()> assetsUpdateListener;
		assetsUpdateListener.bind<&assetsUpdate>();
		assetsUpdateListener.attach(controlThread().assets);
		eventListener<void()> frameCounterListener;
		frameCounterListener.bind<&frameCounter>();
		frameCounterListener.attach(graphicsPrepareThread().prepare);

		window()->title("Degrid");
		setWindowFullscreen(confFullscreenEnabled);
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
