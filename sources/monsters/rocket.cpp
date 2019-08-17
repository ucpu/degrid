#include "monsters.h"

namespace
{
	struct rocketMonsterComponent
	{
		static entityComponent *component;
	};

	entityComponent *rocketMonsterComponent::component;

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
		for (entity *e : rocketMonsterComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, tr, e);
			if (squaredLength(tr.position) > disapearDistance2)
				e->add(entitiesToDestroy);
			else
			{
				tr.orientation = tr.orientation * quat(degs(), degs(), degs(3));

				entity *spark = entities()->createAnonymous();
				CAGE_COMPONENT_ENGINE(transform, transform, spark);
				transform.scale = randomChance() * 0.2 + 0.3;
				transform.position = tr.position + (tr.orientation * vec3(0, 0, 1.2) + randomDirection3() * 0.3) * tr.scale;
				transform.orientation = randomDirectionQuat();
				CAGE_COMPONENT_ENGINE(render, render, spark);
				render.object = hashString("degrid/environment/spark.object");
				DEGRID_COMPONENT(velocity, v, e);
				DEGRID_COMPONENT(velocity, vel, spark);
				vel.velocity = (v.velocity + randomDirection3() * 0.05) * randomChance() * -0.5;
				DEGRID_COMPONENT(timeout, ttl, spark);
				ttl.ttl = randomRange(10, 15);
				CAGE_COMPONENT_ENGINE(textureAnimation, at, spark);
				at.startTime = currentControlTime();
				at.speed = 30.f / ttl.ttl;
				spark->add(entitiesPhysicsEvenWhenPaused);
			}
		}
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
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
	entity *e = initializeMonster(spawnPosition, color, 2.5, hashString("degrid/monster/rocket.object"), hashString("degrid/monster/bum-rocket.ogg"), 6, 2 + monsterMutation(special));
	DEGRID_COMPONENT(rocketMonster, r, e);
	DEGRID_COMPONENT(velocity, v, e);
	v.velocity = game.monstersTarget - spawnPosition;
	v.velocity[1] = 0;
	v.velocity = normalize(v.velocity) * (1.5 + 0.3 * monsterMutation(special));
	CAGE_COMPONENT_ENGINE(transform, tr, e);
	tr.orientation = quat(v.velocity, vec3(0, 1, 0), true);
	monsterReflectMutation(e, special);
}

