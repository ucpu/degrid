#include <cage-core/config.h>
#include <cage-core/assetManager.h>
#include <cage-engine/highPerformanceGpuHint.h>
#include <cage-simple/engine.h>
#include <cage-simple/statisticsGui.h>
#include <cage-simple/fullscreenSwitcher.h>

#include "game.h"
#include "screens/screens.h"

ConfigUint32 confLanguage("degrid/language/language", 0);

void reloadLanguage(uint32 index);

namespace
{
	uint32 loadedLanguageHash;
	uint32 currentLanguageHash;

	void windowClose(InputWindow)
	{
		engineStop();
	}

	void assetsUpdate()
	{
		if (currentLanguageHash != loadedLanguageHash)
		{
			if (loadedLanguageHash)
				engineAssets()->remove(loadedLanguageHash);
			loadedLanguageHash = currentLanguageHash;
			if (loadedLanguageHash)
				engineAssets()->add(loadedLanguageHash);
		}
	}

	void frameCounter()
	{
		statistics.frameIteration++;
	}
}

void reloadLanguage(uint32 index)
{
	constexpr const uint32 Languages[] = {
		HashString("degrid/languages/english.textpack"),
		HashString("degrid/languages/czech.textpack")
	};
	if (index < sizeof(Languages) / sizeof(Languages[0]))
		currentLanguageHash = Languages[index];
	else
		currentLanguageHash = 0;
}

int main(int argc, const char *args[])
{
	try
	{
		configSetBool("cage/config/autoSave", true);
		engineInitialize(EngineCreateConfig());
		controlThread().updatePeriod(1000000 / 30);

		InputListener<InputClassEnum::WindowClose, InputWindow> closeListener;
		closeListener.attach(engineWindow()->events, 1000);
		closeListener.bind<&windowClose>();
		EventListener<void()> assetsUpdateListener;
		assetsUpdateListener.bind<&assetsUpdate>();
		assetsUpdateListener.attach(controlThread().update);
		EventListener<void()> frameCounterListener;
		frameCounterListener.bind<&frameCounter>();
		frameCounterListener.attach(graphicsPrepareThread().prepare);

		engineWindow()->title("Degrid");
		reloadLanguage(confLanguage);
		engineAssets()->add(HashString("degrid/degrid.pack"));

		{
			Holder<FullscreenSwitcher> fullscreen = newFullscreenSwitcher({});
			Holder<StatisticsGui> engineStatistics = newStatisticsGui();
			engineStatistics->statisticsScope = StatisticsGuiScopeEnum::None;
			engineStatistics->screenPosition = Vec2(0.5);

			engineStart();
		}

		engineAssets()->remove(HashString("degrid/degrid.pack"));
		if (loadedLanguageHash)
			engineAssets()->remove(loadedLanguageHash);

		engineFinalize();
		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}
