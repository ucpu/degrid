#include "monsters.h"

namespace
{
	struct RocketMonsterComponent
	{
		static EntityComponent *component;
	};

	EntityComponent *RocketMonsterComponent::component;

	void engineInit()
	{
		RocketMonsterComponent::component = engineEntities()->defineComponent(RocketMonsterComponent());
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		real disapearDistance2 = MapNoPullRadius * 2;
		disapearDistance2 *= disapearDistance2;
		for (Entity *e : RocketMonsterComponent::component->entities())
		{
			TransformComponent &tr = e->value<TransformComponent>();
			if (lengthSquared(tr.position) > disapearDistance2)
				e->add(entitiesToDestroy);
			else
			{
				tr.orientation = tr.orientation * quat(degs(), degs(), degs(3));

				if ((e->name() + statistics.updateIterationIgnorePause) % 3 == 0)
				{
					Entity *spark = engineEntities()->createAnonymous();
					TransformComponent &transform = spark->value<TransformComponent>();
					transform.scale = randomChance() * 0.2 + 0.3;
					transform.position = tr.position + (tr.orientation * vec3(0, 0, 1.2) + randomDirection3() * 0.3) * tr.scale;
					transform.orientation = randomDirectionQuat();
					RenderComponent &render = spark->value<RenderComponent>();
					render.object = HashString("degrid/environment/spark.object");
					VelocityComponent &v = e->value<VelocityComponent>();
					VelocityComponent &vel = spark->value<VelocityComponent>();
					vel.velocity = (v.velocity + randomDirection3() * 0.05) * randomChance() * -0.5;
					TimeoutComponent &ttl = spark->value<TimeoutComponent>();
					ttl.ttl = randomRange(10, 15);
					TextureAnimationComponent &at = spark->value<TextureAnimationComponent>();
					at.startTime = engineControlTime();
					at.speed = 30.f / ttl.ttl;
					spark->add(entitiesPhysicsEvenWhenPaused);
				}
			}
		}
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
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
	RocketMonsterComponent &r = e->value<RocketMonsterComponent>();
	VelocityComponent &v = e->value<VelocityComponent>();
	v.velocity = game.monstersTarget - spawnPosition;
	v.velocity[1] = 0;
	v.velocity = normalize(v.velocity) * (1.5 + 0.3 * monsterMutation(special));
	TransformComponent &tr = e->value<TransformComponent>();
	tr.orientation = quat(v.velocity, vec3(0, 1, 0), true);
	monsterReflectMutation(e, special);
}

