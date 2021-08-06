#include "monsters.h"

namespace
{
	struct ShielderComponent
	{
		static EntityComponent *component;
		uint32 shieldEntity = 0;
		uint32 chargingSteps = 0;
		uint32 turningSteps = 0;
		uint32 stepsLeft = 0;
		real movementSpeed;
	};

	struct ShieldComponent
	{
		static EntityComponent *component;
		bool active = false;
	};

	EntityComponent *ShielderComponent::component;
	EntityComponent *ShieldComponent::component;

	void updateShields()
	{
		// update shields transformations
		for (Entity *e : ShielderComponent::component->entities())
		{
			TransformComponent &et = e->value<TransformComponent>();
			ShielderComponent &es = e->value<ShielderComponent>();
			Entity *s = engineEntities()->get(es.shieldEntity);
			TransformComponent &st = s->value<TransformComponent>();
			st = et;
		}
	}

	void shielderEliminated(Entity *e)
	{
		ShielderComponent &sh = e->value<ShielderComponent>();
		if (engineEntities()->has(sh.shieldEntity))
			engineEntities()->get(sh.shieldEntity)->add(entitiesToDestroy);
	}

	EventListener<void(Entity*)> shielderEliminatedListener;

	void engineInit()
	{
		ShielderComponent::component = engineEntities()->defineComponent(ShielderComponent());
		ShieldComponent::component = engineEntities()->defineComponent(ShieldComponent());
		shielderEliminatedListener.bind<&shielderEliminated>();
		shielderEliminatedListener.attach(ShielderComponent::component->group()->entityRemoved);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		for (Entity *e : ShielderComponent::component->entities())
		{
			TransformComponent &tr = e->value<TransformComponent>();
			VelocityComponent &mv = e->value<VelocityComponent>();
			MonsterComponent &ms = e->value<MonsterComponent>();
			ShielderComponent &sh = e->value<ShielderComponent>();
			Entity *se = engineEntities()->get(sh.shieldEntity);
			ShieldComponent &sse = se->value<ShieldComponent>();

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
				RenderComponent &render = se->value<RenderComponent>();
				render.object = HashString("degrid/monster/shield.object");
			}
			else
			{
				se->remove<RenderComponent>();
				continue;
			}

			// destroy shots
			vec3 forward = tr.orientation * vec3(0, 0, -1);
			spatialSearchQuery->intersection(Sphere(tr.position + forward * (tr.scale + 1), 5));
			for (uint32 otherName : spatialSearchQuery->result())
			{
				Entity *e = engineEntities()->get(otherName);
				if (!e->has<ShotComponent>())
					continue;
				TransformComponent &ot = e->value<TransformComponent>();
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
		ShielderComponent &sh = shielder->value<ShielderComponent>();
		sh.shieldEntity = shield->name();
		sh.movementSpeed = 0.8 + 0.2 * monsterMutation(special);
		sh.turningSteps = randomRange(20u, 30u);
		sh.chargingSteps = randomRange(60u, 180u);
		sh.stepsLeft = sh.turningSteps;
		MonsterComponent &m = shielder->value<MonsterComponent>();
		m.dispersion = 0.2;
		monsterReflectMutation(shielder, special);
	}
	{
		TransformComponent &transformShielder = shielder->value<TransformComponent>();
		TransformComponent &transform = shield->value<TransformComponent>();
		transform = transformShielder;
		ShieldComponent &sh = shield->value<ShieldComponent>();
		sh.active = false;
	}
}
