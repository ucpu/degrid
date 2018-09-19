#include "game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/spatial.h>
#include <cage-core/hashString.h>

#include <cage-client/window.h>
#include <cage-client/engineProfiling.h>

globalStatisticsStruct statistics;

globalStatisticsStruct::globalStatisticsStruct()
{
	detail::memset(this, 0, sizeof(*this));
	timeRenderMin = -1;
}

namespace
{
	void countActiveSounds()
	{
		statistics.soundEffectsCurrent = 0;
		for (entityClass *e : voiceComponent::component->getComponentEntities()->entities())
		{
			statistics.soundEffectsCurrent++;
		}
		statistics.soundEffectsMax = max(statistics.soundEffectsMax, statistics.soundEffectsCurrent);
	}

	void engineUpdate()
	{
		statistics.updateIterationIgnorePause++;
		if (!player.paused)
			statistics.updateIterationWithPause++;

		if (player.gameOver)
			return;

		statistics.shotsCurrent = shotComponent::component->getComponentEntities()->entitiesCount();
		statistics.shotsMax = max(statistics.shotsMax, statistics.shotsCurrent);
		statistics.monstersCurrent = monsterComponent::component->getComponentEntities()->entitiesCount() - snakeTailComponent::component->getComponentEntities()->entitiesCount();
		statistics.monstersMax = max(statistics.monstersMax, statistics.monstersCurrent);
		statistics.entitiesCurrent = entities()->getAllEntities()->entitiesCount();
		statistics.entitiesMax = max(statistics.entitiesMax, statistics.entitiesCurrent);
		statistics.timeRenderCurrent = engineProfilingValues(engineProfilingStatsFlags::FrameTime, engineProfilingModeEnum::Last);
		if (statistics.updateIterationIgnorePause > 1000)
		{
			statistics.timeRenderMin = min(statistics.timeRenderMin, statistics.timeRenderCurrent);
			statistics.timeRenderMax = max(statistics.timeRenderMax, statistics.timeRenderCurrent);
		}
		countActiveSounds();
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
			"Multishot"
		};
		for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			if (player.powerups[i] > 0)
				CAGE_LOG(severityEnum::Info, "statistics", string() + "powerup '" + powerupName[i] + "': " + player.powerups[i]);

		CAGE_LOG(severityEnum::Info, "statistics", string() + "shots dissipation ratio: " + (1.f * statistics.shotsDissipated / (statistics.shotsFired + statistics.shotsTurret - statistics.shotsCurrent)));
#define GCHL_GENERATE(N) if (statistics.N != 0) CAGE_LOG(severityEnum::Info, "statistics", string() + CAGE_STRINGIZE(N) ": " + statistics.N);
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, \
			shotsFired, shotsTurret, shotsDissipated, shotsHit, shotsKill, shotsCurrent, shotsMax, \
			monstersSpawned, monstersSpecial, monstersSucceded, monstersCurrent, monstersMax, monstersCurrentSpawningPriority, monstersFirstHit, shielderStoppedShots, \
			wormholesSpawned, \
			powerupsSpawned, powerupsPicked, powerupsWasted, \
			bombsUsed, bombsHitTotal, bombsKillTotal, bombsHitMax, bombsKillMax, \
			shieldStoppedMonsters, shieldAbsorbedDamage, turretsPlaced, decoysUsed, \
			entitiesCurrent, entitiesMax, \
			environmentGridMarkers, environmentExplosions, \
			keyPressed, buttonPressed, \
			updateIterationWithPause, updateIterationIgnorePause, frameIteration, \
			timeRenderMin, timeRenderMax, timeRenderCurrent, \
			soundEffectsCurrent, soundEffectsMax \
		));
#undef GCHL_GENERATE
		uint64 duration = getApplicationTime() - statistics.timeStart;
		CAGE_LOG(severityEnum::Info, "statistics", string() + "duration: " + (duration / 1e6) + " s");
		CAGE_LOG(severityEnum::Info, "statistics", string() + "average UPS: " + (1e6 * statistics.updateIterationIgnorePause / duration));
		CAGE_LOG(severityEnum::Info, "statistics", string() + "average FPS: " + (1e6 * statistics.frameIteration / duration));
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
		eventListener<void()> gameStopListener;
	public:
		callbacksClass()
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
