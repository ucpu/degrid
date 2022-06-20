#include <cage-core/macros.h>
#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

namespace
{
	struct BossEggComponent
	{
		uint32 countdown = 10;
		uint32 portal = 0;
	};

	void hatchEgg(Entity *e)
	{
		static constexpr const uint32 skyboxNames[] = {
			#define GCHL_GENERATE(N) HashString("degrid/environment/skyboxes/skybox.obj;" CAGE_STRINGIZE(N)),
					GCHL_GENERATE(0)
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
			#undef GCHL_GENERATE
		};
		setSkybox(skyboxNames[game.defeatedBosses + 1]);
		monsterExplosion(e);
		const TransformComponent &t = e->value<TransformComponent>();
		const RenderComponent &r = e->value<RenderComponent>();
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
		auto c = engineEntities()->defineComponent(BossEggComponent());
		eggDestroyedListener.attach(c->group()->entityRemoved);
		eggDestroyedListener.bind<&eggDestroyed>();
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		if ((statistics.updateIteration % 5) == 0 && engineEntities()->component<BossEggComponent>()->group()->count() > 0)
		{
			if (engineEntities()->component<MonsterComponent>()->count() == engineEntities()->component<BossEggComponent>()->count())
			{
				entitiesVisitor([](Entity *e, const TransformComponent &t, BossEggComponent &m) {
					if (length(game.monstersTarget - t.position) > 100)
						return;
					if (m.countdown-- == 0)
					{
						e->add(entitiesToDestroy);
						hatchEgg(e);
					}
				}, engineEntities(), false);
			}
			else
				makeAnnouncement(HashString("announcement/incoming-boss"), HashString("announcement-desc/incoming-boss"), 5);
		}

		entitiesVisitor([](const BossEggComponent &, const TransformComponent &tr, VelocityComponent &mv) {
			Vec3 v = game.monstersTarget - tr.position;
			Real l = length(v);
			mv.velocity = normalize(v) * pow(max(l - 70, 0) * 0.03, 1.5);
		}, engineEntities(), false);
	}

	void lateUpdate()
	{
		entitiesVisitor([](const BossEggComponent &egg, const TransformComponent &tr) {
			TransformComponent &portal = engineEntities()->get(egg.portal)->value<TransformComponent>();
			portal.position = tr.position;
			portal.scale = tr.scale * 0.88;
		}, engineEntities(), false);
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

void spawnBossEgg(const Vec3 &spawnPosition, const Vec3 &color)
{
	CAGE_ASSERT(engineEntities()->component<BossComponent>()->count() == 0);
	if (game.defeatedBosses >= BossesTotalCount)
		return;
	Entity *e = initializeMonster(spawnPosition, color, 10, HashString("degrid/boss/egg.object"), 0, Real::Infinity(), Real::Infinity());
	BossEggComponent &eggc = e->value<BossEggComponent>();
	{
		e->value<BossComponent>();
		e->value<RotationComponent>().rotation = interpolate(Quat(), randomDirectionQuat(), 0.03);
		e->value<GravityComponent>().strength = -10;
	}

	{ // portal
		Entity *p = engineEntities()->createUnique();
		eggc.portal = p->name();
		RenderComponent &r = p->value<RenderComponent>();
		static constexpr const uint32 portalNames[] = {
			#define GCHL_GENERATE(N) HashString("degrid/environment/skyboxes/portal.obj;" CAGE_STRINGIZE(N)),
					GCHL_GENERATE(0)
					CAGE_EVAL_MEDIUM(CAGE_REPEAT(20, GCHL_GENERATE))
			#undef GCHL_GENERATE
		};
		r.object = portalNames[game.defeatedBosses + 1];
		p->value<RotationComponent>().rotation = interpolate(Quat(), randomDirectionQuat(), 0.003);
		p->value<TransformComponent>().orientation = randomDirectionQuat();
	}
}
