#include <exception>

#include "includes.h"
#include "screens.h"
#include "game.h"
#include <cage-client/utility/engineProfiling.h>

configUint32 confLanguage("grid.language.language", 0);

void reloadLanguage(uint32 index);

namespace
{
	uint32 loadedLanguageHash;
	uint32 currentLanguageHash;

	const bool applicationQuit()
	{
		engineStop();
		return true;
	}

	const bool windowClose(windowClass *)
	{
		engineStop();
		return true;
	}

	void setWindowMode(bool fullscreen)
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
				setWindowMode(false);
			}
		}
		else
		{
			window()->modeSetWindowed(windowFlags::Border | windowFlags::Resizeable);
			window()->windowedSize(pointStruct(ww, wh));
		}
	}

	const bool keyRelease(windowClass *, uint32 key, uint32, modifiersFlags modifiers)
	{
		static configBool secondaryCamera("grid.secondary-camera.enabled", false);

		CAGE_LOG_DEBUG(severityEnum::Info, "keyboard", string() + "key: " + key);

		switch (key)
		{
		case 298: // F9
			secondaryCamera = !(bool)secondaryCamera;
			return true;
		case 300: // F11
			setWindowMode(!window()->isFullscreen());
			return true;
		}
		return false;
	}

	const bool update(uint64 time)
	{
		grid::controlUpdate(time);
		if (!grid::player.gameOver && !grid::player.cinematic)
			grid::gameGuiUpdate();
		return false;
	}

	const bool frame(uint64)
	{
		grid::statistics.frameIteration++;
		return false;
	}

	const bool assetsUpdate()
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

	const bool soundInitialize()
	{
		grid::soundsInit();
		return false;
	}

	const bool soundFinalize()
	{
		grid::soundsDone();
		return false;
	}

	const bool soundUpdate(uint64 time)
	{
		grid::soundUpdate(time);
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

		eventListener<void> applicationQuitListener;
		eventListener<void(windowClass*)> windowCloseListener;
		eventListener<void(windowClass *, uint32 key, uint32, modifiersFlags modifiers)> keyReleaseListener;
		eventListener<void(uint64)> updateListener;
		eventListener<void(uint64)> frameListener;
		eventListener<void> assetsUpdateListener;
		eventListener<void> soundInitializeListener;
		eventListener<void> soundFinalizeListener;
		eventListener<void(uint64)> soundUpdateListener;

		updateListener.bind<&update>();
		frameListener.bind<&frame>();
		assetsUpdateListener.bind<&assetsUpdate>();
		soundInitializeListener.bind<&soundInitialize>();
		soundFinalizeListener.bind<&soundFinalize>();
		soundUpdateListener.bind<&soundUpdate>();
		applicationQuitListener.bind<&applicationQuit>();
		windowCloseListener.bind<&windowClose>();
		keyReleaseListener.bind<&keyRelease>();

		updateListener.attach(controlThread::update);
		frameListener.attach(graphicPrepareThread::prepare);
		assetsUpdateListener.attach(controlThread::assets);
		soundInitializeListener.attach(soundThread::initialize);
		soundFinalizeListener.attach(soundThread::finalize);
		soundUpdateListener.attach(soundThread::sound);
		window()->events.applicationQuit.add(applicationQuitListener);
		window()->events.windowClose.add(windowCloseListener);
		window()->events.keyRelease.add(keyReleaseListener);

		window()->title("Grid");
		setWindowMode(configGetBool("grid.fullscreen.enabled"));
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