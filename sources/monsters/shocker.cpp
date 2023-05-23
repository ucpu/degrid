#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

namespace
{
	struct ShockerComponent
	{
		Real radius;
		Real speedFactor;
	};

	const auto engineInitListener = controlThread().initialize.listen([]() {
		engineEntities()->defineComponent(ShockerComponent());
	});

	void lightning(const Vec3 &a, const Vec3 &b, const Vec3 &color)
	{
		const Real d = distance(a, b);
		const Vec3 v = normalize(b - a);
		Vec3 c = (a + b) * 0.5;
		if (d > 25)
		{
			const Vec3 side = normalize(cross(v, Vec3(0, 1, 0)));
			c += side * (d * randomRange(-0.2, 0.2));
			lightning(a, c, color);
			lightning(c, b, color);
			return;
		}
		Entity *e = engineEntities()->createUnique();
		TransformComponent &t = e->value<TransformComponent>();
		t.position = c;
		t.orientation = Quat(v, Vec3(0, 1, 0), true);
		RenderComponent &r = e->value<RenderComponent>();
		r.object = HashString("degrid/monster/shocker/lightning.object");
		r.color = color;
		e->value<TextureAnimationComponent>().offset = randomChance();
		e->value<TimeoutComponent>().ttl = 3;
		e->add(entitiesPhysicsEvenWhenPaused);
		LightComponent &light = e->value<LightComponent>();
		light.color = colorVariation(color);
		light.intensity = 10;
		light.lightType = LightTypeEnum::Point;
		light.attenuation = Vec3(0, 0, 0.01);
	}

	const auto engineUpdateListener = controlThread().update.listen([]() {
		if (game.paused)
		{
			entitiesVisitor([&](Entity *e, const ShockerComponent &) {
				e->remove<SoundComponent>();
				}, engineEntities(), false);
			return;
		}

		const TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
		VelocityComponent &playerVelocity = game.playerEntity->value<VelocityComponent>();

		entitiesVisitor([&](Entity *e, TransformComponent &tr, const ShockerComponent &sh) {
			const Vec3 v = tr.position - playerTransform.position;
			const Real d = length(v);

			// stay away from the player
			if (d < sh.radius * 0.8 && d > 1e-7)
				e->value<VelocityComponent>().velocity += normalize(v) * 0.3;

			// lightning
			if (d < sh.radius)
			{
				playerVelocity.velocity *= sh.speedFactor;
				if (((statistics.updateIterationIgnorePause + e->name()) % 3) == 0)
					lightning(tr.position + randomDirection3() * tr.scale, playerTransform.position + randomDirection3() * playerTransform.scale, e->value<RenderComponent>().color);
				if (!e->has<SoundComponent>())
				{
					SoundComponent &v = e->value<SoundComponent>();
					v.name = HashString("degrid/monster/shocker/lightning.flac");
					v.startTime = uint64(e->name()) * 10000;
				}
			}
			else
				e->remove<SoundComponent>();
		}, engineEntities(), false);
	});
}

void spawnShocker(const Vec3 &spawnPosition, const Vec3 &color)
{
	uint32 special = 0;
	Entity *shocker = initializeSimple(spawnPosition, color, 3, HashString("degrid/monster/shocker/shocker.object"), HashString("degrid/monster/shocker/bum-shocker.ogg"), 5, 3 + monsterMutation(special), 0.3, 1, 0.7, 0.05, 0.7, 0, interpolate(Quat(), randomDirectionQuat(), 0.01));
	ShockerComponent &sh = shocker->value<ShockerComponent>();
	sh.radius = randomRange(70, 80) + 10 * monsterMutation(special);
	sh.speedFactor = 3.2 / (monsterMutation(special) + 4);
	monsterReflectMutation(shocker, special);
}
