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
	Shielder = 1 << 7,
	Snake = 1 << 8,
	Wormhole = 1 << 9,
};

void spawnGeneral(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color);
void spawnSimple(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color);
void spawnSnake(const vec3 &spawnPosition, const vec3 &color);
void spawnShielder(const vec3 &spawnPosition, const vec3 &color);
void spawnWormhole(const vec3 &spawnPosition, const vec3 &color);

uint32 spawnSpecial(uint32 &special);
entityClass *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life);

namespace cage
{
	GCHL_ENUM_BITS(monsterTypeFlags);
}

struct simpleMonsterComponent
{
	static componentClass *component;
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

struct monsterFlickeringComponent
{
	static componentClass *component;
	vec3 baseColorHsv;
	real flickeringFrequency;
	real flickeringOffset;
};
