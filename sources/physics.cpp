#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/spatialStructure.h>

#include "game.h"

EntityGroup *entitiesToDestroy;
EntityGroup *entitiesPhysicsEvenWhenPaused;
Holder<SpatialStructure> spatialSearchData;
Holder<SpatialQuery> spatialSearchQuery;

namespace
{
	void engineInit()
	{
		entitiesToDestroy = engineEntities()->defineGroup();
		entitiesPhysicsEvenWhenPaused = engineEntities()->defineGroup();
		spatialSearchData = newSpatialStructure({});
		spatialSearchQuery = newSpatialQuery(spatialSearchData.get());
	}

	void engineUpdate()
	{
		OPTICK_EVENT("physics");

		if (!game.paused)
		{ // gravity
			OPTICK_EVENT("gravity");
			for (Entity *e : GravityComponent::component->entities())
			{
				CAGE_COMPONENT_ENGINE(Transform, t, e);
				DEGRID_COMPONENT(Gravity, g, e);
				for (Entity *oe : VelocityComponent::component->entities())
				{
					if (oe->has(GravityComponent::component))
						continue;
					CAGE_COMPONENT_ENGINE(Transform, ot, oe);
					vec3 d = t.position - ot.position;
					if (lengthSquared(d) < 1e-3)
						continue;
					ot.position += normalize(d) * (g.strength / max(length(d) - t.scale, 1));
				}
			}
		}

		{ // velocity
			OPTICK_EVENT("velocity");
			for (Entity *e : VelocityComponent::component->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				CAGE_COMPONENT_ENGINE(Transform, t, e);
				DEGRID_COMPONENT(Velocity, v, e);
				t.position += v.velocity;
			}
		}

		{ // rotation
			OPTICK_EVENT("rotation");
			for (Entity *e : RotationComponent::component->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				CAGE_COMPONENT_ENGINE(Transform, t, e);
				DEGRID_COMPONENT(Rotation, r, e);
				t.orientation = r.rotation * t.orientation;
			}
		}

		{ // timeout
			OPTICK_EVENT("timeout");
			for (Entity *e : TimeoutComponent::component->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				DEGRID_COMPONENT(Timeout, t, e);
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
			OPTICK_EVENT("Spatial update");
			spatialSearchData->clear();
			for (Entity *e : TransformComponent::component->entities())
			{
				uint32 n = e->name();
				if (n)
				{
					CAGE_COMPONENT_ENGINE(Transform, tr, e);
					spatialSearchData->update(n, sphere(tr.position, tr.scale));
				}
			}
		}

		{
			OPTICK_EVENT("Spatial rebuild");
			spatialSearchData->rebuild();
		}
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks() : engineInitListener("physics"), engineUpdateListener("physics")
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
