#include <cage-core/enumerate.h>
#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

namespace
{
	struct BodyComponent
	{
		uint32 bulbs[24] = {};
		uint32 shieldEntity = 0;
		uint32 lastHit = 0;
		uint8 cannonsSpawned = 0;
		uint8 cannonsKilled = 0;
	};

	struct CannonComponent
	{
		uint32 bodyEntity = 0;
		uint32 index = 0; // 0 .. 7 (inclusive)
		Real extension;
		Real loading;
		Rads firingOffset;
	};

	void spawnCannon(Entity *body, uint32 index);

	void bodyEliminated(Entity *e)
	{
		const BodyComponent &b = e->value<BodyComponent>();
		if (engineEntities()->has(b.shieldEntity))
			engineEntities()->get(b.shieldEntity)->add(entitiesToDestroy);
		for (uint32 it : b.bulbs)
		{
			if (engineEntities()->has(it))
				engineEntities()->get(it)->add(entitiesToDestroy);
		}
	}

	void cannonEliminated(Entity *e)
	{
		const CannonComponent &c = e->value<CannonComponent>();
		if (engineEntities()->has(c.bodyEntity))
		{
			Entity *body = engineEntities()->get(c.bodyEntity);
			BodyComponent &b = body->value<BodyComponent>();
			if (engineEntities()->has(b.bulbs[b.cannonsKilled]))
			{
				Entity *e = engineEntities()->get(b.bulbs[b.cannonsKilled]);
				RenderComponent &r = e->value<RenderComponent>();
				r.color = Vec3(r.color[1], r.color[0], r.color[2]);
				LightComponent &l = e->value<LightComponent>();
				l.color = Vec3(l.color[1], l.color[0], l.color[2]);
			}
			b.cannonsKilled++;
			if (b.cannonsSpawned < 24)
				spawnCannon(body, c.index);
			if (b.cannonsKilled == 24)
			{
				MonsterComponent &m = body->value<MonsterComponent>();
				m.life = min(m.life, 10);
				if (engineEntities()->has(b.shieldEntity))
				{
					engineEntities()->get(b.shieldEntity)->add(entitiesToDestroy);
					b.shieldEntity = 0;
				}
			}
			b.lastHit = statistics.updateIteration;
		}
	}

	EventListener<void(Entity*)> bodyEliminatedListener;
	EventListener<void(Entity*)> cannonEliminatedListener;

	void engineInit()
	{
		EntityComponent *b = engineEntities()->defineComponent(BodyComponent());
		EntityComponent *c = engineEntities()->defineComponent(CannonComponent());
		bodyEliminatedListener.bind<&bodyEliminated>();
		bodyEliminatedListener.attach(b->group()->entityRemoved);
		cannonEliminatedListener.bind<&cannonEliminated>();
		cannonEliminatedListener.attach(c->group()->entityRemoved);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		EntityManager *ents = engineEntities();

		entitiesVisitor([&](Entity *e, BodyComponent &b, TransformComponent &bt) {
			if (ents->has(b.shieldEntity))
			{
				Real se = clamp(statistics.updateIteration - b.lastHit, 0u, 30u) / 30.0;
				se = sqrt(max(Real(), sin(se * Rads::Full() * 0.5)));
				Entity *sh = ents->get(b.shieldEntity);
				sh->value<TransformComponent>().scale = bt.scale + interpolate(0.1, 20.0, se);
			}
		}, engineEntities(), false);

		entitiesVisitor([&](Entity *e, CannonComponent &cannon) {
			if (ents->has(cannon.bodyEntity))
			{
				MonsterComponent &monster = e->value<MonsterComponent>();
				monster.life = min(monster.life + 0.002, 20);
				cannon.extension += 0.01;
				if (cannon.extension >= 1)
				{
					Entity *body = ents->get(cannon.bodyEntity);
					const BodyComponent &b = body->value<BodyComponent>();
					cannon.loading += (pow(b.cannonsKilled / 20.0, 5) + 1.0) / 70.0;
					if (cannon.loading >= 1)
					{
						cannon.loading -= 1;
						TransformComponent &ct = e->value<TransformComponent>();
						Entity *bullet = initializeMonster(ct.position + ct.orientation * Vec3(0, 0, -ct.scale - 1), Vec3(0.304, 0.067, 0.294), 2.0, HashString("degrid/boss/cannoneer.obj?ball"), 0, 5, 10);
						bullet->value<TransformComponent>().orientation = randomDirectionQuat();
						bullet->value<VelocityComponent>().velocity = ct.orientation * Vec3(0, 0, -1.0);
						bullet->value<TimeoutComponent>().ttl = ShotsTtl;
					}
				}
			}
			else
				e->add(entitiesToDestroy);
		}, engineEntities(), false);
	}

	void engineUpdateLate()
	{
		if (game.paused)
			return;

		EntityManager *ents = engineEntities();

		entitiesVisitor([&](const BodyComponent &b, const TransformComponent &bt) {
			if (ents->has(b.shieldEntity))
			{
				Entity *sh = ents->get(b.shieldEntity);
				TransformComponent &st = sh->value<TransformComponent>();
				st.position = bt.position;
			}
			for (auto it : enumerate(b.bulbs))
			{
				if (ents->has(*it))
				{
					TransformComponent &t = ents->get(*it)->value<TransformComponent>();
					uint32 octet = numeric_cast<uint32>(it.index) / 3;
					sint32 sub = (numeric_cast<sint32>(it.index) % 3) - 1;
					Quat o = Quat(Rads(), Degs(octet * 45 + 22.5), Rads());
					Vec3 p = o * Vec3(0.8, 0.34, sub * -0.15) * bt.scale;
					t.position = bt.position + bt.orientation * p;
					t.scale = 0.2;
				}
			}
		}, engineEntities(), false);

		entitiesVisitor([&](Entity *e, const CannonComponent &c) {
			if (ents->has(c.bodyEntity))
			{
				const TransformComponent &bt = ents->get(c.bodyEntity)->value<TransformComponent>();
				TransformComponent &ct = e->value<TransformComponent>();
				Quat q = bt.orientation * Quat(Degs(), Degs(c.index * 45 + 22.5), Degs());
				Vec3 tp = normalize(game.monstersTarget - ct.position);
				Vec3 tc = q * Vec3(0, 0, -1);
				Vec3 ts = q * Vec3(1, 0, 0);
				ct.orientation = dot(tp, tc) >= 0 ? Quat(tp, Vec3(0, 1, 0)) : q * Quat(Degs(), Degs(-90 * sign(dot(tp, ts))), Degs());
				ct.orientation = interpolate(q, ct.orientation, min(c.extension, 1));
				ct.orientation = ct.orientation * Quat(Degs(), c.firingOffset, Degs());
				ct.position = bt.position
					+ tc * interpolate(bt.scale * 0.5, bt.scale, smoothstep(clamp(c.extension, 0, 1)))
					+ (ct.orientation * Vec3(0, 0, -1)) * ((clamp(c.loading, 0, 1) - 0.5) * ct.scale * 0.5);
			}
			else
				e->add(entitiesToDestroy);
		}, engineEntities(), false);
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener1;
		EventListener<void()> engineUpdateListener2;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener1.attach(controlThread().update);
			engineUpdateListener1.bind<&engineUpdate>();
			engineUpdateListener2.attach(controlThread().update, 43); // after physics
			engineUpdateListener2.bind<&engineUpdateLate>();
		}
	} callbacksInstance;

	void spawnCannon(Entity *body, uint32 index)
	{
		const TransformComponent &bt = body->value<TransformComponent>();
		Entity *cannon = initializeMonster(bt.position, Vec3(0.188, 0.08, 0.076), 5, HashString("degrid/boss/cannoneerCannon.object"), HashString("degrid/monster/boss/cannoneer-cannon-bum.ogg"), 30, Real::Infinity());
		CannonComponent &c = cannon->value<CannonComponent>();
		c.bodyEntity = body->name();
		c.index = index;
		c.loading = randomRange(-0.2, 0.2);
		c.firingOffset = Degs(randomRange(-5.f, 5.f));
		body->value<BodyComponent>().cannonsSpawned++;
	}
}

void spawnBossCannoneer(const Vec3 &spawnPosition, const Vec3 &color)
{
	Entity *body = initializeMonster(spawnPosition, Vec3(0.487, 0.146, 0.05), 15, HashString("degrid/boss/cannoneerBody.object"), HashString("degrid/monster/boss/cannoneer-bum.ogg"), Real::Infinity(), Real::Infinity());
	const TransformComponent &bt = body->value<TransformComponent>();
	BodyComponent &b = body->value<BodyComponent>();
	{ // body
		body->value<BossComponent>();
		body->value<RotationComponent>().rotation = Quat(Degs(), Degs(0.55), Degs());
		SimpleMonsterComponent &simple = body->value<SimpleMonsterComponent>();
		simple.maxSpeed = 0.25;
		simple.acceleration = 0.2;
		simple.circling = 0.7;
		simple.spiraling = 0.3;
	}
	{ // light bulbs
		for (uint32 &it : b.bulbs)
		{
			Entity *e = engineEntities()->createUnique();
			it = e->name();
			RenderComponent &r = e->value<RenderComponent>();
			r.object = HashString("degrid/boss/cannoneerBulb.object");
			r.color = Vec3(0.022, 0.428, 0.025);
			LightComponent &l = e->value<LightComponent>();
			l.color = r.color;
			l.intensity = 50;
			l.lightType = LightTypeEnum::Point;
			l.attenuation = Vec3(1, 0, 1);
		}
	}
	{ // shield
		Entity *shield = initializeMonster(bt.position, color, bt.scale, HashString("degrid/boss/cannoneer.obj?shield"), 0, Real::Infinity(), Real::Infinity());
		b.shieldEntity = shield->name();
		shield->value<TransformComponent>().orientation = randomDirectionQuat();
		TextureAnimationComponent &aniTex = shield->value<TextureAnimationComponent>();
		aniTex.speed = 0.15;
		aniTex.offset = randomChance();
		SoundComponent &sound = shield->value<SoundComponent>();
		sound.name = HashString("degrid/player/shield.ogg");
		sound.startTime = randomRange(0, 100000000);
	}
	for (uint32 i = 0; i < 8; i++)
		spawnCannon(body, i);
}
