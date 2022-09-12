//#define DEGRID_TESTING

#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-engine/core.h>
#include <cage-engine/scene.h>
#include <cage-simple/engine.h>

namespace cage
{
	class Entity;
	class EntityGroup;
	class SpatialStructure;
	class SpatialQuery;
}

using namespace cage;

bool collisionTest(const Vec3 &positionA, Real radiusA, const Vec3 &velocityA, const Vec3 &positionB, Real radiusB, const Vec3 &velocityB);
void powerupSpawn(const Vec3 &position);
void monstersSpawnInitial();
Real lifeDamage(Real damage);
void environmentExplosion(const Vec3 &position, const Vec3 &velocity, const Vec3 &color, Real size);
void monsterExplosion(Entity *e);
void shotExplosion(Entity *e);
bool killMonster(Entity *e, bool allowCallback);
void soundEffect(uint32 sound, const Vec3 &position);
void soundSpeech(uint32 sound);
void soundSpeech(const uint32 sounds[]);
void setSkybox(uint32 objectName);
bool achievementFullfilled(const String &name, bool bossKill = false);
void makeAnnouncement(uint32 headline, uint32 description, uint32 duration = 30 * 30);
uint32 permanentPowerupLimit();
uint32 currentPermanentPowerups();
bool canAddPermanentPowerup();
Vec3 colorVariation(const Vec3 &color);
EventDispatcher<bool()> &gameStartEvent();
EventDispatcher<bool()> &gameStopEvent();

enum class PowerupTypeEnum
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

constexpr const uint32 PowerupMode[(uint32)PowerupTypeEnum::Total] = {
	0, 0, 0, // collectibles
	1, 1, 1, // timed
	2, 2, 2, 2, 2, 2, 2, 2, // permanent
	3 // extra
};
constexpr const float PowerupChances[(uint32)PowerupTypeEnum::Total] = {
	0.1f / 3, 0.1f / 3, 0.1f / 3,
	0.1f / 3, 0.1f / 3, 0.1f / 3,
	0.1f / 8, 0.1f / 8, 0.1f / 8, 0.1f / 8, 0.1f / 8, 0.1f / 8, 0.1f / 8, 0.1f / 8,
	0.7f
};
constexpr const char Letters[] = { 'C', 'E', 'F', 'Q', 'R', 'V', 'X', 'Z' };
constexpr float PlayerScale = 3;
constexpr float MapNoPullRadius = 250;
constexpr const Vec3 PlayerDeathColor = Vec3(0.68, 0.578, 0.252);
constexpr uint32 ShotsTtl = 300;
constexpr uint32 BossesTotalCount = 5;
constexpr const Vec3 RedPillColor = Vec3(229, 101, 84) / 255;
constexpr const Vec3 BluePillColor = Vec3(123, 198, 242) / 255;
constexpr uint32 PowerupSellPriceBase = 5;
constexpr uint32 PowerupBuyPriceBase = 10;

extern EntityGroup *entitiesToDestroy;
extern EntityGroup *entitiesPhysicsEvenWhenPaused;
extern Holder<SpatialStructure> spatialSearchData;
extern Holder<SpatialQuery> spatialSearchQuery;

struct Achievements
{
	uint32 bosses = 0;
	uint32 acquired = 0;
};
extern Achievements achievements;

struct GlobalGame
{
	// game state
	bool cinematic = true;
	bool paused = false;
	bool gameOver = false;
	bool jokeMap = false;

	uint32 defeatedBosses = 0;
	uint32 buyPriceMultiplier = 1;

	// entities
	Entity *playerEntity = nullptr;
	Entity *shieldEntity = nullptr;

	// player
	Real life;
	Real shootingCooldown;
	Vec3 shotsColor;
	uint64 score = 0;
	uint32 powerups[(uint32)PowerupTypeEnum::Total] = {};
	uint32 money = 0;
	Real powerupSpawnChance;
	Vec3 monstersTarget;

	// ship controls (options dependent)
	Vec3 moveDirection;
	Vec3 fireDirection;
};
extern GlobalGame game;

struct GlobalStatistics
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
	Real monstersCurrentSpawningPriority; 
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
	Real shieldAbsorbedDamage; 
	uint32 turretsPlaced;
	uint32 decoysUsed;
	uint32 entitiesCurrent;
	uint32 entitiesMax; // maximum number of all entities at any single moment
	uint32 environmentGridMarkers; // initial number of grid markers
	uint32 environmentExplosions; // total number of explosions
	uint32 keyPressed; // keyboard keys pressed
	uint32 buttonPressed; // mouse buttons pressed
	uint32 updateIterationIgnorePause; // number of game update ticks, does increment during pause
	uint32 updateIteration; // number of game update ticks, does NOT increment during pause
	uint32 frameIteration; // number of rendered frames
	uint32 soundEffectsCurrent;
	uint32 soundEffectsMax;

	GlobalStatistics();
};
extern GlobalStatistics statistics;

// components

struct GravityComponent
{
	Real strength;
};

struct VelocityComponent
{
	Vec3 velocity;
};

struct RotationComponent
{
	Quat rotation;
};

struct TimeoutComponent
{
	uint32 ttl = 0; // game updates (does not tick when paused)
};

struct GridComponent
{
	Vec3 place;
	Vec3 originalColor;
};

struct ShotComponent
{
	Real damage;
	bool homing = false;
};

struct PowerupComponent
{
	PowerupTypeEnum type = PowerupTypeEnum::Total;
};

struct MonsterComponent
{
	Real life;
	Real damage;
	Real groundLevel;
	Real dispersion;
	uint32 defeatedSound = 0;
	Delegate<void(uint32)> defeatedCallback;
};

struct BossComponent
{};
