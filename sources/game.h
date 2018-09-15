//#define GRID_TESTING

using namespace cage;

namespace grid
{
	void controlInitialize();
	void controlFinalize();
	void controlUpdate();
	void playerInit();
	void playerDone();
	void playerUpdate();
	void physicsUpdate();
	bool collisionTest(const vec3 &positionA, real radiusA, const vec3 velocityA, const vec3 &positionB, real radiusB, const vec3 velocityB);
	void powerupUpdate();
	void powerupSpawn(const vec3 &position);
	void monstersInit();
	void monstersDone();
	void monstersUpdate();
	void monstersSpawnInitial();
	void environmentInit();
	void environmentExplosion(const vec3 &position, const vec3 &velocity, const vec3 &color, real size, real scale);
	void environmentUpdate();
	void monsterExplosion(entityClass *e);
	void shotExplosion(entityClass *e);
	void gameStart(bool cinematic);
	void gameGuiUpdate();
	void soundsInit();
	void soundsDone();
	void soundUpdate();
	void soundEffect(uint32 sound, const vec3 &position);
	void soundSpeech(uint32 sound);
	void soundSpeech(uint32 *sounds);
	vec3 colorVariation(const vec3 &color);

	void mousePress(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
	void mouseRelease(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
	void mouseMove(mouseButtonsFlags buttons, modifiersFlags modifiers, const pointStruct &point);
	void keyPress(uint32 key, modifiersFlags modifiers);
	void keyRelease(uint32 key, modifiersFlags modifiers);

	enum class powerupTypeEnum
	{
		// collectibles
		Bomb,
		Turret,
		Decoy,
		// one-time
		HomingShots,
		SuperDamage,
		Shield,
		// permanents
		MaxSpeed,
		Acceleration,
		ShotsDamage,
		ShotsSpeed,
		FiringSpeed,
		Multishot,
		// total
		Total
	};

	const uint32 powerupMode[(uint32)powerupTypeEnum::Total] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2 };
	const char letters[] = { 'C', 'E', 'F', 'Q', 'R', 'V', 'X', 'Z' };

	extern groupClass *entitiesToDestroy;
	extern holder<spatialDataClass> spatialData;
	extern holder<spatialQueryClass> spatialQuery;
	extern quat skyboxOrientation;
	extern quat skyboxRotation;

	const real playerScale = 3;
	const real mapNoPullRadius = 200;

	struct globalPlayerStruct
	{
		// entities
		entityClass *playerEntity;
		entityClass *shieldEntity;
		entityClass *skyboxRenderEntity;
		entityClass *primaryCameraEntity;
		entityClass *skyboxPrimaryCameraEntity;
		entityClass *secondaryCameraEntity;
		entityClass *skyboxSecondaryCameraEntity;

		// global
		bool cinematic;
		bool paused;
		bool gameOver;
		vec3 monstersTarget;

		// player
		real life;
		real shootingCooldown;
		vec3 shotsColor;
		uint32 score;
		uint32 scorePrevious;
		uint32 powerups[(uint32)powerupTypeEnum::Total];
		vec3 deathColor;
		real powerupSpawnChance;

		// input (options independent)
		vec3 arrowsDirection;
		vec3 mouseCurrentPosition;
		vec3 mouseLeftPosition;
		vec3 mouseRightPosition;

		// ship controls (options dependent)
		vec3 moveDirection;
		vec3 fireDirection;
	};
	extern globalPlayerStruct player;

	struct globalStatisticsStruct
	{
		uint64 timeStart;
		uint64 timeRenderMin; // minimal render time [us]
		uint64 timeRenderMax; // maximal render time [us]
		uint64 timeRenderCurrent; // last frame render time [us]
		uint32 shotsFired; // total number of shots fired by player's ship
		uint32 shotsTurret; // total number of shots fired by turrets
		uint32 shotsDissipated; // total number of shots that went too far away of the map
		uint32 shotsHit; // total number of monsters hit by shots
		uint32 shotsKill; // total number of monsters killed by shots
		uint32 shotsCurrent;
		uint32 shotsMax; // maximum number of shots at any single moment
		uint32 monstersSpawned; // total number of monsters spawned (including special)
		uint32 monstersSpecial; // total number of monsters, who spawned with special augments
		uint32 monstersSucceded; // monsters that hit the player
		uint32 monstersCurrent;
		uint32 monstersMax; // maximum number of monsters at any single moment
		real monstersCurrentSpawningPriority; // current value of variable, that controls monsters spawning
		uint32 monstersFirstHit; // the time (in relation to updateIteration) in which the player was first hit by a monster
		uint32 monstersLastHit;
		uint32 shielderStoppedShots; // the number of shots eliminated by shielder
		uint32 wormholesSpawned;
		uint32 powerupsSpawned;
		uint32 powerupsPicked; // powerups picked up by the player
		uint32 powerupsWasted; // picked up powerups, that were over a limit (and converted into score)
		uint32 bombsUsed;
		uint32 bombsHitTotal; // total number of monsters hit by all bombs
		uint32 bombsKillTotal; // total number of monsters killed by all bombs
		uint32 bombsHitMax; // number of monsters hit by the most hitting bomb
		uint32 bombsKillMax; // number of monsters killed by the most killing bomb
		uint32 shieldStoppedMonsters; // number of monsters blocked by player's shield
		real shieldAbsorbedDamage; // total amount of damage absorbed from the blocked monsters
		uint32 turretsPlaced;
		uint32 decoysUsed;
		uint32 entitiesCurrent;
		uint32 entitiesMax; // maximum number of all entities at any sinle moment
		uint32 environmentGridMarkers; // initial number of grid markers
		uint32 environmentExplosions; // total number of explosions
		uint32 keyPressed; // keyboard keys pressed
		uint32 buttonPressed; // mouse buttons pressed
		uint32 updateIterationNoPause; // number of game update ticks, does increment during pause
		uint32 updateIterationPaused; // number of game update ticks, does NOT increment during pause
		uint32 frameIteration; // numer of rendered frames
		uint32 soundEffectsCurrent;
		uint32 soundEffectsMax;
	};
	extern globalStatisticsStruct statistics;

	struct globalSoundsStruct
	{
		real suspense;
	};
	extern globalSoundsStruct sounds;

	extern configUint32 confControlMovement;
	extern configUint32 confControlFiring;
	extern configUint32 confControlBomb;
	extern configUint32 confControlTurret;
	extern configUint32 confControlDecoy;
	extern configString confPlayerName;
	extern configFloat confPlayerShotColorR;
	extern configFloat confPlayerShotColorG;
	extern configFloat confPlayerShotColorB;
	extern configFloat confVolumeMusic;
	extern configFloat confVolumeEffects;
	extern configFloat confVolumeSpeech;

	// general components

	struct velocityComponent
	{
		static componentClass *component;
		vec3 velocity;
	};

	struct timeoutComponent
	{
		static componentClass *component;
		uint32 ttl; // game updates (does not tick when paused)
		timeoutComponent() : ttl(0) {}
	};

	// environment

	struct gridComponent
	{
		static componentClass *component;
		vec3 place;
		vec3 originalColor;
	};

	// player components

	struct shotComponent
	{
		static componentClass *component;
		real damage;
		bool homing;
		shotComponent() : homing(false) {}
	};

	struct powerupComponent
	{
		static componentClass *component;
		quat animation;
		powerupTypeEnum type;
		powerupComponent() : type(powerupTypeEnum::Total) {}
	};

	struct turretComponent
	{
		static componentClass *component;
		uint32 shooting;
		turretComponent() : shooting(0) {}
	};

	struct decoyComponent
	{
		static componentClass *component;
		quat rotation;
	};

	// monsters components

	struct monsterComponent
	{
		static componentClass *component;
		vec3 baseColorHsv;
		real life;
		real damage;
		real flickeringSpeed;
		real flickeringOffset;
		real groundLevel;
		real dispersion;
		uint32 destroyedSound;
		delegate<void(uint32)> shotDownCallback;
		monsterComponent() : destroyedSound(0) {}
	};

	struct simpleMonsterComponent
	{
		static componentClass *component;
		quat animation;
		real maxSpeed;
		real acceleration;
		real avoidance;
	};

	struct snakeTailComponent
	{
		static componentClass *component;
		uint32 follow;
		snakeTailComponent() : follow(0) {}
	};

	struct snakeHeadComponent
	{
		static componentClass *component;
		real speedMin, speedMax;
	};

	struct shielderComponent
	{
		static componentClass *component;
		uint32 shieldEntity;
		real movementSpeed;
		uint32 chargingSteps;
		uint32 turningSteps;
		uint32 stepsLeft;
		shielderComponent() : chargingSteps(0), turningSteps(0), stepsLeft(0) {}
	};

	struct shieldComponent
	{
		static componentClass *component;
		bool active;
		shieldComponent() : active(false) {}
	};

	struct wormholeComponent
	{
		static componentClass *component;
		real maxSpeed;
		real acceleration;
		real maxLife;
	};
}

#define GRID_GET_COMPONENT(T, C, E) ::grid::CAGE_JOIN(T, Component) &C = E->value<::grid::CAGE_JOIN(T, Component)>(::grid::CAGE_JOIN(T, Component)::component);
