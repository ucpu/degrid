#include "monsters.h"

namespace
{
	struct bossEggComponent
	{
		static componentClass *component;
		
		uint32 countdown;
		
		bossEggComponent() : countdown(10)
		{}
	};

	componentClass *bossEggComponent::component;

	void hatchEgg(entityClass *e)
	{
		static const uint32 skyboxNames[] = {
			#define GCHL_GENERATE(N) hashString("grid/environment/skyboxes/skybox.obj;" CAGE_STRINGIZE(N)),
					GCHL_GENERATE(0)
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
			#undef GCHL_GENERATE
		};
		setSkybox(skyboxNames[game.defeatedBosses + 1]);
		monsterExplosion(e);
		ENGINE_GET_COMPONENT(transform, t, e);
		ENGINE_GET_COMPONENT(render, r, e);
		switch (game.defeatedBosses)
		{
		case 0: return spawnBossCannoneer(t.position, r.color);
		case 1: return spawnBossCannoneer(t.position, r.color); // todo other bosses
		case 2: return spawnBossCannoneer(t.position, r.color);
		case 3: return spawnBossCannoneer(t.position, r.color);
		case 4: return spawnBossCannoneer(t.position, r.color);
		default:
			CAGE_THROW_CRITICAL(notImplementedException, "hatchEgg");
		}
	}

	void engineInit()
	{
		bossEggComponent::component = entities()->defineComponent(bossEggComponent(), true);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		if (statistics.updateIteration % 5 == 0 && bossEggComponent::component->group()->count() > 0)
		{
			if (monsterComponent::component->group()->count() == bossEggComponent::component->group()->count())
			{
				for (entityClass *e : bossEggComponent::component->entities())
				{
					ENGINE_GET_COMPONENT(transform, t, e);
					if (length(game.monstersTarget - t.position) > 90)
						continue;
					GRID_GET_COMPONENT(bossEgg, m, e);
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

		for (entityClass *e : bossEggComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			GRID_GET_COMPONENT(velocity, mv, e);
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
	entityClass *e = initializeMonster(spawnPosition, color, 4, hashString("grid/boss/egg.object"), 0, real::PositiveInfinity, real::PositiveInfinity);
	GRID_GET_COMPONENT(boss, boss, e);
	GRID_GET_COMPONENT(bossEgg, eggc, e);
	GRID_GET_COMPONENT(rotation, rot, e);
	rot.rotation = interpolate(quat(), randomDirectionQuat(), 0.05);
}
