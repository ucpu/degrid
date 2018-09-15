#include <cage-core/core.h>
#include <cage-core/math.h>
#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>

#include <cage-client/core.h>
#include <cage-client/engine.h>

#include "game.h"

namespace grid
{
	bool collisionTest(const vec3 &positionA, real radiusA, const vec3 velocityA, const vec3 &positionB, real radiusB, const vec3 velocityB)
	{
		vec3 m = velocityB - velocityA;
		return intersects(makeSegment(positionB, positionB + m), sphere(positionA, radiusA + radiusB));
	}

	void physicsUpdate()
	{
		if (player.paused)
			return;

		{ // velocity
			for (entityClass *e : velocityComponent::component->getComponentEntities()->entities())
			{
				ENGINE_GET_COMPONENT(transform, t, e);
				GRID_GET_COMPONENT(velocity, v, e);
				t.position += v.velocity;
			}
		}

		{ // timeout
			for (entityClass *e : timeoutComponent::component->getComponentEntities()->entities())
			{
				GRID_GET_COMPONENT(timeout, t, e);
				if (t.ttl == 0)
					e->addGroup(entitiesToDestroy);
				else
					t.ttl--;
			}
		}
	}
}
