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
		ShockerComponent::component = entities()->defineComponent(ShockerComponent(), true);
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
		Entity *e = entities()->createUnique();
		CAGE_COMPONENT_ENGINE(Transform, t, e);
		t.position = c;
		t.orientation = quat(v, vec3(0, 1, 0), true);
		CAGE_COMPONENT_ENGINE(Render, r, e);
		r.object = HashString("degrid/monster/shocker/lightning.object");
		r.color = color;
		CAGE_COMPONENT_ENGINE(TextureAnimation, anim, e);
		anim.offset = randomChance();
		DEGRID_COMPONENT(Timeout, ttl, e);
		ttl.ttl = 3;
		e->add(entitiesPhysicsEvenWhenPaused);
		CAGE_COMPONENT_ENGINE(Light, light, e);
		light.color = colorVariation(color) * 10;
		light.lightType = LightTypeEnum::Point;
		light.attenuation = vec3(0, 0, 0.01);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("shocker");

		if (game.paused)
		{
			for (Entity *e : ShockerComponent::component->entities())
				e->remove(SoundComponent::component);
			return;
		}

		CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
		DEGRID_COMPONENT(Velocity, playerVelocity, game.playerEntity);

		for (Entity *e : ShockerComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(Shocker, sh, e);
			vec3 v = tr.position - playerTransform.position;
			real d = length(v);

			// stay away from the player
			if (d < sh.radius * 0.8 && d > 1e-7)
			{
				DEGRID_COMPONENT(Velocity, mv, e);
				mv.velocity += normalize(v) * 0.3;
			}

			// lightning
			if (d < sh.radius)
			{
				playerVelocity.velocity *= sh.speedFactor;
				if (((statistics.updateIterationIgnorePause + e->name()) % 3) == 0)
				{
					CAGE_COMPONENT_ENGINE(Render, r, e);
					lightning(tr.position + randomDirection3() * tr.scale, playerTransform.position + randomDirection3() * playerTransform.scale, r.color);
				}
				if (!e->has(SoundComponent::component))
				{
					CAGE_COMPONENT_ENGINE(Sound, v, e);
					v.name = HashString("degrid/monster/shocker/lightning.flac");
					v.startTime = uint64(e->name()) * 10000;
				}
			}
			else
				e->remove(SoundComponent::component);
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
	DEGRID_COMPONENT(Shocker, sh, shocker);
	sh.radius = randomRange(70, 80) + 10 * monsterMutation(special);
	sh.speedFactor = 3.2 / (monsterMutation(special) + 4);
	monsterReflectMutation(shocker, special);
}
