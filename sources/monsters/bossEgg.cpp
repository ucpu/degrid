#include <cage-core/macros.h>

#include "monsters.h"

namespace
{
	struct BossEggComponent
	{
		static EntityComponent *component;
		
		uint32 countdown = 10;
		uint32 portal = 0;
	};

	EntityComponent *BossEggComponent::component;

	void hatchEgg(Entity *e)
	{
		constexpr const uint32 skyboxNames[] = {
			#define GCHL_GENERATE(N) HashString("degrid/environment/skyboxes/skybox.obj;" CAGE_STRINGIZE(N)),
					GCHL_GENERATE(0)
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
			#undef GCHL_GENERATE
		};
		setSkybox(skyboxNames[game.defeatedBosses + 1]);
		monsterExplosion(e);
		TransformComponent &t = e->value<TransformComponent>();
		RenderComponent &r = e->value<RenderComponent>();
		switch (game.defeatedBosses)
		{
		case 0: return spawnBossCannoneer(t.position, r.color);
		case 1: return spawnBossCannoneer(t.position, r.color); // todo other bosses
		case 2: return spawnBossCannoneer(t.position, r.color);
		case 3: return spawnBossCannoneer(t.position, r.color);
		case 4: return spawnBossCannoneer(t.position, r.color);
		default:
			CAGE_THROW_CRITICAL(NotImplemented, "hatchEgg");
		}
	}

	void eggDestroyed(Entity *e)
	{
		BossEggComponent &egg = e->value<BossEggComponent>();
		if (engineEntities()->has(egg.portal))
			engineEntities()->get(egg.portal)->destroy();
	}

	EventListener<void(Entity *e)> eggDestroyedListener;
	void engineInit()
	{
		BossEggComponent::component = engineEntities()->defineComponent(BossEggComponent());
		eggDestroyedListener.attach(BossEggComponent::component->group()->entityRemoved);
		eggDestroyedListener.bind<&eggDestroyed>();
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		if ((statistics.updateIteration % 5) == 0 && BossEggComponent::component->group()->count() > 0)
		{
			if (engineEntities()->component<MonsterComponent>()->count() == engineEntities()->component<BossEggComponent>()->count())
			{
				for (Entity *e : BossEggComponent::component->entities())
				{
					TransformComponent &t = e->value<TransformComponent>();
					if (length(game.monstersTarget - t.position) > 100)
						continue;
					BossEggComponent &m = e->value<BossEggComponent>();
					if (m.countdown-- == 0)
					{
						e->add(entitiesToDestroy);
						hatchEgg(e);
					}
				}
			}
			else
				makeAnnouncement(HashString("announcement/incoming-boss"), HashString("announcement-desc/incoming-boss"), 5);
		}

		for (Entity *e : engineEntities()->component<BossEggComponent>()->entities())
		{
			TransformComponent &tr = e->value<TransformComponent>();
			VelocityComponent &mv = e->value<VelocityComponent>();
			vec3 v = game.monstersTarget - tr.position;
			real l = length(v);
			mv.velocity = normalize(v) * pow(max(l - 70, 0) * 0.03, 1.5);
		}
	}

	void lateUpdate()
	{
		for (Entity *e : engineEntities()->component<BossEggComponent>()->entities())
		{
			TransformComponent &tr = e->value<TransformComponent>();
			BossEggComponent &egg = e->value<BossEggComponent>();
			TransformComponent &portal = engineEntities()->get(egg.portal)->value<TransformComponent>();
			portal.position = tr.position;
			portal.scale = tr.scale * 0.88;
		}
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
		EventListener<void()> engineLateUpdateListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
			engineLateUpdateListener.attach(controlThread().update, 42); // after physics
			engineLateUpdateListener.bind<&lateUpdate>();
		}
	} callbacksInstance;
}

void spawnBossEgg(const vec3 &spawnPosition, const vec3 &color)
{
	CAGE_ASSERT(engineEntities()->component<BossComponent>()->count() == 0);
	if (game.defeatedBosses >= BossesTotalCount)
		return;
	Entity *e = initializeMonster(spawnPosition, color, 10, HashString("degrid/boss/egg.object"), 0, real::Infinity(), real::Infinity());
	BossEggComponent &eggc = e->value<BossEggComponent>();
	{
		BossComponent &boss = e->value<BossComponent>();
		RotationComponent &rot = e->value<RotationComponent>();
		rot.rotation = interpolate(quat(), randomDirectionQuat(), 0.03);
		GravityComponent &grav = e->value<GravityComponent>();
		grav.strength = -10;
	}

	{ // portal
		Entity *p = engineEntities()->createUnique();
		eggc.portal = p->name();
		RenderComponent &r = p->value<RenderComponent>();
		constexpr const uint32 portalNames[] = {
			#define GCHL_GENERATE(N) HashString("degrid/environment/skyboxes/portal.obj;" CAGE_STRINGIZE(N)),
					GCHL_GENERATE(0)
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
			#undef GCHL_GENERATE
		};
		r.object = portalNames[game.defeatedBosses + 1];
		RotationComponent &rotp = p->value<RotationComponent>();
		rotp.rotation = interpolate(quat(), randomDirectionQuat(), 0.003);
		TransformComponent &t = p->value<TransformComponent>();
		t.orientation = randomDirectionQuat();
	}
}
