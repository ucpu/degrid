#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/spatialStructure.h>

#include "../game.h"

enum class MonsterTypeFlags
{
	None = 0,
	Circle = 1 << 0,
	SmallTriangle = 1 << 1,
	SmallCube = 1 << 2,
	LargeTriangle = 1 << 3,
	LargeCube = 1 << 4,
	PinWheel = 1 << 5,
	Diamond = 1 << 6,
	Snake = 1 << 7,
	Shielder = 1 << 8,
	Shocker = 1 << 9,
	Wormhole = 1 << 10,
	Rocket = 1 << 11,
	Spawner = 1 << 12,
	BossEgg = 1 << 29,
	BossBody = 1 << 30,
	BossExtension = 1 << 31,
};
namespace cage
{
	GCHL_ENUM_BITS(MonsterTypeFlags);
}

void spawnGeneral(MonsterTypeFlags type, const Vec3 &spawnPosition, const Vec3 &color);
void spawnSimple(MonsterTypeFlags type, const Vec3 &spawnPosition, const Vec3 &color);
void spawnSnake(const Vec3 &spawnPosition, const Vec3 &color);
void spawnShielder(const Vec3 &spawnPosition, const Vec3 &color);
void spawnShocker(const Vec3 &spawnPosition, const Vec3 &color);
void spawnWormhole(const Vec3 &spawnPosition, const Vec3 &color);
void spawnRocket(const Vec3 &spawnPosition, const Vec3 &color);
void spawnSpawner(const Vec3 &spawnPosition, const Vec3 &color);
void spawnBossEgg(const Vec3 &spawnPosition, const Vec3 &color);
void spawnBossCannoneer(const Vec3 &spawnPosition, const Vec3 &color);

Entity *initializeMonster(const Vec3 &spawnPosition, const Vec3 &color, Real scale, uint32 objectName, uint32 deadSound, Real damage, Real life);
Entity *initializeSimple(const Vec3 &spawnPosition, const Vec3 &color, Real scale, uint32 objectName, uint32 deadSound, Real damage, Real life, Real maxSpeed, Real accelerationFraction, Real avoidance, Real dispersion, Real circling, Real spiraling, const Quat &animation);
uint32 monsterMutation(uint32 &special);
void monsterReflectMutation(Entity *e, uint32 special);

struct SimpleMonsterComponent
{
	Real maxSpeed;
	Real acceleration;
	Real avoidance;
	Real circling;
	Real spiraling;
};
