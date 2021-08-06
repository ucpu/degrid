#include "monsters.h"

namespace
{
	struct ShockerComponent
	{
		static EntityComponent *component;
		real radius;
		real speedFactor;
	};

	EntityComponent *ShockerComponent::component;

	void engineInit()
	{
		ShockerComponent::component = engineEntities()->defineComponent(ShockerComponent());
	}

	void lightning(const vec3 &a, const vec3 &b, const vec3 &color)
	{
		real d = distance(a, b);
		vec3 v = normalize(b - a);
		vec3 c = (a + b) * 0.5;
		if (d > 25)
		{
			vec3 side = normalize(cross(v, vec3(0, 1, 0)));
			c += side * (d * randomRange(-0.2, 0.2));
			lightning(a, c, color);
			lightning(c, b, color);
			return;
		}
		Entity *e = engineEntities()->createUnique();
		TransformComponent &t = e->value<TransformComponent>();
		t.position = c;
		t.orientation = quat(v, vec3(0, 1, 0), true);
		RenderComponent &r = e->value<RenderComponent>();
		r.object = HashString("degrid/monster/shocker/lightning.object");
		r.color = color;
		TextureAnimationComponent &anim = e->value<TextureAnimationComponent>();
		anim.offset = randomChance();
		TimeoutComponent &ttl = e->value<TimeoutComponent>();
		ttl.ttl = 3;
		e->add(entitiesPhysicsEvenWhenPaused);
		LightComponent &light = e->value<LightComponent>();
		light.color = colorVariation(color);
		light.intensity = 10;
		light.lightType = LightTypeEnum::Point;
		light.attenuation = vec3(0, 0, 0.01);
	}

	void engineUpdate()
	{
		if (game.paused)
		{
			for (Entity *e : ShockerComponent::component->entities())
				e->remove<SoundComponent>();
			return;
		}

		TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
		VelocityComponent &playerVelocity = game.playerEntity->value<VelocityComponent>();

		for (Entity *e : ShockerComponent::component->entities())
		{
			TransformComponent &tr = e->value<TransformComponent>();
			ShockerComponent &sh = e->value<ShockerComponent>();
			vec3 v = tr.position - playerTransform.position;
			real d = length(v);

			// stay away from the player
			if (d < sh.radius * 0.8 && d > 1e-7)
			{
				VelocityComponent &mv = e->value<VelocityComponent>();
				mv.velocity += normalize(v) * 0.3;
			}

			// lightning
			if (d < sh.radius)
			{
				playerVelocity.velocity *= sh.speedFactor;
				if (((statistics.updateIterationIgnorePause + e->name()) % 3) == 0)
				{
					RenderComponent &r = e->value<RenderComponent>();
					lightning(tr.position + randomDirection3() * tr.scale, playerTransform.position + randomDirection3() * playerTransform.scale, r.color);
				}
				if (!e->has<SoundComponent>())
				{
					SoundComponent &v = e->value<SoundComponent>();
					v.name = HashString("degrid/monster/shocker/lightning.flac");
					v.startTime = uint64(e->name()) * 10000;
				}
			}
			else
				e->remove<SoundComponent>();
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

void spawnShocker(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	Entity *shocker = initializeSimple(spawnPosition, color, 3, HashString("degrid/monster/shocker/shocker.object"), HashString("degrid/monster/shocker/bum-shocker.ogg"), 5, 3 + monsterMutation(special), 0.3, 1, 0.7, 0.05, 0.7, 0, interpolate(quat(), randomDirectionQuat(), 0.01));
	ShockerComponent &sh = shocker->value<ShockerComponent>();
	sh.radius = randomRange(70, 80) + 10 * monsterMutation(special);
	sh.speedFactor = 3.2 / (monsterMutation(special) + 4);
	monsterReflectMutation(shocker, special);
}
