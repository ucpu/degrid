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

entityClass *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life);
uint32 monsterMutation(uint32 &special);
void monsterReflectMutation(entityClass *e, uint32 special);

namespace cage
{
	GCHL_ENUM_BITS(monsterTypeFlags);
}
