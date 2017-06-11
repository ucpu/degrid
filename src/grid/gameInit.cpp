#include "includes.h"
#include "screens.h"
#include "game.h"

namespace grid
{
	componentClass *gridStruct::component;
	componentClass *effectStruct::component;
	componentClass *shotStruct::component;
	componentClass *monsterStruct::component;
	componentClass *simpleMonsterStruct::component;
	componentClass *snakeHeadStruct::component;
	componentClass *snakeTailStruct::component;
	componentClass *shielderStruct::component;
	componentClass *shieldStruct::component;
	componentClass *wormholeStruct::component;
	componentClass *powerupStruct::component;
	componentClass *turretStruct::component;
	componentClass *decoyStruct::component;

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
		const bool shielderEliminated(entityClass *e)
		{
			GRID_GET_COMPONENT(shielder, sh, e);
			if (entities()->hasEntity(sh.shieldEntity))
				entities()->getEntity(sh.shieldEntity)->addGroup(entitiesToDestroy);
			return false;
		}

		eventListener<void(entityClass*)> shielderEliminatedListener;
	}

	void controlInitialize()
	{
#ifdef GRID_TESTING
		CAGE_LOG(severityEnum::Info, "grid", string() + "testing game build");
#endif // GRID_TESTING
		gridStruct::component = entities()->defineComponent(gridStruct(), true);
		effectStruct::component = entities()->defineComponent(effectStruct(), true);
		shotStruct::component = entities()->defineComponent(shotStruct(), true);
		monsterStruct::component = entities()->defineComponent(monsterStruct(), true);
		simpleMonsterStruct::component = entities()->defineComponent(simpleMonsterStruct(), true);
		snakeTailStruct::component = entities()->defineComponent(snakeTailStruct(), true);
		snakeHeadStruct::component = entities()->defineComponent(snakeHeadStruct(), true);
		shielderStruct::component = entities()->defineComponent(shielderStruct(), true);
		shieldStruct::component = entities()->defineComponent(shieldStruct(), true);
		wormholeStruct::component = entities()->defineComponent(wormholeStruct(), true);
		powerupStruct::component = entities()->defineComponent(powerupStruct(), true);
		turretStruct::component = entities()->defineComponent(turretStruct(), true);
		decoyStruct::component = entities()->defineComponent(decoyStruct(), true);
		shielderEliminatedListener.bind<&shielderEliminated>();
		shielderStruct::component->getComponentEntities()->entityRemoved.add(shielderEliminatedListener);
		entitiesToDestroy = entities()->defineGroup();
		skyboxOrientation = randomDirectionQuat();
		skyboxRotation = interpolate(quat(), randomDirectionQuat(), 2e-4);
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

		static const string powerupName[puTotal] = {
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
		for (uint32 i = 0; i < puTotal; i++)
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

	void controlUpdate(uint64 uTime)
	{
		player.updateTime = uTime;
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
			uint32 count = entities()->getAllEntities()->entitiesCount();
			entityClass *const *ents = entities()->getAllEntities()->entitiesArray();
			spatialData->clear();
			for (uint32 i = 0; i < count; i++)
			{
				entityClass *e = ents[i];
				uint32 n = e->getName();
				if (n && e->hasComponent(transformStruct::component))
				{
					ENGINE_GET_COMPONENT(transform, tr, e);
					spatialData->update(n, aabb(tr.position - tr.scale, tr.position + tr.scale));
				}
			}
			spatialData->rebuild();
		}

		statistics.shotsCurrent = shotStruct::component->getComponentEntities()->entitiesCount();
		statistics.shotsMax = max(statistics.shotsMax, statistics.shotsCurrent);
		statistics.monstersCurrent = monsterStruct::component->getComponentEntities()->entitiesCount() - snakeTailStruct::component->getComponentEntities()->entitiesCount();
		statistics.monstersMax = max(statistics.monstersMax, statistics.monstersCurrent);
		statistics.entitiesCurrent = entities()->getAllEntities()->entitiesCount();
		statistics.entitiesMax = max(statistics.entitiesMax, statistics.entitiesCurrent);
		statistics.timeRenderCurrent = engineProfiling::getTime(engineProfiling::profilingTimeFlags::FrameTime, false);
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
			for (uint32 i = 0; i < 30; i++)
			{
				rads ang = randomAngle();
				powerupSpawn(vec3(sin(ang), 0, cos(ang)) * 30);
			}
			*/
			for (uint32 i = 0; i < puTotal; i++)
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