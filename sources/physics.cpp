#include "game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/spatial.h>

groupClass *entitiesToDestroy;
groupClass *entitiesPhysicsEvenWhenPaused;
holder<spatialDataClass> spatialData;
holder<spatialQueryClass> spatialQuery;

namespace
{
	void engineInit()
	{
		entitiesToDestroy = entities()->defineGroup();
		entitiesPhysicsEvenWhenPaused = entities()->defineGroup();
		spatialData = newSpatialData(spatialDataCreateConfig());
		spatialQuery = newSpatialQuery(spatialData.get());
	}

	void engineUpdate()
	{
		if (!game.paused)
		{ // gravity
			for (entityClass *e : gravityComponent::component->getComponentEntities()->entities())
			{
				ENGINE_GET_COMPONENT(transform, t, e);
				GRID_GET_COMPONENT(gravity, g, e);
				for (entityClass *oe : velocityComponent::component->getComponentEntities()->entities())
				{
					if (oe->hasComponent(gravityComponent::component))
						continue;
					ENGINE_GET_COMPONENT(transform, ot, oe);
					vec3 d = t.position - ot.position;
					if (d.squaredLength() < 1e-3)
						continue;
					ot.position += d.normalize() * (g.strength / max(d.length() - t.scale, 1));
				}
			}
		}

		{ // velocity
			for (entityClass *e : velocityComponent::component->getComponentEntities()->entities())
			{
				if (game.paused && !e->hasGroup(entitiesPhysicsEvenWhenPaused))
					continue;
				ENGINE_GET_COMPONENT(transform, t, e);
				GRID_GET_COMPONENT(velocity, v, e);
				t.position += v.velocity;
			}
		}

		{ // rotation
			for (entityClass *e : rotationComponent::component->getComponentEntities()->entities())
			{
				if (game.paused && !e->hasGroup(entitiesPhysicsEvenWhenPaused))
					continue;
				ENGINE_GET_COMPONENT(transform, t, e);
				GRID_GET_COMPONENT(rotation, r, e);
				t.orientation = r.rotation * t.orientation;
			}
		}

		{ // timeout
			for (entityClass *e : timeoutComponent::component->getComponentEntities()->entities())
			{
				if (game.paused && !e->hasGroup(entitiesPhysicsEvenWhenPaused))
					continue;
				GRID_GET_COMPONENT(timeout, t, e);
				if (t.ttl == 0)
					e->addGroup(entitiesToDestroy);
				else
					t.ttl--;
			}
		}

		entitiesToDestroy->destroyAllEntities();

		{ // update spatial
			spatialData->clear();
			for (entityClass *e : transformComponent::component->getComponentEntities()->entities())
			{
				uint32 n = e->getName();
				if (n)
				{
					ENGINE_GET_COMPONENT(transform, tr, e);
					spatialData->update(n, sphere(tr.position, tr.scale));
				}
			}
			spatialData->rebuild();
		}
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
		eventListener<void()> gameStopListener;
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

bool collisionTest(const vec3 &positionA, real radiusA, const vec3 velocityA, const vec3 &positionB, real radiusB, const vec3 velocityB)
{
	vec3 m = velocityB - velocityA;
	return intersects(makeSegment(positionB, positionB + m), sphere(positionA, radiusA + radiusB));
}
