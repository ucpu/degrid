#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/utility/spatial.h>
#include <cage-core/utility/hashString.h>

#include <cage-client/core.h>
#include <cage-client/engine.h>
#include <cage-client/window.h>
#include <cage-client/utility/engineProfiling.h>

#include "game.h"
#include "screens.h"

namespace grid
{
	componentClass *gridComponent::component;
	componentClass *effectComponent::component;
	componentClass *shotComponent::component;
	componentClass *monsterComponent::component;
	componentClass *simpleMonsterComponent::component;
	componentClass *snakeHeadComponent::component;
	componentClass *snakeTailComponent::component;
	componentClass *shielderComponent::component;
	componentClass *shieldComponent::component;
	componentClass *wormholeComponent::component;
	componentClass *powerupComponent::component;
	componentClass *turretComponent::component;
	componentClass *decoyComponent::component;

	groupClass *entitiesToDestroy;
	holder<spatialDataClass> spatialData;
	holder<spatialQueryClass> spatialQuery;
	quat skyboxOrientation;
	quat skyboxRotation;
	globalPlayerStruct player;
	globalStatisticsStruct statistics;

	configUint32 confControlMovement("grid.control.movement", 0);
	configUint32 confControlFiring("grid.control.firing", 4);
	configUint32 confControlBomb("grid.control.bomb", 4 + 4);
	configUint32 confControlTurret("grid.control.turret", 2 + 4);
	configUint32 confControlDecoy("grid.control.decoy", 0 + 4);
	configString confPlayerName("grid.player.name", "player name");
	configFloat confPlayerShotColorR("grid.player.shotColorR", 1);
	configFloat confPlayerShotColorG("grid.player.shotColorG", 1);
	configFloat confPlayerShotColorB("grid.player.shotColorB", 1);
	configFloat confVolumeMusic("grid.volume.music", 0.5f);
	configFloat confVolumeEffects("grid.volume.effects", 0.5f);
	configFloat confVolumeSpeech("grid.volume.speech", 0.7f);

	namespace
	{
		bool shielderEliminated(entityClass *e)
		{
			GRID_GET_COMPONENT(shielder, sh, e);
			if (entities()->hasEntity(sh.shieldEntity))
				entities()->getEntity(sh.shieldEntity)->addGroup(entitiesToDestroy);
			return false;
		}

		eventListener<bool(entityClass*)> shielderEliminatedListener;
	}

	void controlInitialize()
	{
#ifdef GRID_TESTING
		CAGE_LOG(severityEnum::Info, "grid", string() + "TESTING GAME BUILD");
#endif // GRID_TESTING
		gridComponent::component = entities()->defineComponent(gridComponent(), true);
		effectComponent::component = entities()->defineComponent(effectComponent(), true);
		shotComponent::component = entities()->defineComponent(shotComponent(), true);
		monsterComponent::component = entities()->defineComponent(monsterComponent(), true);
		simpleMonsterComponent::component = entities()->defineComponent(simpleMonsterComponent(), true);
		snakeTailComponent::component = entities()->defineComponent(snakeTailComponent(), true);
		snakeHeadComponent::component = entities()->defineComponent(snakeHeadComponent(), true);
		shielderComponent::component = entities()->defineComponent(shielderComponent(), true);
		shieldComponent::component = entities()->defineComponent(shieldComponent(), true);
		wormholeComponent::component = entities()->defineComponent(wormholeComponent(), true);
		powerupComponent::component = entities()->defineComponent(powerupComponent(), true);
		turretComponent::component = entities()->defineComponent(turretComponent(), true);
		decoyComponent::component = entities()->defineComponent(decoyComponent(), true);
		shielderEliminatedListener.bind<&shielderEliminated>();
		shielderComponent::component->getComponentEntities()->entityRemoved.attach(shielderEliminatedListener);
		entitiesToDestroy = entities()->defineGroup();
		skyboxOrientation = randomDirectionQuat();
		skyboxRotation = interpolate(quat(), randomDirectionQuat(), 5e-5);
		gameStart(true);
	}

	void controlFinalize()
	{}

	void gameFinish()
	{
		CAGE_LOG(severityEnum::Info, "grid", string() + "game over, score: " + player.score);

		{
			uint32 sounds[] = {
				hashString("grid/speech/over/game-over.wav"),
				hashString("grid/speech/over/lets-try-again.wav"),
				hashString("grid/speech/over/oh-no.wav"),
				hashString("grid/speech/over/pitty.wav"),
				hashString("grid/speech/over/thats-it.wav"),
				0 };
			soundSpeech(sounds);
		}

		player.paused = player.gameOver = true;
		playerDone();
		monstersDone();

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
			powerupsSpawned, powerupsTimedout, powerupsPicked, powerupsWasted, \
			bombsUsed, bombsHitTotal, bombsKillTotal, bombsHitMax, bombsKillMax, \
			shieldStoppedMonsters, shieldAbsorbedDamage, turretsPlaced, decoysUsed, \
			entitiesCurrent, entitiesMax, \
			environmentGridMarkers, environmentExplosions, \
			keyPressed, buttonPressed, \
			updateIterationPaused, updateIterationNoPause, frameIteration, \
			timeRenderMin, timeRenderMax, timeRenderCurrent, \
			soundEffectsCurrent, soundEffectsMax \
		));
#undef GCHL_GENERATE
		uint64 duration = getApplicationTime() - statistics.timeStart;
		CAGE_LOG(severityEnum::Info, "statistics", string() + "duration: " + (duration / 1e6) + " s");
		CAGE_LOG(severityEnum::Info, "statistics", string() + "average TPS: " + (1e6 * statistics.updateIterationNoPause / duration));
		CAGE_LOG(severityEnum::Info, "statistics", string() + "average FPS: " + (1e6 * statistics.frameIteration / duration));

		setScreenGameover(grid::player.score);
	}

	void controlUpdate()
	{
		statistics.updateIterationNoPause++;
		if (!player.paused)
			statistics.updateIterationPaused++;
		skyboxOrientation = skyboxRotation * skyboxOrientation;

		powerupUpdate();
		playerUpdate();
		monstersUpdate();
		environmentUpdate();

		entitiesToDestroy->destroyAllEntities();

		if (player.gameOver)
			return;

		{ // update spatial
			spatialData->clear();
			for (entityClass *e : entities()->getAllEntities()->entities())
			{
				uint32 n = e->getName();
				if (n && e->hasComponent(transformComponent::component))
				{
					ENGINE_GET_COMPONENT(transform, tr, e);
					spatialData->update(n, aabb(tr.position - tr.scale, tr.position + tr.scale));
				}
			}
			spatialData->rebuild();
		}

		statistics.shotsCurrent = shotComponent::component->getComponentEntities()->entitiesCount();
		statistics.shotsMax = max(statistics.shotsMax, statistics.shotsCurrent);
		statistics.monstersCurrent = monsterComponent::component->getComponentEntities()->entitiesCount() - snakeTailComponent::component->getComponentEntities()->entitiesCount();
		statistics.monstersMax = max(statistics.monstersMax, statistics.monstersCurrent);
		statistics.entitiesCurrent = entities()->getAllEntities()->entitiesCount();
		statistics.entitiesMax = max(statistics.entitiesMax, statistics.entitiesCurrent);
		statistics.timeRenderCurrent = engineProfilingValues(engineProfilingStatsFlags::FrameTime, engineProfilingModeEnum::Last);
		if (statistics.updateIterationNoPause > 1000)
		{
			statistics.timeRenderMin = min(statistics.timeRenderMin, statistics.timeRenderCurrent);
			statistics.timeRenderMax = max(statistics.timeRenderMax, statistics.timeRenderCurrent);
		}

		if (player.cinematic)
			return;

		if (player.life <= 1e-5)
			gameFinish();
	}

	void gameStart(bool cinematicParam)
	{
		CAGE_LOG(severityEnum::Info, "grid", string() + "new game, cinematic: " + cinematicParam);

		entities()->getAllEntities()->destroyAllEntities();
		detail::memset(&player, 0, sizeof(player));
		detail::memset(&statistics, 0, sizeof(statistics));

		statistics.timeRenderMin = -1;
		player.paused = player.gameOver = false;
		player.cinematic = cinematicParam;
		player.mapNoPullRadius = 200;
		player.scale = 3;
		player.life = 100;
		player.deathColor = vec3(0.68, 0.578, 0.252);
		player.powerupSpawnChance = 0.3;

#ifdef GRID_TESTING
		if (!player.cinematic)
		{
			player.life = 1000000;
			/*
			for (uint32 i = 0; i < 100; i++)
			{
				rads ang = randomAngle();
				powerupSpawn(vec3(sin(ang), 0, cos(ang)) * 30);
			}
			*/
			for (uint32 i = 0; i < (uint32)powerupTypeEnum::Total; i++)
			{
				switch (powerupMode[i])
				{
				case 0: player.powerups[i] = 10; break;
				case 2: player.powerups[i] = 2; break;
				}
			}
		}
#endif

		spatialData = newSpatialData(spatialDataCreateConfig());
		spatialQuery = newSpatialQuery(spatialData.get());

		environmentInit();
		playerInit();
		monstersInit();

		if (!cinematicParam)
		{
			uint32 sounds[] = {
				hashString("grid/speech/starts/enemies-are-approaching.wav"),
				hashString("grid/speech/starts/enemy-is-approaching.wav"),
				hashString("grid/speech/starts/its-a-trap.wav"),
				hashString("grid/speech/starts/lets-do-this.wav"),
				hashString("grid/speech/starts/let-us-kill-some-.wav"),
				hashString("grid/speech/starts/ready-set-go.wav"),
				0
			};
			soundSpeech(sounds);
		}

		statistics.timeStart = getApplicationTime();
	}
}
