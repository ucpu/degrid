#include "monsters.h"
#include <cage-core/enumerate.h>

namespace
{
	struct bodyComponent
	{
		static entityComponent *component;

		uint32 bulbs[24];
		uint32 shieldEntity;
		uint32 lastHit;
		uint8 cannonsSpawned;
		uint8 cannonsKilled;

		bodyComponent() : shieldEntity(0), lastHit(0), cannonsSpawned(0), cannonsKilled(0)
		{
			for (uint32 &it : bulbs)
				it = 0;
		}
	};

	struct cannonComponent
	{
		static entityComponent *component;

		uint32 bodyEntity;
		uint32 index; // 0 .. 7 (inclusive)
		real extension; // 0 = fully retracted, 1 = fully exposed
		real loading; // 1 = fully loaded
		rads firingOffset;

		cannonComponent() : bodyEntity(0), index(0), extension(0), loading(0), firingOffset(0)
		{}
	};

	entityComponent *bodyComponent::component;
	entityComponent *cannonComponent::component;

	void spawnCannon(entity *body, uint32 index);

	void bodyEliminated(entity *e)
	{
		DEGRID_COMPONENT(body, b, e);
		if (entities()->has(b.shieldEntity))
			entities()->get(b.shieldEntity)->add(entitiesToDestroy);
		for (uint32 it : b.bulbs)
		{
			if (entities()->has(it))
				entities()->get(it)->add(entitiesToDestroy);
		}
	}

	void cannonEliminated(entity *e)
	{
		DEGRID_COMPONENT(cannon, c, e);
		if (entities()->has(c.bodyEntity))
		{
			entity *body = entities()->get(c.bodyEntity);
			DEGRID_COMPONENT(body, b, body);
			if (entities()->has(b.bulbs[b.cannonsKilled]))
			{
				entity *e = entities()->get(b.bulbs[b.cannonsKilled]);
				CAGE_COMPONENT_ENGINE(render, r, e);
				r.color = vec3(r.color[1], r.color[0], r.color[2]);
				CAGE_COMPONENT_ENGINE(light, l, e);
				l.color = vec3(l.color[1], l.color[0], l.color[2]);
			}
			b.cannonsKilled++;
			if (b.cannonsSpawned < 24)
				spawnCannon(body, c.index);
			if (b.cannonsKilled == 24)
			{
				DEGRID_COMPONENT(monster, m, body);
				m.life = min(m.life, 10);
				if (entities()->has(b.shieldEntity))
				{
					entities()->get(b.shieldEntity)->add(entitiesToDestroy);
					b.shieldEntity = 0;
				}
			}
			b.lastHit = statistics.updateIteration;
		}
	}

	eventListener<void(entity*)> bodyEliminatedListener;
	eventListener<void(entity*)> cannonEliminatedListener;

	void engineInit()
	{
		bodyComponent::component = entities()->defineComponent(bodyComponent(), true);
		cannonComponent::component = entities()->defineComponent(cannonComponent(), true);
		bodyEliminatedListener.bind<&bodyEliminated>();
		bodyEliminatedListener.attach(bodyComponent::component->group()->entityRemoved);
		cannonEliminatedListener.bind<&cannonEliminated>();
		cannonEliminatedListener.attach(cannonComponent::component->group()->entityRemoved);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("cannoneer boss");

		if (game.paused)
			return;

		entityManager *ents = entities();

		for (entity *e : bodyComponent::component->entities())
		{
			DEGRID_COMPONENT(body, b, e);
			if (ents->has(b.shieldEntity))
			{
				entity *sh = ents->get(b.shieldEntity);
				CAGE_COMPONENT_ENGINE(transform, bt, e);
				CAGE_COMPONENT_ENGINE(transform, st, sh);
				real se = clamp(statistics.updateIteration - b.lastHit, 0u, 30u) / 30.0;
				se = sqrt(max(0, sin(se * rads::Stright())));
				st.scale = bt.scale + interpolate(0.1, 20.0, se);
			}
		}

		for (entity *e : cannonComponent::component->entities())
		{
			DEGRID_COMPONENT(cannon, cannon, e);
			if (ents->has(cannon.bodyEntity))
			{
				DEGRID_COMPONENT(monster, monster, e);
				monster.life = min(monster.life + 0.01, 20);
				entity *body = ents->get(cannon.bodyEntity);
				cannon.extension += 0.01;
				if (cannon.extension >= 1)
				{
					DEGRID_COMPONENT(body, b, body);
					cannon.loading += (pow(b.cannonsKilled / 20.0, 5) + 1.0) / 70.0;
					if (cannon.loading >= 1)
					{
						cannon.loading -= 1;
						CAGE_COMPONENT_ENGINE(transform, ct, e);
						entity *bullet = initializeMonster(ct.position + ct.orientation * vec3(0, 0, -ct.scale - 1), vec3(0.304, 0.067, 0.294), 2.0, hashString("degrid/boss/cannoneer.obj?ball"), 0, 5, 10);
						CAGE_COMPONENT_ENGINE(transform, bt, bullet);
						bt.orientation = randomDirectionQuat();
						DEGRID_COMPONENT(velocity, vel, bullet);
						vel.velocity = ct.orientation * vec3(0, 0, -1.0);
						DEGRID_COMPONENT(timeout, ttl, bullet);
						ttl.ttl = shotsTtl;
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

		entityManager *ents = entities();

		for (entity *e : bodyComponent::component->entities())
		{
			DEGRID_COMPONENT(body, b, e);
			CAGE_COMPONENT_ENGINE(transform, bt, e);
			if (ents->has(b.shieldEntity))
			{
				entity *sh = ents->get(b.shieldEntity);
				CAGE_COMPONENT_ENGINE(transform, st, sh);
				st.position = bt.position;
			}
			for (auto it : enumerate(b.bulbs))
			{
				if (ents->has(*it))
				{
					CAGE_COMPONENT_ENGINE(transform, t, ents->get(*it));
					uint32 octet = numeric_cast<uint32>(it.cnt) / 3;
					sint32 sub = (numeric_cast<sint32>(it.cnt) % 3) - 1;
					quat o = quat(rads(), degs(octet * 45 + 22.5), rads());
					vec3 p = o * vec3(0.8, 0.34, sub * -0.15) * bt.scale;
					t.position = bt.position + bt.orientation * p;
					t.scale = 0.2;
				}
			}
		}

		for (entity *e : cannonComponent::component->entities())
		{
			DEGRID_COMPONENT(cannon, c, e);
			if (ents->has(c.bodyEntity))
			{
				CAGE_COMPONENT_ENGINE(transform, bt, ents->get(c.bodyEntity));
				CAGE_COMPONENT_ENGINE(transform, ct, e);
				DEGRID_COMPONENT(monster, m, e);
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

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener1;
		eventListener<void()> engineUpdateListener2;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener1.attach(controlThread().update);
			engineUpdateListener1.bind<&engineUpdate>();
			engineUpdateListener2.attach(controlThread().update, 43); // after physics
			engineUpdateListener2.bind<&engineUpdateLate>();
		}
	} callbacksInstance;

	void spawnCannon(entity *body, uint32 index)
	{
		CAGE_COMPONENT_ENGINE(transform, bt, body);
		entity *cannon = initializeMonster(bt.position, vec3(0.188, 0.08, 0.076), 5, hashString("degrid/boss/cannoneerCannon.object"), hashString("degrid/monster/boss/cannoneer-cannon-bum.ogg"), 30, real::Infinity());
		DEGRID_COMPONENT(cannon, c, cannon);
		c.bodyEntity = body->name();
		c.index = index;
		c.loading = randomRange(-0.2, 0.2);
		c.firingOffset = degs(randomRange(-5.f, 5.f));
		DEGRID_COMPONENT(body, bb, body);
		bb.cannonsSpawned++;
	}
}

void spawnBossCannoneer(const vec3 &spawnPosition, const vec3 &color)
{
	entity *body = initializeMonster(spawnPosition, vec3(0.487, 0.146, 0.05), 15, hashString("degrid/boss/cannoneerBody.object"), hashString("degrid/monster/boss/cannoneer-bum.ogg"), real::Infinity(), real::Infinity());
	CAGE_COMPONENT_ENGINE(transform, bt, body);
	DEGRID_COMPONENT(body, b, body);
	{ // body
		DEGRID_COMPONENT(boss, boss, body);
		DEGRID_COMPONENT(rotation, rotation, body);
		rotation.rotation = quat(degs(), degs(0.55), degs());
		DEGRID_COMPONENT(simpleMonster, simple, body);
		simple.maxSpeed = 0.25;
		simple.acceleration = 0.2;
		simple.circling = 0.7;
		simple.spiraling = 0.3;
	}
	{ // light bulbs
		for (uint32 &it : b.bulbs)
		{
			entity *e = entities()->createUnique();
			it = e->name();
			CAGE_COMPONENT_ENGINE(render, r, e);
			r.object = hashString("degrid/boss/cannoneerBulb.object");
			r.color = vec3(0.022, 0.428, 0.025);
			CAGE_COMPONENT_ENGINE(light, l, e);
			l.color = r.color * 50;
			l.lightType = lightTypeEnum::Point;
			l.attenuation = vec3(1, 0, 1);
		}
	}
	{ // shield
		entity *shield = initializeMonster(bt.position, color, bt.scale, hashString("degrid/boss/cannoneer.obj?shield"), 0, real::Infinity(), real::Infinity());
		b.shieldEntity = shield->name();
		CAGE_COMPONENT_ENGINE(transform, t, shield);
		t.orientation = randomDirectionQuat();
		CAGE_COMPONENT_ENGINE(textureAnimation, aniTex, shield);
		aniTex.speed = 0.15;
		aniTex.offset = randomChance();
		CAGE_COMPONENT_ENGINE(voice, sound, shield);
		sound.name = hashString("degrid/player/shield.ogg");
		sound.startTime = randomRange(0, 100000000);
	}
	for (uint32 i = 0; i < 8; i++)
		spawnCannon(body, i);
}
