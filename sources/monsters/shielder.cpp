#include "monsters.h"

namespace
{
	struct ShielderComponent
	{
		static EntityComponent *component;
		uint32 shieldEntity;
		real movementSpeed;
		uint32 chargingSteps;
		uint32 turningSteps;
		uint32 stepsLeft;
		ShielderComponent() : chargingSteps(0), turningSteps(0), stepsLeft(0) {}
	};

	struct ShieldComponent
	{
		static EntityComponent *component;
		bool active;
		ShieldComponent() : active(false) {}
	};

	EntityComponent *ShielderComponent::component;
	EntityComponent *ShieldComponent::component;

	void updateShields()
	{
		// update shields transformations
		for (Entity *e : ShielderComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, et, e);
			DEGRID_COMPONENT(Shielder, es, e);
			Entity *s = engineEntities()->get(es.shieldEntity);
			CAGE_COMPONENT_ENGINE(Transform, st, s);
			st = et;
		}
	}

	void shielderEliminated(Entity *e)
	{
		DEGRID_COMPONENT(Shielder, sh, e);
		if (engineEntities()->has(sh.shieldEntity))
			engineEntities()->get(sh.shieldEntity)->add(entitiesToDestroy);
	}

	EventListener<void(Entity*)> shielderEliminatedListener;

	void engineInit()
	{
		ShielderComponent::component = engineEntities()->defineComponent(ShielderComponent(), true);
		ShieldComponent::component = engineEntities()->defineComponent(ShieldComponent(), true);
		shielderEliminatedListener.bind<&shielderEliminated>();
		shielderEliminatedListener.attach(ShielderComponent::component->group()->entityRemoved);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("shielder");

		if (game.paused)
			return;

		for (Entity *e : ShielderComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(Velocity, mv, e);
			DEGRID_COMPONENT(Monster, ms, e);
			DEGRID_COMPONENT(Shielder, sh, e);
			Entity *se = engineEntities()->get(sh.shieldEntity);
			DEGRID_COMPONENT(Shield, sse, se);

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
				CAGE_COMPONENT_ENGINE(Render, render, se);
				render.object = HashString("degrid/monster/shield.object");
			}
			else
			{
				se->remove(RenderComponent::component);
				continue;
			}

			// destroy shots
			vec3 forward = tr.orientation * vec3(0, 0, -1);
			SpatialSearchQuery->intersection(sphere(tr.position + forward * (tr.scale + 1), 5));
			for (uint32 otherName : SpatialSearchQuery->result())
			{
				Entity *e = engineEntities()->get(otherName);
				if (!e->has(ShotComponent::component))
					continue;
				CAGE_COMPONENT_ENGINE(Transform, ot, e);
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

void spawnShielder(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	Entity *shielder = initializeMonster(spawnPosition, color, 3, HashString("degrid/monster/shielder.object"), HashString("degrid/monster/bum-shielder.ogg"), 5, 3 + monsterMutation(special));
	Entity *shield = engineEntities()->createUnique();
	{
		DEGRID_COMPONENT(Shielder, sh, shielder);
		sh.shieldEntity = shield->name();
		sh.movementSpeed = 0.8 + 0.2 * monsterMutation(special);
		sh.turningSteps = randomRange(20u, 30u);
		sh.chargingSteps = randomRange(60u, 180u);
		sh.stepsLeft = sh.turningSteps;
		DEGRID_COMPONENT(Monster, m, shielder);
		m.dispersion = 0.2;
		monsterReflectMutation(shielder, special);
	}
	{
		CAGE_COMPONENT_ENGINE(Transform, transformShielder, shielder);
		CAGE_COMPONENT_ENGINE(Transform, transform, shield);
		transform = transformShielder;
		DEGRID_COMPONENT(Shield, sh, shield);
		sh.active = false;
	}
}
