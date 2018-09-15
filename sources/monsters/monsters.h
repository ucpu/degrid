#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/utility/hashString.h>
#include <cage-core/utility/spatial.h>

#include <cage-client/core.h>
#include <cage-client/engine.h>

#include "../game.h"

namespace grid
{
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

	uint32 spawnSpecial(uint32 &special);
	entityClass *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life);
}

namespace cage
{
	GCHL_ENUM_BITS(grid::monsterTypeFlags);
}
