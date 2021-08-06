#include <cage-core/color.h>

#include "monsters.h"

#include <vector>

namespace
{
	struct WormholeComponent
	{
		static EntityComponent *component;
		real maxSpeed;
		real acceleration;
	};

	struct MonsterFlickeringComponent
	{
		static EntityComponent *component;
		vec3 baseColorHsv;
		real flickeringFrequency;
		real flickeringOffset;
	};

	EntityComponent *WormholeComponent::component;
	EntityComponent *MonsterFlickeringComponent::component;

	void countWormholes(uint32 &positive, uint32 &negative)
	{
		positive = negative = 0;
		for (Entity *e : WormholeComponent::component->entities())
		{
			GravityComponent &g = e->value<GravityComponent>();
			if (g.strength > 0)
				positive++;
			else if (g.strength < 0)
				negative++;
		}
	}

	Entity *pickWormhole(sint32 sgn)
	{
		std::vector<Entity*> candidates;
		for (Entity *e : WormholeComponent::component->entities())
		{
			GravityComponent &g = e->value<GravityComponent>();
			if (sign(g.strength) == sgn)
				candidates.push_back(e);
		}
		if (candidates.empty())
			return nullptr;
		return candidates[randomRange(0u, numeric_cast<uint32>(candidates.size()))];
	}

	void updateMonsterFlickering(Entity *oe)
	{
		MonsterComponent &om = oe->value<MonsterComponent>();
		if (om.damage < 100)
		{
			om.damage *= 2;
			bool hadFlickering = oe->has(MonsterFlickeringComponent::component);
			MonsterFlickeringComponent &mof = oe->value<MonsterFlickeringComponent>();
			if (!hadFlickering)
			{
				RenderComponent &render = oe->value<RenderComponent>();
				mof.baseColorHsv = colorRgbToHsv(render.color);
			}
			mof.flickeringFrequency += 1e-6;
			mof.flickeringOffset += randomChance();
		}
	}

	void wormholeKilled(uint32 name)
	{
		Entity *w = engineEntities()->get(name);
		TransformComponent &wt = w->value<TransformComponent>();
		GravityComponent &wg = w->value<GravityComponent>();

		// kill one pushing wormhole
		if (wg.strength > 0)
		{
			Entity *ow = pickWormhole(-1);
			if (ow)
				killMonster(ow, true);
		}

		// create temporary opposite gravity effect
		Entity *e = engineEntities()->createUnique();
		TransformComponent &et = e->value<TransformComponent>();
		GravityComponent &eg = e->value<GravityComponent>();
		et.position = wt.position;
		eg.strength = -wg.strength * 5;
		TimeoutComponent &ttl = e->value<TimeoutComponent>();
		ttl.ttl = 3;
	}

	void engineInit()
	{
		WormholeComponent::component = engineEntities()->defineComponent(WormholeComponent());
		MonsterFlickeringComponent::component = engineEntities()->defineComponent(MonsterFlickeringComponent());
	}

	void engineUpdate()
	{
		{ // flickering
			for (Entity *e : MonsterFlickeringComponent::component->entities())
			{
				RenderComponent &r = e->value<RenderComponent>();
				MonsterFlickeringComponent &m = e->value<MonsterFlickeringComponent>();
				real l = (real)engineControlTime() * m.flickeringFrequency + m.flickeringOffset;
				real s = sin(rads::Full() * l) * 0.5 + 0.5;
				r.color = colorHsvToRgb(vec3(m.baseColorHsv[0], s, m.baseColorHsv[2]));
			}
		}

		if (game.paused)
			return;

		uint32 positive, negative;
		countWormholes(positive, negative);

		for (Entity *e : WormholeComponent::component->entities())
		{
			uint32 myName = e->name();
			TransformComponent &t = e->value<TransformComponent>();
			GravityComponent &g = e->value<GravityComponent>();

			// move the wormhole
			{
				WormholeComponent &w = e->value<WormholeComponent>();
				VelocityComponent &v = e->value<VelocityComponent>();
				v.velocity += normalize(game.monstersTarget - t.position) * w.acceleration;
				v.velocity = normalize(v.velocity) * min(length(v.velocity), w.maxSpeed);
			}

			if (g.strength > 0)
			{ // this is sucking wormhole
				TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
				spatialSearchQuery->intersection(Sphere(t.position, t.scale + 0.1));
				for (uint32 otherName : spatialSearchQuery->result())
				{
					if (otherName == myName)
						continue;
					Entity *oe = engineEntities()->get(otherName);

					// gravity irrelevant entities
					if (!oe->has<VelocityComponent>())
						continue;

					// shots
					if (oe->has<ShotComponent>())
						continue;

					bool teleport = false;

					// player
					if (oe == game.playerEntity)
					{
						if (game.powerups[(uint32)PowerupTypeEnum::Shield] > 0)
						{
							teleport = true;
							statistics.wormholeJumps++;
							achievementFullfilled("spacetime-travel");
						}
						else
							continue;
					}

					// grids
					if (oe->has<GridComponent>())
						teleport = true;

					// monsters
					if (oe->has<MonsterComponent>())
					{
						updateMonsterFlickering(oe);
						teleport = true;
					}

					if (teleport)
					{
						TransformComponent &ot = oe->value<TransformComponent>();
						rads angle = randomAngle();
						vec3 dir = vec3(cos(angle), 0, sin(angle));
						Entity *target = pickWormhole(-1);
						if (target)
						{
							TransformComponent &tt = target->value<TransformComponent>();
							ot.position = tt.position + dir * tt.scale;
						}
						else
							ot.position = playerTransform.position + dir * randomRange(200, 250);
						//oe->remove(TransformComponent::componentHistory);
					}
					else
						oe->add(entitiesToDestroy);
				}
			}
			else
			{ // this is pushing wormhole
				if (positive == 0)
				{ // destroy itself (the wormhole has no opposite to feed it)
					killMonster(e, true);
				}
			}

			// empowering the wormhole over time
			g.strength += sign(g.strength) * 0.005;
			t.scale += 0.0005;
		}
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

void spawnWormhole(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 positive, negative;
	countWormholes(positive, negative);
	statistics.wormholesSpawned++;
	uint32 special = 0;
	Entity *wormhole = initializeMonster(spawnPosition, color, 5, HashString("degrid/monster/wormhole.object"), HashString("degrid/monster/bum-wormhole.ogg"), 200, randomRange(200, 300) + 100 * monsterMutation(special));
	TransformComponent &transform = wormhole->value<TransformComponent>();
	transform.orientation = randomDirectionQuat();
	MonsterComponent &m = wormhole->value<MonsterComponent>();
	m.dispersion = 0.001;
	m.defeatedCallback.bind<&wormholeKilled>();
	WormholeComponent &wh = wormhole->value<WormholeComponent>();
	wh.maxSpeed = 0.03 + 0.01 * monsterMutation(special);
	wh.acceleration = 0.001;
	GravityComponent &g = wormhole->value<GravityComponent>();
	g.strength = 10 + 3 * monsterMutation(special);
	if (positive > 0 && (negative == 0 || randomChance() < 0.5))
		g.strength *= -1;
	RenderComponent &render = wormhole->value<RenderComponent>();
	render.color = vec3(g.strength < 0 ? 1 : 0);
	TextureAnimationComponent &at = wormhole->value<TextureAnimationComponent>();
	at.speed *= (randomChance() + 0.5) * 0.05 * sign(g.strength);
	RotationComponent &rotation = wormhole->value<RotationComponent>();
	rotation.rotation = interpolate(quat(), randomDirectionQuat(), 0.01);
	monsterReflectMutation(wormhole, special);
	soundEffect(HashString("degrid/monster/wormhole.ogg"), spawnPosition);
}

