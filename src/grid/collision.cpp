#include "includes.h"
#include "game.h"

namespace grid
{
	const bool collisionTest(const vec3 &positionA, real radiusA, const vec3 movementA, const vec3 &positionB, real radiusB, const vec3 movementB)
	{
		vec3 m = movementB - movementA;
		real distance = m.length();
		return testRaySphereAnySide(positionA, radiusA + radiusB, positionB, m.normalize(), distance);
	}
}