#include "game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/spatial.h>

entityGroup *entitiesToDestroy;
entityGroup *entitiesPhysicsEvenWhenPaused;
holder<spatialData> spatialSearchData;
holder<spatialQuery> spatialSearchQuery;

namespace
{
	void engineInit()
	{
		entitiesToDestroy = entities()->defineGroup();
		entitiesPhysicsEvenWhenPaused = entities()->defineGroup();
		spatialSearchData = newSpatialData(spatialDataCreateConfig());
		spatialSearchQuery = newSpatialQuery(spatialSearchData.get());
	}

	void engineUpdate()
	{
		OPTICK_EVENT("physics");

		if (!game.paused)
		{ // gravity
			OPTICK_EVENT("gravity");
			for (entity *e : gravityComponent::component->entities())
			{
				CAGE_COMPONENT_ENGINE(transform, t, e);
				DEGRID_COMPONENT(gravity, g, e);
				for (entity *oe : velocityComponent::component->entities())
				{
					if (oe->has(gravityComponent::component))
						continue;
					CAGE_COMPONENT_ENGINE(transform, ot, oe);
					vec3 d = t.position - ot.position;
					if (lengthSquared(d) < 1e-3)
						continue;
					ot.position += normalize(d) * (g.strength / max(length(d) - t.scale, 1));
				}
			}
		}

		{ // velocity
			OPTICK_EVENT("velocity");
			for (entity *e : velocityComponent::component->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				CAGE_COMPONENT_ENGINE(transform, t, e);
				DEGRID_COMPONENT(velocity, v, e);
				t.position += v.velocity;
			}
		}

		{ // rotation
			OPTICK_EVENT("rotation");
			for (entity *e : rotationComponent::component->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				CAGE_COMPONENT_ENGINE(transform, t, e);
				DEGRID_COMPONENT(rotation, r, e);
				t.orientation = r.rotation * t.orientation;
			}
		}

		{ // timeout
			OPTICK_EVENT("timeout");
			for (entity *e : timeoutComponent::component->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				DEGRID_COMPONENT(timeout, t, e);
				if (t.ttl == 0)
					e->add(entitiesToDestroy);
				else
					t.ttl--;
			}
		}

		{
			OPTICK_EVENT("destroy entities");
			entitiesToDestroy->destroy();
		}

		{
			OPTICK_EVENT("spatial update");
			spatialSearchData->clear();
			for (entity *e : transformComponent::component->entities())
			{
				uint32 n = e->name();
				if (n)
				{
					CAGE_COMPONENT_ENGINE(transform, tr, e);
					spatialSearchData->update(n, sphere(tr.position, tr.scale));
				}
			}
		}

		{
			OPTICK_EVENT("spatial rebuild");
			spatialSearchData->rebuild();
		}
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize, -20);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update, 30);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

bool collisionTest(const vec3 &positionA, real radiusA, const vec3 &velocityA, const vec3 &positionB, real radiusB, const vec3 &velocityB)
{
	vec3 m = velocityB - velocityA;
	if (lengthSquared(m) > 1e-7)
		return intersects(makeSegment(positionB, positionB + m), sphere(positionA, radiusA + radiusB));
	return intersects(positionB, sphere(positionA, radiusA + radiusB));
}
