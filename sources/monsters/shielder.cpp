#include "monsters.h"

namespace
{
	struct shielderComponent
	{
		static componentClass *component;
		uint32 shieldEntity;
		real movementSpeed;
		uint32 chargingSteps;
		uint32 turningSteps;
		uint32 stepsLeft;
		shielderComponent() : chargingSteps(0), turningSteps(0), stepsLeft(0) {}
	};

	struct shieldComponent
	{
		static componentClass *component;
		bool active;
		shieldComponent() : active(false) {}
	};

	componentClass *shielderComponent::component;
	componentClass *shieldComponent::component;

	void updateShields()
	{
		// update shields transformations
		for (entityClass *e : shielderComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, et, e);
			GRID_GET_COMPONENT(shielder, es, e);
			entityClass *s = entities()->get(es.shieldEntity);
			ENGINE_GET_COMPONENT(transform, st, s);
			st = et;
		}
	}

	void shielderEliminated(entityClass *e)
	{
		GRID_GET_COMPONENT(shielder, sh, e);
		if (entities()->has(sh.shieldEntity))
			entities()->get(sh.shieldEntity)->add(entitiesToDestroy);
	}

	eventListener<void(entityClass*)> shielderEliminatedListener;

	void engineInit()
	{
		shielderComponent::component = entities()->defineComponent(shielderComponent(), true);
		shieldComponent::component = entities()->defineComponent(shieldComponent(), true);
		shielderEliminatedListener.bind<&shielderEliminated>();
		shielderEliminatedListener.attach(shielderComponent::component->group()->entityRemoved);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		for (entityClass *e : shielderComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			GRID_GET_COMPONENT(velocity, mv, e);
			GRID_GET_COMPONENT(monster, ms, e);
			GRID_GET_COMPONENT(shielder, sh, e);
			entityClass *se = entities()->get(sh.shieldEntity);
			GRID_GET_COMPONENT(shield, sse, se);

			// update the monster
			if (sse.active)
			{ // charging
				if (sh.stepsLeft)
				{
					vec3 t = normalize(game.monstersTarget - tr.position);
					tr.orientation = interpolate(tr.orientation, quat(t, vec3(0, 1, 0)), 0.02);
					vec3 f = tr.orientation * vec3(0, 0, -1);
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
				mv.velocity = vec3();
				if (sh.stepsLeft)
				{
					vec3 t = normalize(game.monstersTarget - tr.position);
					tr.orientation = interpolate(tr.orientation, quat(t, vec3(0, 1, 0)), 0.95 / sh.stepsLeft);
				}
				else
				{
					sse.active = true;
					sh.stepsLeft = sh.chargingSteps;
				}
			}

			CAGE_ASSERT_RUNTIME(sh.stepsLeft > 0);
			sh.stepsLeft--;

			// update shield rendering
			if (sse.active)
			{
				ENGINE_GET_COMPONENT(render, render, se);
				render.object = hashString("grid/monster/shield.object");
			}
			else
			{
				se->remove(renderComponent::component);
				continue;
			}

			// destroy shots
			vec3 forward = tr.orientation * vec3(0, 0, -1);
			spatialQuery->intersection(sphere(tr.position + forward * (tr.scale + 3), 5));
			for (uint32 otherName : spatialQuery->result())
			{
				entityClass *e = entities()->get(otherName);
				if (!e->has(shotComponent::component))
					continue;
				ENGINE_GET_COMPONENT(transform, ot, e);
				vec3 toShot = ot.position - tr.position;
				real lenShot = toShot.length();
				if (lenShot < tr.scale || lenShot > tr.scale + 6)
					continue;
				vec3 dirShot = toShot.normalize();
				if (dirShot.dot(forward) < cos(degs(45)))
					continue;
				e->add(entitiesToDestroy);
				statistics.shielderStoppedShots++;
				shotExplosion(e);
			}
		}
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener1;
		eventListener<void()> engineUpdateListener2;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener1.attach(controlThread().update);
			engineUpdateListener1.bind<&engineUpdate>();
			engineUpdateListener2.attach(controlThread().update, 40); // after physics
			engineUpdateListener2.bind<&updateShields>();
		}
	} callbacksInstance;
}

void spawnShielder(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	entityClass *shielder = initializeMonster(spawnPosition, color, 3, hashString("grid/monster/shielder.object"), hashString("grid/monster/bum-shielder.ogg"), 5, 3 + monsterMutation(special));
	entityClass *shield = entities()->createUnique();
	{
		GRID_GET_COMPONENT(shielder, sh, shielder);
		sh.shieldEntity = shield->name();
		sh.movementSpeed = 0.8 + 0.2 * monsterMutation(special);
		sh.turningSteps = randomRange(20u, 30u);
		sh.chargingSteps = randomRange(60u, 180u);
		sh.stepsLeft = sh.turningSteps;
		GRID_GET_COMPONENT(monster, m, shielder);
		m.dispersion = 0.2;
		monsterReflectMutation(shielder, special);
	}
	{
		ENGINE_GET_COMPONENT(transform, transformShielder, shielder);
		ENGINE_GET_COMPONENT(transform, transform, shield);
		transform = transformShielder;
		GRID_GET_COMPONENT(shield, sh, shield);
		sh.active = false;
	}
}
