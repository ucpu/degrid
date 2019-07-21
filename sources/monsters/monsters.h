#include "../game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/spatial.h>

enum class monsterTypeFlags
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
	GCHL_ENUM_BITS(monsterTypeFlags);
}

void spawnGeneral(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color);
void spawnSimple(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color);
void spawnSnake(const vec3 &spawnPosition, const vec3 &color);
void spawnShielder(const vec3 &spawnPosition, const vec3 &color);
void spawnShocker(const vec3 &spawnPosition, const vec3 &color);
void spawnWormhole(const vec3 &spawnPosition, const vec3 &color);
void spawnRocket(const vec3 &spawnPosition, const vec3 &color);
void spawnSpawner(const vec3 &spawnPosition, const vec3 &color);
void spawnBossEgg(const vec3 &spawnPosition, const vec3 &color);
void spawnBossCannoneer(const vec3 &spawnPosition, const vec3 &color);

entity *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life);
entity *initializeSimple(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life, real maxSpeed, real accelerationFraction, real avoidance, real dispersion, real circling, real spiraling, const quat &animation);
uint32 monsterMutation(uint32 &special);
void monsterReflectMutation(entity *e, uint32 special);

struct simpleMonsterComponent
{
	static entityComponent *component;
	real maxSpeed;
	real acceleration;
	real avoidance;
	real circling;
	real spiraling;
};
