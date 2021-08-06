#include <cage-core/enumerate.h>

#include "monsters.h"

namespace
{
	struct BodyComponent
	{
		static EntityComponent *component;

		uint32 bulbs[24] = {};
		uint32 shieldEntity = 0;
		uint32 lastHit = 0;
		uint8 cannonsSpawned = 0;
		uint8 cannonsKilled = 0;
	};

	struct CannonComponent
	{
		static EntityComponent *component;

		uint32 bodyEntity = 0;
		uint32 index = 0; // 0 .. 7 (inclusive)
		real extension; // 0 = fully retracted, 1 = fully exposed
		real loading; // 1 = fully loaded
		rads firingOffset;
	};

	EntityComponent *BodyComponent::component;
	EntityComponent *CannonComponent::component;

	void spawnCannon(Entity *body, uint32 index);

	void bodyEliminated(Entity *e)
	{
		BodyComponent &b = e->value<BodyComponent>();
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
		CannonComponent &c = e->value<CannonComponent>();
		if (engineEntities()->has(c.bodyEntity))
		{
			Entity *body = engineEntities()->get(c.bodyEntity);
			BodyComponent &b = body->value<BodyComponent>();
			if (engineEntities()->has(b.bulbs[b.cannonsKilled]))
			{
				Entity *e = engineEntities()->get(b.bulbs[b.cannonsKilled]);
				RenderComponent &r = e->value<RenderComponent>();
				r.color = vec3(r.color[1], r.color[0], r.color[2]);
				LightComponent &l = e->value<LightComponent>();
				l.color = vec3(l.color[1], l.color[0], l.color[2]);
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
		BodyComponent::component = engineEntities()->defineComponent(BodyComponent());
		CannonComponent::component = engineEntities()->defineComponent(CannonComponent());
		bodyEliminatedListener.bind<&bodyEliminated>();
		bodyEliminatedListener.attach(BodyComponent::component->group()->entityRemoved);
		cannonEliminatedListener.bind<&cannonEliminated>();
		cannonEliminatedListener.attach(CannonComponent::component->group()->entityRemoved);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		EntityManager *ents = engineEntities();

		for (Entity *e : BodyComponent::component->entities())
		{
			BodyComponent &b = e->value<BodyComponent>();
			if (ents->has(b.shieldEntity))
			{
				Entity *sh = ents->get(b.shieldEntity);
				TransformComponent &bt = e->value<TransformComponent>();
				TransformComponent &st = sh->value<TransformComponent>();
				real se = clamp(statistics.updateIteration - b.lastHit, 0u, 30u) / 30.0;
				se = sqrt(max(real(), sin(se * rads::Full() * 0.5)));
				st.scale = bt.scale + interpolate(0.1, 20.0, se);
			}
		}

		for (Entity *e : CannonComponent::component->entities())
		{
			CannonComponent &cannon = e->value<CannonComponent>();
			if (ents->has(cannon.bodyEntity))
			{
				MonsterComponent &monster = e->value<MonsterComponent>();
				monster.life = min(monster.life + 0.002, 20);
				Entity *body = ents->get(cannon.bodyEntity);
				cannon.extension += 0.01;
				if (cannon.extension >= 1)
				{
					BodyComponent &b = body->value<BodyComponent>();
					cannon.loading += (pow(b.cannonsKilled / 20.0, 5) + 1.0) / 70.0;
					if (cannon.loading >= 1)
					{
						cannon.loading -= 1;
						TransformComponent &ct = e->value<TransformComponent>();
						Entity *bullet = initializeMonster(ct.position + ct.orientation * vec3(0, 0, -ct.scale - 1), vec3(0.304, 0.067, 0.294), 2.0, HashString("degrid/boss/cannoneer.obj?ball"), 0, 5, 10);
						TransformComponent &bt = bullet->value<TransformComponent>();
						bt.orientation = randomDirectionQuat();
						VelocityComponent &vel = bullet->value<VelocityComponent>();
						vel.velocity = ct.orientation * vec3(0, 0, -1.0);
						TimeoutComponent &ttl = bullet->value<TimeoutComponent>();
						ttl.ttl = ShotsTtl;
					}
				}
			}
			else
				e->add(entitiesToDestroy);
		}
	}

	void engineUpdateLate()
	{
		if (game.paused)
			return;

		EntityManager *ents = engineEntities();

		for (Entity *e : BodyComponent::component->entities())
		{
			BodyComponent &b = e->value<BodyComponent>();
			TransformComponent &bt = e->value<TransformComponent>();
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
					quat o = quat(rads(), degs(octet * 45 + 22.5), rads());
					vec3 p = o * vec3(0.8, 0.34, sub * -0.15) * bt.scale;
					t.position = bt.position + bt.orientation * p;
					t.scale = 0.2;
				}
			}
		}

		for (Entity *e : CannonComponent::component->entities())
		{
			CannonComponent &c = e->value<CannonComponent>();
			if (ents->has(c.bodyEntity))
			{
				TransformComponent &bt = ents->get(c.bodyEntity)->value<TransformComponent>();
				TransformComponent &ct = e->value<TransformComponent>();
				MonsterComponent &m = e->value<MonsterComponent>();
				quat q = bt.orientation * quat(degs(), degs(c.index * 45 + 22.5), degs());
				vec3 tp = normalize(game.monstersTarget - ct.position);
				vec3 tc = q * vec3(0, 0, -1);
				vec3 ts = q * vec3(1, 0, 0);
				ct.orientation = dot(tp, tc) >= 0 ? quat(tp, vec3(0, 1, 0)) : q * quat(degs(), degs(-90 * sign(dot(tp, ts))), degs());
				ct.orientation = interpolate(q, ct.orientation, min(c.extension, 1));
				ct.orientation = ct.orientation * quat(degs(), c.firingOffset, degs());
				ct.position = bt.position
					+ tc * interpolate(bt.scale * 0.5, bt.scale, smoothstep(clamp(c.extension, 0, 1)))
					+ (ct.orientation * vec3(0, 0, -1)) * ((clamp(c.loading, 0, 1) - 0.5) * ct.scale * 0.5);
			}
			else
				e->add(entitiesToDestroy);
		}
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
		TransformComponent &bt = body->value<TransformComponent>();
		Entity *cannon = initializeMonster(bt.position, vec3(0.188, 0.08, 0.076), 5, HashString("degrid/boss/cannoneerCannon.object"), HashString("degrid/monster/boss/cannoneer-cannon-bum.ogg"), 30, real::Infinity());
		CannonComponent &c = cannon->value<CannonComponent>();
		c.bodyEntity = body->name();
		c.index = index;
		c.loading = randomRange(-0.2, 0.2);
		c.firingOffset = degs(randomRange(-5.f, 5.f));
		BodyComponent &bb = body->value<BodyComponent>();
		bb.cannonsSpawned++;
	}
}

void spawnBossCannoneer(const vec3 &spawnPosition, const vec3 &color)
{
	Entity *body = initializeMonster(spawnPosition, vec3(0.487, 0.146, 0.05), 15, HashString("degrid/boss/cannoneerBody.object"), HashString("degrid/monster/boss/cannoneer-bum.ogg"), real::Infinity(), real::Infinity());
	TransformComponent &bt = body->value<TransformComponent>();
	BodyComponent &b = body->value<BodyComponent>();
	{ // body
		BossComponent &boss = body->value<BossComponent>();
		RotationComponent &rotation = body->value<RotationComponent>();
		rotation.rotation = quat(degs(), degs(0.55), degs());
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
			r.color = vec3(0.022, 0.428, 0.025);
			LightComponent &l = e->value<LightComponent>();
			l.color = r.color;
			l.intensity = 50;
			l.lightType = LightTypeEnum::Point;
			l.attenuation = vec3(1, 0, 1);
		}
	}
	{ // shield
		Entity *shield = initializeMonster(bt.position, color, bt.scale, HashString("degrid/boss/cannoneer.obj?shield"), 0, real::Infinity(), real::Infinity());
		b.shieldEntity = shield->name();
		TransformComponent &t = shield->value<TransformComponent>();
		t.orientation = randomDirectionQuat();
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
