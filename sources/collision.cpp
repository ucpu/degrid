#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/config.h>

#include <cage-client/core.h>

#include "game.h"

namespace grid
{
	bool collisionTest(const vec3 &positionA, real radiusA, const vec3 movementA, const vec3 &positionB, real radiusB, const vec3 movementB)
	{
		vec3 m = movementB - movementA;
		return intersects(makeSegment(positionB, positionB + m), sphere(positionA, radiusA + radiusB));
	}
}
