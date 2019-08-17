#include "monsters.h"

namespace
{
	struct shielderComponent
	{
		static entityComponent *component;
		uint32 shieldEntity;
		real movementSpeed;
		uint32 chargingSteps;
		uint32 turningSteps;
		uint32 stepsLeft;
		shielderComponent() : chargingSteps(0), turningSteps(0), stepsLeft(0) {}
	};

	struct shieldComponent
	{
		static entityComponent *component;
		bool active;
		shieldComponent() : active(false) {}
	};

	entityComponent *shielderComponent::component;
	entityComponent *shieldComponent::component;

	void updateShields()
	{
		// update shields transformations
		for (entity *e : shielderComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, et, e);
			DEGRID_COMPONENT(shielder, es, e);
			entity *s = entities()->get(es.shieldEntity);
			CAGE_COMPONENT_ENGINE(transform, st, s);
			st = et;
		}
	}

	void shielderEliminated(entity *e)
	{
		DEGRID_COMPONENT(shielder, sh, e);
		if (entities()->has(sh.shieldEntity))
			entities()->get(sh.shieldEntity)->add(entitiesToDestroy);
	}

	eventListener<void(entity*)> shielderEliminatedListener;

	void engineInit()
	{
		shielderComponent::component = entities()->defineComponent(shielderComponent(), true);
		shieldComponent::component = entities()->defineComponent(shieldComponent(), true);
		shielderEliminatedListener.bind<&shielderEliminated>();
		shielderEliminatedListener.attach(shielderComponent::component->group()->entityRemoved);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("shielder");

		if (game.paused)
			return;

		for (entity *e : shielderComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, tr, e);
			DEGRID_COMPONENT(velocity, mv, e);
			DEGRID_COMPONENT(monster, ms, e);
			DEGRID_COMPONENT(shielder, sh, e);
			entity *se = entities()->get(sh.shieldEntity);
			DEGRID_COMPONENT(shield, sse, se);

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

			CAGE_ASSERT(sh.stepsLeft > 0);
			sh.stepsLeft--;

			// update shield rendering
			if (sse.active)
			{
				CAGE_COMPONENT_ENGINE(render, render, se);
				render.object = hashString("degrid/monster/shield.object");
			}
			else
			{
				se->remove(renderComponent::component);
				continue;
			}

			// destroy shots
			vec3 forward = tr.orientation * vec3(0, 0, -1);
			spatialSearchQuery->intersection(sphere(tr.position + forward * (tr.scale + 1), 5));
			for (uint32 otherName : spatialSearchQuery->result())
			{
				entity *e = entities()->get(otherName);
				if (!e->has(shotComponent::component))
					continue;
				CAGE_COMPONENT_ENGINE(transform, ot, e);
				vec3 toShot = ot.position - tr.position;
				vec3 dirShot = normalize(toShot);
				if (dot(dirShot, forward) < cos(degs(45)))
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
			engineUpdateListener2.attach(controlThread().update, 41); // after physics
			engineUpdateListener2.bind<&updateShields>();
		}
	} callbacksInstance;
}

void spawnShielder(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	entity *shielder = initializeMonster(spawnPosition, color, 3, hashString("degrid/monster/shielder.object"), hashString("degrid/monster/bum-shielder.ogg"), 5, 3 + monsterMutation(special));
	entity *shield = entities()->createUnique();
	{
		DEGRID_COMPONENT(shielder, sh, shielder);
		sh.shieldEntity = shield->name();
		sh.movementSpeed = 0.8 + 0.2 * monsterMutation(special);
		sh.turningSteps = randomRange(20u, 30u);
		sh.chargingSteps = randomRange(60u, 180u);
		sh.stepsLeft = sh.turningSteps;
		DEGRID_COMPONENT(monster, m, shielder);
		m.dispersion = 0.2;
		monsterReflectMutation(shielder, special);
	}
	{
		CAGE_COMPONENT_ENGINE(transform, transformShielder, shielder);
		CAGE_COMPONENT_ENGINE(transform, transform, shield);
		transform = transformShielder;
		DEGRID_COMPONENT(shield, sh, shield);
		sh.active = false;
	}
}
