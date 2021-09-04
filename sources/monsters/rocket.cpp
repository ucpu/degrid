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

		Real disapearDistance2 = MapNoPullRadius * 2;
		disapearDistance2 *= disapearDistance2;
		for (Entity *e : RocketMonsterComponent::component->entities())
		{
			TransformComponent &tr = e->value<TransformComponent>();
			if (lengthSquared(tr.position) > disapearDistance2)
				e->add(entitiesToDestroy);
			else
			{
				tr.orientation = tr.orientation * Quat(Degs(), Degs(), Degs(3));

				if ((e->name() + statistics.updateIterationIgnorePause) % 3 == 0)
				{
					Entity *spark = engineEntities()->createAnonymous();
					TransformComponent &Transform = spark->value<TransformComponent>();
					Transform.scale = randomChance() * 0.2 + 0.3;
					Transform.position = tr.position + (tr.orientation * Vec3(0, 0, 1.2) + randomDirection3() * 0.3) * tr.scale;
					Transform.orientation = randomDirectionQuat();
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

void spawnRocket(const Vec3 &spawnPosition, const Vec3 &color)
{
	uint32 special = 0;
	Entity *e = initializeMonster(spawnPosition, color, 2.5, HashString("degrid/monster/rocket.object"), HashString("degrid/monster/bum-rocket.ogg"), 6, 2 + monsterMutation(special));
	RocketMonsterComponent &r = e->value<RocketMonsterComponent>();
	VelocityComponent &v = e->value<VelocityComponent>();
	v.velocity = game.monstersTarget - spawnPosition;
	v.velocity[1] = 0;
	v.velocity = normalize(v.velocity) * (1.5 + 0.3 * monsterMutation(special));
	TransformComponent &tr = e->value<TransformComponent>();
	tr.orientation = Quat(v.velocity, Vec3(0, 1, 0), true);
	monsterReflectMutation(e, special);
}

