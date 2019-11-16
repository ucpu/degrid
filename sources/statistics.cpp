#include "game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/spatial.h>
#include <cage-core/hashString.h>

#include <cage-engine/window.h>
#include <cage-engine/engineProfiling.h>

globalStatisticsStruct statistics;

globalStatisticsStruct::globalStatisticsStruct()
{
	detail::memset(this, 0, sizeof(*this));
	timeRenderMin = -1;
}

namespace
{
	void engineUpdate()
	{
		OPTICK_EVENT("statistics");

		statistics.updateIterationIgnorePause++;
		if (!game.paused)
			statistics.updateIteration++;

		if (game.gameOver)
			return;

		statistics.shotsCurrent = shotComponent::component->group()->count();
		statistics.shotsMax = max(statistics.shotsMax, statistics.shotsCurrent);
		statistics.monstersCurrent = monsterComponent::component->group()->count();
		statistics.monstersMax = max(statistics.monstersMax, statistics.monstersCurrent);
		statistics.entitiesCurrent = entities()->group()->count();
		statistics.entitiesMax = max(statistics.entitiesMax, statistics.entitiesCurrent);
		statistics.timeRenderCurrent = engineProfilingValues(engineProfilingStatsFlags::FrameTime, engineProfilingModeEnum::Last);
		if (statistics.updateIterationIgnorePause > 1000)
		{
			statistics.timeRenderMin = min(statistics.timeRenderMin, statistics.timeRenderCurrent);
			statistics.timeRenderMax = max(statistics.timeRenderMax, statistics.timeRenderCurrent);
		}
		statistics.soundEffectsCurrent = voiceComponent::component->group()->count();
		statistics.soundEffectsMax = max(statistics.soundEffectsMax, statistics.soundEffectsCurrent);
	}

	void gameStart()
	{
		statistics = globalStatisticsStruct();
		statistics.timeStart = getApplicationTime();
	}

	void gameStop()
	{
		static const string powerupName[(uint32)powerupTypeEnum::Total] = {
			"Bomb",
			"Turret",
			"Decoy",
			"HomingShots",
			"SuperDamage",
			"Shield",
			"MaxSpeed",
			"Acceleration",
			"ShotsDamage",
			"ShotsSpeed",
			"Shooting",
			"Multishot",
			"Armor",
			"Duration",
			"Coin"
		};
		for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			if (game.powerups[i] > 0)
				CAGE_LOG(severityEnum::Info, "statistics", stringizer() + "powerup '" + powerupName[i] + "': " + game.powerups[i]);

#define GCHL_GENERATE(N) if (statistics.N != 0) CAGE_LOG(severityEnum::Info, "statistics", stringizer() + CAGE_STRINGIZE(N) ": " + statistics.N);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, \
			shotsFired, shotsTurret, shotsHit, shotsKill, shotsCurrent, shotsMax, \
			monstersSpawned, monstersMutated, monstersMutations, monstersSucceded, monstersCurrent, monstersMax, monstersCurrentSpawningPriority, monstersFirstHit, \
			shielderStoppedShots, \
			wormholesSpawned, wormholeJumps, \
			powerupsSpawned, coinsSpawned, powerupsPicked, powerupsWasted, \
			bombsUsed, bombsHitTotal, bombsKillTotal, bombsHitMax, bombsKillMax \
		));
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, \
			shieldStoppedMonsters, shieldAbsorbedDamage, turretsPlaced, decoysUsed, \
			entitiesCurrent, entitiesMax, \
			environmentGridMarkers, environmentExplosions, \
			keyPressed, buttonPressed, \
			updateIteration, updateIterationIgnorePause, frameIteration, \
			timeRenderMin, timeRenderMax, timeRenderCurrent, \
			soundEffectsCurrent, soundEffectsMax \
		));
#undef GCHL_GENERATE

		uint64 duration = getApplicationTime() - statistics.timeStart;
		CAGE_LOG(severityEnum::Info, "statistics", stringizer() + "duration: " + (duration / 1e6) + " s");
		CAGE_LOG(severityEnum::Info, "statistics", stringizer() + "average UPS: " + (1e6 * statistics.updateIterationIgnorePause / duration));
		CAGE_LOG(severityEnum::Info, "statistics", stringizer() + "average FPS: " + (1e6 * statistics.frameIteration / duration));
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
		eventListener<void()> gameStopListener;
	public:
		callbacksClass() : engineUpdateListener("statistics"), gameStartListener("statistics"), gameStopListener("statistics")
		{
			engineUpdateListener.attach(controlThread().update, -60);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), -60);
			gameStartListener.bind<&gameStart>();
			gameStopListener.attach(gameStopEvent(), 60);
			gameStopListener.bind<&gameStop>();
		}
	} callbacksInstance;
}
