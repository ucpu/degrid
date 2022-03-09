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
		spatialSearchQuery = newSpatialQuery(spatialSearchData.share());
	}

	void engineUpdate()
	{
		if (!game.paused)
		{ // gravity
			for (Entity *e : engineEntities()->component<GravityComponent>()->entities())
			{
				TransformComponent &t = e->value<TransformComponent>();
				GravityComponent &g = e->value<GravityComponent>();
				for (Entity *oe : engineEntities()->component<VelocityComponent>()->entities())
				{
					if (oe->has<GravityComponent>())
						continue;
					TransformComponent &ot = oe->value<TransformComponent>();
					Vec3 d = t.position - ot.position;
					if (lengthSquared(d) < 1e-3)
						continue;
					ot.position += normalize(d) * (g.strength / max(length(d) - t.scale, 1));
				}
			}
		}

		{ // velocity
			for (Entity *e : engineEntities()->component<VelocityComponent>()->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				TransformComponent &t = e->value<TransformComponent>();
				VelocityComponent &v = e->value<VelocityComponent>();
				t.position += v.velocity;
			}
		}

		{ // rotation
			for (Entity *e : engineEntities()->component<RotationComponent>()->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				TransformComponent &t = e->value<TransformComponent>();
				RotationComponent &r = e->value<RotationComponent>();
				t.orientation = r.rotation * t.orientation;
			}
		}

		{ // timeout
			for (Entity *e : engineEntities()->component<TimeoutComponent>()->entities())
			{
				if (game.paused && !e->has(entitiesPhysicsEvenWhenPaused))
					continue;
				TimeoutComponent &t = e->value<TimeoutComponent>();
				if (t.ttl == 0)
					e->add(entitiesToDestroy);
				else
					t.ttl--;
			}
		}

		{
			entitiesToDestroy->destroy();
		}

		{
			spatialSearchData->clear();
			for (Entity *e : engineEntities()->component<TransformComponent>()->entities())
			{
				uint32 n = e->name();
				if (n)
				{
					TransformComponent &tr = e->value<TransformComponent>();
					spatialSearchData->update(n, Sphere(tr.position, tr.scale));
				}
			}
			spatialSearchData->rebuild();
		}
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize, -40);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update, 30);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

bool collisionTest(const Vec3 &positionA, Real radiusA, const Vec3 &velocityA, const Vec3 &positionB, Real radiusB, const Vec3 &velocityB)
{
	Vec3 m = velocityB - velocityA;
	if (lengthSquared(m) > 1e-7)
		return intersects(makeSegment(positionB, positionB + m), Sphere(positionA, radiusA + radiusB));
	return intersects(positionB, Sphere(positionA, radiusA + radiusB));
}
