#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

namespace
{
	struct RocketMonsterComponent
	{};

	const auto engineInitListener = controlThread().initialize.listen([]() {
		engineEntities()->defineComponent(RocketMonsterComponent());
	});

	const auto engineUpdateListener = controlThread().update.listen([]() {
		if (game.paused)
			return;

		entitiesVisitor([&](Entity *e, const RocketMonsterComponent &, TransformComponent &tr) {
			static constexpr Real DisapearDistance2 = sqr(MapNoPullRadius * 2);
			if (lengthSquared(tr.position) > DisapearDistance2)
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
					spark->value<RenderComponent>().object = HashString("degrid/environment/spark.object");
					spark->value<VelocityComponent>().velocity = (e->value<VelocityComponent>().velocity + randomDirection3() * 0.05) * randomChance() * -0.5;
					TimeoutComponent &ttl = spark->value<TimeoutComponent>();
					ttl.ttl = randomRange(10, 15);
					TextureAnimationComponent &at = spark->value<TextureAnimationComponent>();
					at.startTime = engineControlTime();
					at.speed = 30.f / ttl.ttl;
					spark->add(entitiesPhysicsEvenWhenPaused);
				}
			}
			}, engineEntities(), false);
	});
}

void spawnRocket(const Vec3 &spawnPosition, const Vec3 &color)
{
	uint32 special = 0;
	Entity *e = initializeMonster(spawnPosition, color, 2.5, HashString("degrid/monster/rocket.object"), HashString("degrid/monster/bum-rocket.ogg"), 6, 2 + monsterMutation(special));
	e->value<RocketMonsterComponent>();
	VelocityComponent &v = e->value<VelocityComponent>();
	v.velocity = game.monstersTarget - spawnPosition;
	v.velocity[1] = 0;
	v.velocity = normalize(v.velocity) * (1.5 + 0.3 * monsterMutation(special));
	e->value<TransformComponent>().orientation = Quat(v.velocity, Vec3(0, 1, 0), true);
	monsterReflectMutation(e, special);
}

