#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/macros.h>
#include <cage-engine/window.h>
#include <cage-simple/statisticsGui.h>

#include "game.h"

GlobalStatistics statistics;

GlobalStatistics::GlobalStatistics()
{
	detail::memset(this, 0, sizeof(*this));
	timeRenderMin = -1;
}

namespace
{
	void engineUpdate()
	{
		statistics.updateIterationIgnorePause++;
		if (!game.paused)
			statistics.updateIteration++;

		if (game.gameOver)
			return;

		statistics.shotsCurrent = engineEntities()->component<ShotComponent>()->count();
		statistics.shotsMax = max(statistics.shotsMax, statistics.shotsCurrent);
		statistics.monstersCurrent = engineEntities()->component<MonsterComponent>()->count();
		statistics.monstersMax = max(statistics.monstersMax, statistics.monstersCurrent);
		statistics.entitiesCurrent = engineEntities()->group()->count();
		statistics.entitiesMax = max(statistics.entitiesMax, statistics.entitiesCurrent);
		statistics.timeRenderCurrent = engineStatisticsValues(StatisticsGuiFlags::FrameTime, StatisticsGuiModeEnum::Last);
		if (statistics.updateIterationIgnorePause > 1000)
		{
			statistics.timeRenderMin = min(statistics.timeRenderMin, statistics.timeRenderCurrent);
			statistics.timeRenderMax = max(statistics.timeRenderMax, statistics.timeRenderCurrent);
		}
		statistics.soundEffectsCurrent = engineEntities()->component<SoundComponent>()->count();
		statistics.soundEffectsMax = max(statistics.soundEffectsMax, statistics.soundEffectsCurrent);
	}

	void gameStart()
	{
		statistics = GlobalStatistics();
		statistics.timeStart = applicationTime();
	}

	void gameStop()
	{
		static const String PowerupName[(uint32)PowerupTypeEnum::Total] = {
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
		for (uint32 i = 0; i < (uint32)PowerupTypeEnum::Total; i++)
			if (game.powerups[i] > 0)
				CAGE_LOG(SeverityEnum::Info, "statistics", Stringizer() + "powerup '" + PowerupName[i] + "': " + game.powerups[i]);

#define GCHL_GENERATE(N) if (statistics.N != 0) CAGE_LOG(SeverityEnum::Info, "statistics", Stringizer() + CAGE_STRINGIZE(N) ": " + statistics.N);
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

		const uint64 duration = applicationTime() - statistics.timeStart;
		CAGE_LOG(SeverityEnum::Info, "statistics", Stringizer() + "duration: " + (duration / 1e6) + " s");
		CAGE_LOG(SeverityEnum::Info, "statistics", Stringizer() + "average UPS: " + (1e6 * statistics.updateIterationIgnorePause / duration));
		CAGE_LOG(SeverityEnum::Info, "statistics", Stringizer() + "average FPS: " + (1e6 * statistics.frameIteration / duration));
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
		EventListener<void()> gameStartListener;
		EventListener<void()> gameStopListener;
	public:
		Callbacks() : engineUpdateListener("statistics"), gameStartListener("statistics"), gameStopListener("statistics")
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
