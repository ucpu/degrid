//#define GRID_TESTING

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>

#include <cage-client/core.h>
#include <cage-client/engine.h>

using namespace cage;

bool collisionTest(const vec3 &positionA, real radiusA, const vec3 velocityA, const vec3 &positionB, real radiusB, const vec3 velocityB);
void powerupSpawn(const vec3 &position);
void monstersSpawnInitial();
void environmentExplosion(const vec3 &position, const vec3 &velocity, const vec3 &color, real size, real scale);
void monsterExplosion(entityClass *e);
void shotExplosion(entityClass *e);
bool killMonster(entityClass *e);
void soundEffect(uint32 sound, const vec3 &position);
void soundSpeech(uint32 sound);
void soundSpeech(uint32 sounds[]);
vec3 colorVariation(const vec3 &color);
cage::eventDispatcher<bool()> &gameStartEvent();
cage::eventDispatcher<bool()> &gameStopEvent();

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
const real playerScale = 3;
const real mapNoPullRadius = 200;
const vec3 playerDeathColor = vec3(0.68, 0.578, 0.252);
const uint32 shotsTtl = 300;

extern groupClass *entitiesToDestroy;
extern groupClass *entitiesPhysicsEvenWhenPaused;
extern holder<spatialDataClass> spatialData;
extern holder<spatialQueryClass> spatialQuery;

struct globalGameStruct
{
	// game state
	bool cinematic;
	bool paused;
	bool gameOver;

	// entities
	entityClass *playerEntity;
	entityClass *shieldEntity;

	// player
	real life;
	real shootingCooldown;
	vec3 shotsColor;
	uint32 score;
	uint32 powerups[(uint32)powerupTypeEnum::Total];
	real powerupSpawnChance;
	vec3 monstersTarget;

	// ship controls (options dependent)
	vec3 moveDirection;
	vec3 fireDirection;

	globalGameStruct();
};
extern globalGameStruct game;

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
	uint32 wormholeJumps;
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
	uint32 updateIterationIgnorePause; // number of game update ticks, does increment during pause
	uint32 updateIterationWithPause; // number of game update ticks, does NOT increment during pause
	uint32 frameIteration; // numer of rendered frames
	uint32 soundEffectsCurrent;
	uint32 soundEffectsMax;

	globalStatisticsStruct();
};
extern globalStatisticsStruct statistics;

// components

struct gravityComponent
{
	static componentClass *component;
	real strength; // positive -> pull closer, negative -> push away
};

struct velocityComponent
{
	static componentClass *component;
	vec3 velocity;
};

struct rotationComponent
{
	static componentClass *component;
	quat rotation;
};

struct timeoutComponent
{
	static componentClass *component;
	uint32 ttl; // game updates (does not tick when paused)
	timeoutComponent() : ttl(0) {}
};

struct gridComponent
{
	static componentClass *component;
	vec3 place;
	vec3 originalColor;
};

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
	powerupTypeEnum type;
	powerupComponent() : type(powerupTypeEnum::Total) {}
};

struct monsterComponent
{
	static componentClass *component;
	real life;
	real damage;
	real groundLevel;
	real dispersion;
	uint32 defeatedSound;
	delegate<void(uint32)> defeatedCallback;
	monsterComponent() : defeatedSound(0) {}
};

#define GRID_GET_COMPONENT(T, C, E) ::CAGE_JOIN(T, Component) &C = E->value<::CAGE_JOIN(T, Component)>(::CAGE_JOIN(T, Component)::component);
