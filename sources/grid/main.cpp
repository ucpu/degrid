#include <exception>

#include "includes.h"
#include "screens.h"
#include "game.h"
#include <cage-client/utility/engineProfiling.h>
#include <cage-client/utility/highPerformanceGpuHint.h>

configUint32 confLanguage("grid.language.language", 0);

void reloadLanguage(uint32 index);

namespace
{
	uint32 loadedLanguageHash;
	uint32 currentLanguageHash;

	bool windowClose(windowClass *)
	{
		engineStop();
		return true;
	}

	void setWindowFullscreen(bool fullscreen)
	{
		static configUint32 ww("grid.window.width", 1280);
		static configUint32 wh("grid.window.height", 720);
		static configUint32 fw("grid.fullscreen.width", 0);
		static configUint32 fh("grid.fullscreen.height", 0);
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

	bool keyRelease(windowClass *, uint32 key, uint32, modifiersFlags modifiers)
	{
		static configBool secondaryCamera("grid.secondary-camera.enabled", false);

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

	bool update(uint64 time)
	{
		grid::controlUpdate(time);
		if (!grid::player.gameOver && !grid::player.cinematic)
			grid::gameGuiUpdate();
		return false;
	}

	bool frame()
	{
		grid::statistics.frameIteration++;
		return false;
	}

	bool assetsUpdate()
	{
		if (currentLanguageHash != loadedLanguageHash)
		{
			if (loadedLanguageHash)
				assets()->remove(loadedLanguageHash);
			loadedLanguageHash = currentLanguageHash;
			if (loadedLanguageHash)
				assets()->add(loadedLanguageHash);
		}
		return false;
	}

	bool soundInitialize()
	{
		grid::soundsInit();
		return false;
	}

	bool soundFinalize()
	{
		grid::soundsDone();
		return false;
	}

	bool soundUpdate()
	{
		grid::soundUpdate();
		return false;
	}
}

void reloadLanguage(uint32 index)
{
	static const uint32 languages[] = {
		hashString("grid/languages/english.textpack"),
		hashString("grid/languages/czech.textpack")
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
		if (newFilesystem()->exists("options.ini"))
			configLoadIni("options.ini", "grid");

		//configSetBool("cage-client.debug.shaderIntrospection", true);
		configSetBool("cage-client.debug.engineRenderMissingMeshes", true);
		controlThread::tickTime = 1000000 / 30;

		engineInitialize(engineCreateConfig());

		eventListener<bool(windowClass*)> windowCloseListener;
		eventListener<bool(windowClass *, uint32 key, uint32, modifiersFlags modifiers)> keyReleaseListener;
		eventListener<bool(uint64)> updateListener;
		eventListener<bool()> frameListener;
		eventListener<bool()> assetsUpdateListener;
		eventListener<bool()> soundInitializeListener;
		eventListener<bool()> soundFinalizeListener;
		eventListener<bool()> soundUpdateListener;

		updateListener.bind<&update>();
		frameListener.bind<&frame>();
		assetsUpdateListener.bind<&assetsUpdate>();
		soundInitializeListener.bind<&soundInitialize>();
		soundFinalizeListener.bind<&soundFinalize>();
		soundUpdateListener.bind<&soundUpdate>();
		windowCloseListener.bind<&windowClose>();
		keyReleaseListener.bind<&keyRelease>();

		updateListener.attach(controlThread::update);
		frameListener.attach(graphicPrepareThread::prepare);
		assetsUpdateListener.attach(controlThread::assets);
		soundInitializeListener.attach(soundThread::initialize);
		soundFinalizeListener.attach(soundThread::finalize);
		soundUpdateListener.attach(soundThread::sound);
		window()->events.windowClose.attach(windowCloseListener);
		window()->events.keyRelease.attach(keyReleaseListener);

		window()->title("Grid");
		setWindowFullscreen(configGetBool("grid.fullscreen.enabled"));
		reloadLanguage(confLanguage);
		grid::controlInitialize();
		setScreenMainmenu();
		assets()->add(hashString("grid/grid.pack"));

		holder<engineProfilingClass> engineProfiling = newEngineProfiling();
		engineProfiling->profilingMode = profilingModeEnum::None;
		engineProfiling->keyToggleFullscreen = 0;
		engineProfiling->screenPosition = vec2(0.5, 0.5);

		engineStart();

		grid::controlFinalize();
		assets()->remove(hashString("grid/grid.pack"));
		if (loadedLanguageHash)
			assets()->remove(loadedLanguageHash);

		engineProfiling.clear();

		engineFinalize();

		try
		{
			configSaveIni("options.ini", "grid");
		}
		catch (...)
		{
			CAGE_LOG(severityEnum::Warning, "grid", "failed to save game configuration");
		}
		return 0;
	}
	catch (const cage::exception &e)
	{
		CAGE_LOG(severityEnum::Note, "exception", e.message);
		CAGE_LOG(severityEnum::Error, "grid", "caught cage exception in main");
	}
	catch (const std::exception &e)
	{
		CAGE_LOG(severityEnum::Note, "exception", e.what());
		CAGE_LOG(severityEnum::Error, "grid", "caught std exception in main");
	}
	catch (...)
	{
		CAGE_LOG(severityEnum::Error, "grid", "caught unknown exception in main");
	}
	return 1;
}