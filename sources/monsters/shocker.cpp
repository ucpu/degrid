#include "monsters.h"

namespace
{
	struct shockerComponent
	{
		static entityComponent *component;
		real radius;
		real speedFactor;
	};

	entityComponent *shockerComponent::component;

	void engineInit()
	{
		shockerComponent::component = entities()->defineComponent(shockerComponent(), true);
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
		entity *e = entities()->createUnique();
		CAGE_COMPONENT_ENGINE(transform, t, e);
		t.position = c;
		t.orientation = quat(v, vec3(0, 1, 0), true);
		CAGE_COMPONENT_ENGINE(render, r, e);
		r.object = hashString("degrid/monster/shocker/lightning.object");
		r.color = color;
		CAGE_COMPONENT_ENGINE(textureAnimation, anim, e);
		anim.offset = randomChance();
		DEGRID_COMPONENT(timeout, ttl, e);
		ttl.ttl = 3;
		e->add(entitiesPhysicsEvenWhenPaused);
		CAGE_COMPONENT_ENGINE(light, light, e);
		light.color = colorVariation(color) * 10;
		light.lightType = lightTypeEnum::Point;
		light.attenuation = vec3(0, 0, 0.01);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("shocker");

		if (game.paused)
		{
			for (entity *e : shockerComponent::component->entities())
				e->remove(voiceComponent::component);
			return;
		}

		CAGE_COMPONENT_ENGINE(transform, playerTransform, game.playerEntity);
		DEGRID_COMPONENT(velocity, playerVelocity, game.playerEntity);

		for (entity *e : shockerComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, tr, e);
			DEGRID_COMPONENT(shocker, sh, e);
			vec3 v = tr.position - playerTransform.position;
			real d = length(v);

			// stay away from the player
			if (d < sh.radius * 0.8 && d > 1e-7)
			{
				DEGRID_COMPONENT(velocity, mv, e);
				mv.velocity += normalize(v) * 0.3;
			}

			// lightning
			if (d < sh.radius)
			{
				playerVelocity.velocity *= sh.speedFactor;
				if (((statistics.updateIterationIgnorePause + e->name()) % 3) == 0)
				{
					CAGE_COMPONENT_ENGINE(render, r, e);
					lightning(tr.position + randomDirection3() * tr.scale, playerTransform.position + randomDirection3() * playerTransform.scale, r.color);
				}
				if (!e->has(voiceComponent::component))
				{
					CAGE_COMPONENT_ENGINE(voice, v, e);
					v.name = hashString("degrid/monster/shocker/lightning.flac");
					v.startTime = uint64(e->name()) * 10000;
				}
			}
			else
				e->remove(voiceComponent::component);
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

void spawnShocker(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	entity *shocker = initializeSimple(spawnPosition, color, 3, hashString("degrid/monster/shocker/shocker.object"), hashString("degrid/monster/shocker/bum-shocker.ogg"), 5, 3 + monsterMutation(special), 0.3, 1, 0.7, 0.05, 0.7, 0, interpolate(quat(), randomDirectionQuat(), 0.01));
	DEGRID_COMPONENT(shocker, sh, shocker);
	sh.radius = randomRange(70, 80) + 10 * monsterMutation(special);
	sh.speedFactor = 3.2 / (monsterMutation(special) + 4);
	monsterReflectMutation(shocker, special);
}
