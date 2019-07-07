//#define DEGRID_TESTING

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>

#include <cage-client/core.h>
#include <cage-client/engine.h>

#include <optick.h>

using namespace cage;

bool collisionTest(const vec3 &positionA, real radiusA, const vec3 &velocityA, const vec3 &positionB, real radiusB, const vec3 &velocityB);
void powerupSpawn(const vec3 &position);
void monstersSpawnInitial();
real lifeDamage(real damage); // how much life is taken by the damage (based on players armor)
void environmentExplosion(const vec3 &position, const vec3 &velocity, const vec3 &color, real size, real scale);
void monsterExplosion(entity *e);
void shotExplosion(entity *e);
bool killMonster(entity *e, bool allowCallback);
void soundEffect(uint32 sound, const vec3 &position);
void soundSpeech(uint32 sound);
void soundSpeech(uint32 sounds[]);
void setSkybox(uint32 objectName);
bool achievementFullfilled(const string &name, bool bossKill = false); // returns if this is the first time the achievement is fulfilled
void makeAnnouncement(uint32 headline, uint32 description, uint32 duration = 30 * 30);
uint32 permanentPowerupLimit();
uint32 currentPermanentPowerups();
bool canAddPermanentPowerup();
vec3 colorVariation(const vec3 &color);
eventDispatcher<bool()> &gameStartEvent();
eventDispatcher<bool()> &gameStopEvent();

enum class powerupTypeEnum
{
	// collectibles
	Bomb,
	Turret,
	Decoy,
	// timed
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
	Armor,
	Duration,
	// extra
	Coin,
	// total
	Total
};

const uint32 powerupMode[(uint32)powerupTypeEnum::Total] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3 };
const char letters[] = { 'C', 'E', 'F', 'Q', 'R', 'V', 'X', 'Z' };
const real playerScale = 3;
const real mapNoPullRadius = 200;
const vec3 playerDeathColor = vec3(0.68, 0.578, 0.252);
const uint32 shotsTtl = 300;
const real powerupIsCoin = 0.85;
const uint32 bossesTotalCount = 5;
const vec3 redPillColor = vec3(229, 101, 84) / 255;
const vec3 bluePillColor = vec3(123, 198, 242) / 255;
const uint32 basePermanentPowerupSellPrice = 20;
const uint32 basePermanentPowerupBuyPrice = 100;

extern entityGroup *entitiesToDestroy;
extern entityGroup *entitiesPhysicsEvenWhenPaused;
extern holder<spatialData> spatialSearchData;
extern holder<spatialQuery> spatialSearchQuery;

struct achievementsStruct
{
	uint32 bosses;
	uint32 acquired;

	achievementsStruct();
};
extern achievementsStruct achievements;

struct globalGameStruct
{
	// game state
	bool cinematic;
	bool paused;
	bool gameOver;
	bool jokeMap;

	uint32 defeatedBosses;
	uint32 buyPriceMultiplier;

	// entities
	entity *playerEntity;
	entity *shieldEntity;

	// player
	real life;
	real shootingCooldown;
	vec3 shotsColor;
	uint64 score;
	uint32 powerups[(uint32)powerupTypeEnum::Total];
	uint32 money;
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
	uint32 shotsHit; // total number of monsters hit by shots
	uint32 shotsKill; // total number of monsters killed by shots
	uint32 shotsCurrent;
	uint32 shotsMax; // maximum number of shots at any single moment
	uint32 monstersSpawned; // total number of monsters spawned (including special)
	uint32 monstersMutated; // total number of monsters, who spawned with any mutation
	uint32 monstersMutations; // total number of mutations (may be more than one per monster)
	uint32 monstersSucceded; // monsters that hit the player
	uint32 monstersCurrent;
	uint32 monstersMax; // maximum number of monsters at any single moment
	real monstersCurrentSpawningPriority; // current value of variable, that controls monsters spawning
	uint64 monstersCurrentMutationIteration;
	uint32 monstersFirstHit; // the time (in relation to updateIteration) in which the player was first hit by a monster
	uint32 monstersLastHit;
	uint32 shielderStoppedShots; // the number of shots eliminated by shielder
	uint32 wormholesSpawned;
	uint32 wormholeJumps;
	uint32 powerupsSpawned;
	uint32 coinsSpawned;
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
	uint32 updateIteration; // number of game update ticks, does NOT increment during pause
	uint32 frameIteration; // numer of rendered frames
	uint32 soundEffectsCurrent;
	uint32 soundEffectsMax;

	globalStatisticsStruct();
};
extern globalStatisticsStruct statistics;

// components

struct gravityComponent
{
	static entityComponent *component;
	real strength; // positive -> pull closer, negative -> push away
};

struct velocityComponent
{
	static entityComponent *component;
	vec3 velocity;
};

struct rotationComponent
{
	static entityComponent *component;
	quat rotation;
};

struct timeoutComponent
{
	static entityComponent *component;
	uint32 ttl; // game updates (does not tick when paused)
	timeoutComponent() : ttl(0) {}
};

struct gridComponent
{
	static entityComponent *component;
	vec3 place;
	vec3 originalColor;
};

struct shotComponent
{
	static entityComponent *component;
	real damage;
	bool homing;
	shotComponent() : homing(false) {}
};

struct powerupComponent
{
	static entityComponent *component;
	powerupTypeEnum type;
	powerupComponent() : type(powerupTypeEnum::Total) {}
};

struct monsterComponent
{
	static entityComponent *component;
	real life;
	real damage;
	real groundLevel;
	real dispersion;
	uint32 defeatedSound;
	delegate<void(uint32)> defeatedCallback;
	monsterComponent() : defeatedSound(0) {}
};

struct bossComponent
{
	static entityComponent *component;
};

#define DEGRID_COMPONENT(T, C, E) ::CAGE_JOIN(T, Component) &C = E->value<::CAGE_JOIN(T, Component)>(::CAGE_JOIN(T, Component)::component);
