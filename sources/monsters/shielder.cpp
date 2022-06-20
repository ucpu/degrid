#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

namespace
{
	struct ShielderComponent
	{
		uint32 shieldEntity = 0;
		uint32 chargingSteps = 0;
		uint32 turningSteps = 0;
		uint32 stepsLeft = 0;
		Real movementSpeed;
	};

	struct ShieldComponent
	{
		bool active = false;
	};

	void updateShields()
	{
		entitiesVisitor([&](Entity *e, const ShielderComponent &es, const TransformComponent &et) {
			Entity *s = engineEntities()->get(es.shieldEntity);
			s->value<TransformComponent>() = et;
		}, engineEntities(), false);
	}

	void shielderEliminated(Entity *e)
	{
		const ShielderComponent &sh = e->value<ShielderComponent>();
		if (engineEntities()->has(sh.shieldEntity))
			engineEntities()->get(sh.shieldEntity)->add(entitiesToDestroy);
	}

	EventListener<void(Entity*)> shielderEliminatedListener;

	void engineInit()
	{
		auto c = engineEntities()->defineComponent(ShielderComponent());
		engineEntities()->defineComponent(ShieldComponent());
		shielderEliminatedListener.bind<&shielderEliminated>();
		shielderEliminatedListener.attach(c->group()->entityRemoved);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		entitiesVisitor([&](Entity *e, TransformComponent &tr, VelocityComponent &mv, MonsterComponent &ms, ShielderComponent &sh) {
			Entity *se = engineEntities()->get(sh.shieldEntity);
			ShieldComponent &sse = se->value<ShieldComponent>();

			// update the monster
			if (sse.active)
			{ // charging
				if (sh.stepsLeft)
				{
					const Vec3 t = normalize(game.monstersTarget - tr.position);
					tr.orientation = interpolate(tr.orientation, Quat(t, Vec3(0, 1, 0)), 0.02);
					const Vec3 f = tr.orientation * Vec3(0, 0, -1);
					mv.velocity = f * sh.movementSpeed;
				}
				else
				{
					sse.active = false;
					sh.stepsLeft = sh.turningSteps;
				}
			}
			else
			{ // turning
				mv.velocity = Vec3();
				if (sh.stepsLeft)
				{
					const Vec3 t = normalize(game.monstersTarget - tr.position);
					tr.orientation = interpolate(tr.orientation, Quat(t, Vec3(0, 1, 0)), 0.95 / sh.stepsLeft);
				}
				else
				{
					sse.active = true;
					sh.stepsLeft = sh.chargingSteps;
				}
			}

			CAGE_ASSERT(sh.stepsLeft > 0);
			sh.stepsLeft--;

			// update shield rendering
			if (sse.active)
				se->value<RenderComponent>().object = HashString("degrid/monster/shield.object");
			else
			{
				se->remove<RenderComponent>();
				return;
			}

			// destroy shots
			const Vec3 forward = tr.orientation * Vec3(0, 0, -1);
			spatialSearchQuery->intersection(Sphere(tr.position + forward * (tr.scale + 1), 5));
			for (uint32 otherName : spatialSearchQuery->result())
			{
				Entity *e = engineEntities()->get(otherName);
				if (!e->has<ShotComponent>())
					continue;
				const Vec3 toShot = e->value<TransformComponent>().position - tr.position;
				const Vec3 dirShot = normalize(toShot);
				if (dot(dirShot, forward) < cos(Degs(45)))
					continue;
				e->add(entitiesToDestroy);
				statistics.shielderStoppedShots++;
				shotExplosion(e);
			}
		}, engineEntities(), false);
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener1;
		EventListener<void()> engineUpdateListener2;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener1.attach(controlThread().update);
			engineUpdateListener1.bind<&engineUpdate>();
			engineUpdateListener2.attach(controlThread().update, 41); // after physics
			engineUpdateListener2.bind<&updateShields>();
		}
	} callbacksInstance;
}

void spawnShielder(const Vec3 &spawnPosition, const Vec3 &color)
{
	uint32 special = 0;
	Entity *shielder = initializeMonster(spawnPosition, color, 3, HashString("degrid/monster/shielder.object"), HashString("degrid/monster/bum-shielder.ogg"), 5, 3 + monsterMutation(special));
	Entity *shield = engineEntities()->createUnique();
	{
		ShielderComponent &sh = shielder->value<ShielderComponent>();
		sh.shieldEntity = shield->name();
		sh.movementSpeed = 0.8 + 0.2 * monsterMutation(special);
		sh.turningSteps = randomRange(20u, 30u);
		sh.chargingSteps = randomRange(60u, 180u);
		sh.stepsLeft = sh.turningSteps;
		shielder->value<MonsterComponent>().dispersion = 0.2;
		monsterReflectMutation(shielder, special);
	}
	{
		shield->value<TransformComponent>() = shielder->value<TransformComponent>();
		shield->value<ShieldComponent>().active = false;
	}
}
