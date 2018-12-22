#include "monsters.h"

namespace
{
	struct shockerComponent
	{
		static componentClass *component;
		real radius;
	};

	componentClass *shockerComponent::component;

	void engineInit()
	{
		shockerComponent::component = entities()->defineComponent(shockerComponent(), true);
	}

	void lightning(const vec3 &a, const vec3 &b, const vec3 &color)
	{
		real d = a.distance(b);
		vec3 v = (b - a).normalize();
		vec3 c = (a + b) * 0.5;
		if (d > 25)
		{
			vec3 side = cross(v, vec3(0, 1, 0));
			c += side * (d * randomRange(-0.3, 0.3));
			lightning(a, c, color);
			lightning(c, b, color);
			return;
		}
		entityClass *e = entities()->createUnique();
		ENGINE_GET_COMPONENT(transform, t, e);
		t.position = c;
		t.orientation = quat(v, vec3(0, 1, 0), true);
		t.scale = 10;
		ENGINE_GET_COMPONENT(render, r, e);
		r.object = hashString("grid/monster/shocker/lightning.object");
		r.color = color;
		ENGINE_GET_COMPONENT(animatedTexture, anim, e);
		anim.offset = randomChance();
		GRID_GET_COMPONENT(timeout, ttl, e);
		ttl.ttl = 3;
		e->add(entitiesPhysicsEvenWhenPaused);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, game.playerEntity);

		for (entityClass *e : shockerComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			GRID_GET_COMPONENT(shocker, sh, e);
			vec3 v = tr.position - playerTransform.position;
			real d = v.length();

			// stay away from the player
			if (d < sh.radius * 0.9 && d > 1e-7)
			{
				GRID_GET_COMPONENT(velocity, mv, e);
				mv.velocity += v.normalize() * 0.5;
			}

			// lightning
			if (d < sh.radius)
			{
				playerVelocity.velocity *= 0.72;
				if (((statistics.updateIterationIgnorePause + e->name()) % 3) == 0)
				{
					ENGINE_GET_COMPONENT(render, r, e);
					lightning(tr.position + randomDirection3() * tr.scale, playerTransform.position + randomDirection3() * playerTransform.scale, r.color);
				}
				if (!e->has(voiceComponent::component))
				{
					ENGINE_GET_COMPONENT(voice, v, e);
					v.name = hashString("grid/monster/shocker/lightning.flac");
					v.startTime = currentControlTime();
				}
			}
			else
			{
				e->remove(voiceComponent::component);
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

void spawnShocker(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	entityClass *shocker = initializeSimple(spawnPosition, color, 3, hashString("grid/monster/shocker/shocker.object"), hashString("grid/monster/shocker/bum-shocker.ogg"), 5, 3 + monsterMutation(special), 0.3, 1, 0.7, 0.05, interpolate(quat(), randomDirectionQuat(), 0.01));
	GRID_GET_COMPONENT(shocker, sh, shocker);
	sh.radius = randomRange(70, 80) + 10 * monsterMutation(special);
	monsterReflectMutation(shocker, special);
}