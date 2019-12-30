#include "monsters.h"

namespace
{
	struct rocketMonsterComponent
	{
		static EntityComponent *component;
	};

	EntityComponent *rocketMonsterComponent::component;

	void engineInit()
	{
		rocketMonsterComponent::component = entities()->defineComponent(rocketMonsterComponent(), true);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("rocket");

		if (game.paused)
			return;

		real disapearDistance2 = mapNoPullRadius * 2;
		disapearDistance2 *= disapearDistance2;
		for (Entity *e : rocketMonsterComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			if (lengthSquared(tr.position) > disapearDistance2)
				e->add(entitiesToDestroy);
			else
			{
				tr.orientation = tr.orientation * quat(degs(), degs(), degs(3));

				Entity *spark = entities()->createAnonymous();
				CAGE_COMPONENT_ENGINE(Transform, transform, spark);
				transform.scale = randomChance() * 0.2 + 0.3;
				transform.position = tr.position + (tr.orientation * vec3(0, 0, 1.2) + randomDirection3() * 0.3) * tr.scale;
				transform.orientation = randomDirectionQuat();
				CAGE_COMPONENT_ENGINE(Render, render, spark);
				render.object = HashString("degrid/environment/spark.object");
				DEGRID_COMPONENT(velocity, v, e);
				DEGRID_COMPONENT(velocity, vel, spark);
				vel.velocity = (v.velocity + randomDirection3() * 0.05) * randomChance() * -0.5;
				DEGRID_COMPONENT(timeout, ttl, spark);
				ttl.ttl = randomRange(10, 15);
				CAGE_COMPONENT_ENGINE(TextureAnimation, at, spark);
				at.startTime = currentControlTime();
				at.speed = 30.f / ttl.ttl;
				spark->add(entitiesPhysicsEvenWhenPaused);
			}
		}
	}

	class callbacksClass
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

void spawnRocket(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	Entity *e = initializeMonster(spawnPosition, color, 2.5, HashString("degrid/monster/rocket.object"), HashString("degrid/monster/bum-rocket.ogg"), 6, 2 + monsterMutation(special));
	DEGRID_COMPONENT(rocketMonster, r, e);
	DEGRID_COMPONENT(velocity, v, e);
	v.velocity = game.monstersTarget - spawnPosition;
	v.velocity[1] = 0;
	v.velocity = normalize(v.velocity) * (1.5 + 0.3 * monsterMutation(special));
	CAGE_COMPONENT_ENGINE(Transform, tr, e);
	tr.orientation = quat(v.velocity, vec3(0, 1, 0), true);
	monsterReflectMutation(e, special);
}

