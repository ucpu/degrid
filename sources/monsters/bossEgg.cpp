#include "monsters.h"

namespace
{
	struct bossEggComponent
	{
		static EntityComponent *component;
		
		uint32 countdown;
		uint32 portal;
		
		bossEggComponent() : countdown(10), portal(0)
		{}
	};

	EntityComponent *bossEggComponent::component;

	void hatchEgg(Entity *e)
	{
		static const uint32 skyboxNames[] = {
			#define GCHL_GENERATE(N) HashString("degrid/environment/skyboxes/skybox.obj;" CAGE_STRINGIZE(N)),
					GCHL_GENERATE(0)
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
			#undef GCHL_GENERATE
		};
		setSkybox(skyboxNames[game.defeatedBosses + 1]);
		monsterExplosion(e);
		CAGE_COMPONENT_ENGINE(Transform, t, e);
		CAGE_COMPONENT_ENGINE(Render, r, e);
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
		DEGRID_COMPONENT(bossEgg, egg, e);
		if (entities()->has(egg.portal))
			entities()->get(egg.portal)->destroy();
	}

	EventListener<void(Entity *e)> eggDestroyedListener;
	void engineInit()
	{
		bossEggComponent::component = entities()->defineComponent(bossEggComponent(), true);
		eggDestroyedListener.attach(bossEggComponent::component->group()->entityRemoved);
		eggDestroyedListener.bind<&eggDestroyed>();
	}

	void engineUpdate()
	{
		OPTICK_EVENT("boss egg");

		if (game.paused)
			return;

		if (statistics.updateIteration % 5 == 0 && bossEggComponent::component->group()->count() > 0)
		{
			if (monsterComponent::component->group()->count() == bossEggComponent::component->group()->count())
			{
				for (Entity *e : bossEggComponent::component->entities())
				{
					CAGE_COMPONENT_ENGINE(Transform, t, e);
					if (length(game.monstersTarget - t.position) > 100)
						continue;
					DEGRID_COMPONENT(bossEgg, m, e);
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

		for (Entity *e : bossEggComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(velocity, mv, e);
			vec3 v = game.monstersTarget - tr.position;
			real l = length(v);
			mv.velocity = normalize(v) * pow(max(l - 70, 0) * 0.03, 1.5);
		}
	}

	void lateUpdate()
	{
		for (Entity *e : bossEggComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(bossEgg, egg, e);
			CAGE_COMPONENT_ENGINE(Transform, portal, entities()->get(egg.portal));
			portal.position = tr.position;
			portal.scale = tr.scale * 0.88;
		}
	}

	class callbacksClass
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
		EventListener<void()> EngineLateUpdateListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
			EngineLateUpdateListener.attach(controlThread().update, 42); // after physics
			EngineLateUpdateListener.bind<&lateUpdate>();
		}
	} callbacksInstance;
}

void spawnBossEgg(const vec3 &spawnPosition, const vec3 &color)
{
	CAGE_ASSERT(bossComponent::component->group()->count() == 0);
	if (game.defeatedBosses >= bossesTotalCount)
		return;
	Entity *e = initializeMonster(spawnPosition, color, 10, HashString("degrid/boss/egg.object"), 0, real::Infinity(), real::Infinity());
	DEGRID_COMPONENT(bossEgg, eggc, e);
	{
		DEGRID_COMPONENT(boss, boss, e);
		DEGRID_COMPONENT(rotation, rot, e);
		rot.rotation = interpolate(quat(), randomDirectionQuat(), 0.03);
		DEGRID_COMPONENT(gravity, grav, e);
		grav.strength = -10;
	}

	{ // portal
		Entity *p = entities()->createUnique();
		eggc.portal = p->name();
		CAGE_COMPONENT_ENGINE(Render, r, p);
		static const uint32 portalNames[] = {
			#define GCHL_GENERATE(N) HashString("degrid/environment/skyboxes/portal.obj;" CAGE_STRINGIZE(N)),
					GCHL_GENERATE(0)
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
			#undef GCHL_GENERATE
		};
		r.object = portalNames[game.defeatedBosses + 1];
		DEGRID_COMPONENT(rotation, rotp, p);
		rotp.rotation = interpolate(quat(), randomDirectionQuat(), 0.003);
		CAGE_COMPONENT_ENGINE(Transform, t, p);
		t.orientation = randomDirectionQuat();
	}
}
