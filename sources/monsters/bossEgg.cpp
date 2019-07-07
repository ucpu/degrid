#include "monsters.h"

namespace
{
	struct bossEggComponent
	{
		static entityComponent *component;
		
		uint32 countdown;
		
		bossEggComponent() : countdown(10)
		{}
	};

	entityComponent *bossEggComponent::component;

	void hatchEgg(entity *e)
	{
		static const uint32 skyboxNames[] = {
			#define GCHL_GENERATE(N) hashString("degrid/environment/skyboxes/skybox.obj;" CAGE_STRINGIZE(N)),
					GCHL_GENERATE(0)
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
			#undef GCHL_GENERATE
		};
		setSkybox(skyboxNames[game.defeatedBosses + 1]);
		monsterExplosion(e);
		CAGE_COMPONENT_ENGINE(transform, t, e);
		CAGE_COMPONENT_ENGINE(render, r, e);
		switch (game.defeatedBosses)
		{
		case 0: return spawnBossCannoneer(t.position, r.color);
		case 1: return spawnBossCannoneer(t.position, r.color); // todo other bosses
		case 2: return spawnBossCannoneer(t.position, r.color);
		case 3: return spawnBossCannoneer(t.position, r.color);
		case 4: return spawnBossCannoneer(t.position, r.color);
		default:
			CAGE_THROW_CRITICAL(notImplemented, "hatchEgg");
		}
	}

	void engineInit()
	{
		bossEggComponent::component = entities()->defineComponent(bossEggComponent(), true);
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
				for (entity *e : bossEggComponent::component->entities())
				{
					CAGE_COMPONENT_ENGINE(transform, t, e);
					if (length(game.monstersTarget - t.position) > 90)
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
				makeAnnouncement(hashString("announcement/incoming-boss"), hashString("announcement-desc/incoming-boss"), 5);
		}

		for (entity *e : bossEggComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, tr, e);
			DEGRID_COMPONENT(velocity, mv, e);
			vec3 v = game.monstersTarget - tr.position;
			real l = v.length();
			mv.velocity = normalize(v) * pow(max(l - 70, 0) * 0.03, 1.5);
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

void spawnBossEgg(const vec3 &spawnPosition, const vec3 &color)
{
	CAGE_ASSERT_RUNTIME(bossComponent::component->group()->count() == 0);
	if (game.defeatedBosses >= bossesTotalCount)
		return;
	entity *e = initializeMonster(spawnPosition, color, 10, hashString("degrid/boss/egg.object"), 0, real::Infinity(), real::Infinity());
	DEGRID_COMPONENT(boss, boss, e);
	DEGRID_COMPONENT(bossEgg, eggc, e);
	DEGRID_COMPONENT(rotation, rot, e);
	rot.rotation = interpolate(quat(), randomDirectionQuat(), 0.05);
}
