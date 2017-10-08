#include "../includes.h"
#include "../game.h"

namespace grid
{
	enum monsterTypeFlags
	{
		mtCircle = 1 << 0,
		mtSmallTriangle = 1 << 1,
		mtSmallCube = 1 << 2,
		mtLargeTriangle = 1 << 3,
		mtLargeCube = 1 << 4,
		mtPinWheel = 1 << 5,
		mtDiamond = 1 << 6,
		mtShielder = 1 << 7,
		mtSnake = 1 << 8,
		mtWormhole = 1 << 9,
	};

	void spawnUpdate();
	void spawnInit();
	void spawnDone();
	void spawnGeneral(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color);

	void updateSimple();
	void spawnSimple(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color);

	void updateSnake();
	void spawnSnake(const vec3 &spawnPosition, const vec3 &color);

	void updateShielder();
	void spawnShielder(const vec3 &spawnPosition, const vec3 &color);

	void updateWormhole();
	void spawnWormhole(const vec3 &spawnPosition, const vec3 &color);

	const uint32 spawnSpecial(uint32 &special);
	entityClass *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life);
}
