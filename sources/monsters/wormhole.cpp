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
			DEGRID_COMPONENT(Gravity, g, e);
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
			DEGRID_COMPONENT(Gravity, g, e);
			if (sign(g.strength) == sgn)
				candidates.push_back(e);
		}
		if (candidates.empty())
			return nullptr;
		return candidates[randomRange(0u, numeric_cast<uint32>(candidates.size()))];
	}

	void updateMonsterFlickering(Entity *oe)
	{
		DEGRID_COMPONENT(Monster, om, oe);
		if (om.damage < 100)
		{
			om.damage *= 2;
			bool hadFlickering = oe->has(MonsterFlickeringComponent::component);
			DEGRID_COMPONENT(MonsterFlickering, mof, oe);
			if (!hadFlickering)
			{
				CAGE_COMPONENT_ENGINE(Render, render, oe);
				mof.baseColorHsv = colorRgbToHsv(render.color);
			}
			mof.flickeringFrequency += 1e-6;
			mof.flickeringOffset += randomChance();
		}
	}

	void wormholeKilled(uint32 name)
	{
		Entity *w = engineEntities()->get(name);
		CAGE_COMPONENT_ENGINE(Transform, wt, w);
		DEGRID_COMPONENT(Gravity, wg, w);

		// kill one pushing wormhole
		if (wg.strength > 0)
		{
			Entity *ow = pickWormhole(-1);
			if (ow)
				killMonster(ow, true);
		}

		// create temporary opposite gravity effect
		Entity *e = engineEntities()->createUnique();
		CAGE_COMPONENT_ENGINE(Transform, et, e);
		DEGRID_COMPONENT(Gravity, eg, e);
		et.position = wt.position;
		eg.strength = -wg.strength * 5;
		DEGRID_COMPONENT(Timeout, ttl, e);
		ttl.ttl = 3;
	}

	void engineInit()
	{
		WormholeComponent::component = engineEntities()->defineComponent(WormholeComponent(), true);
		MonsterFlickeringComponent::component = engineEntities()->defineComponent(MonsterFlickeringComponent(), true);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("wormhole");

		{ // flickering
			for (Entity *e : MonsterFlickeringComponent::component->entities())
			{
				CAGE_COMPONENT_ENGINE(Render, r, e);
				DEGRID_COMPONENT(MonsterFlickering, m, e);
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
			CAGE_COMPONENT_ENGINE(Transform, t, e);
			DEGRID_COMPONENT(Gravity, g, e);

			// move the wormhole
			{
				DEGRID_COMPONENT(Wormhole, w, e);
				DEGRID_COMPONENT(Velocity, v, e);
				v.velocity += normalize(game.monstersTarget - t.position) * w.acceleration;
				v.velocity = normalize(v.velocity) * min(length(v.velocity), w.maxSpeed);
			}

			if (g.strength > 0)
			{ // this is sucking wormhole
				CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
				spatialSearchQuery->intersection(sphere(t.position, t.scale + 0.1));
				for (uint32 otherName : spatialSearchQuery->result())
				{
					if (otherName == myName)
						continue;
					Entity *oe = engineEntities()->get(otherName);

					// gravity irrelevant entities
					if (!oe->has(VelocityComponent::component))
						continue;

					// shots
					if (oe->has(ShotComponent::component))
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
					if (oe->has(GridComponent::component))
						teleport = true;

					// monsters
					if (oe->has(MonsterComponent::component))
					{
						updateMonsterFlickering(oe);
						teleport = true;
					}

					if (teleport)
					{
						CAGE_COMPONENT_ENGINE(Transform, ot, oe);
						rads angle = randomAngle();
						vec3 dir = vec3(cos(angle), 0, sin(angle));
						Entity *target = pickWormhole(-1);
						if (target)
						{
							CAGE_COMPONENT_ENGINE(Transform, tt, target);
							ot.position = tt.position + dir * tt.scale;
						}
						else
							ot.position = playerTransform.position + dir * randomRange(200, 250);
						oe->remove(TransformComponent::componentHistory);
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
	CAGE_COMPONENT_ENGINE(Transform, transform, wormhole);
	transform.orientation = randomDirectionQuat();
	DEGRID_COMPONENT(Monster, m, wormhole);
	m.dispersion = 0.001;
	m.defeatedCallback.bind<&wormholeKilled>();
	DEGRID_COMPONENT(Wormhole, wh, wormhole);
	wh.maxSpeed = 0.03 + 0.01 * monsterMutation(special);
	wh.acceleration = 0.001;
	DEGRID_COMPONENT(Gravity, g, wormhole);
	g.strength = 10 + 3 * monsterMutation(special);
	if (positive > 0 && (negative == 0 || randomChance() < 0.5))
		g.strength *= -1;
	CAGE_COMPONENT_ENGINE(Render, render, wormhole);
	render.color = vec3(g.strength < 0 ? 1 : 0);
	CAGE_COMPONENT_ENGINE(TextureAnimation, at, wormhole);
	at.speed *= (randomChance() + 0.5) * 0.05 * sign(g.strength);
	DEGRID_COMPONENT(Rotation, rotation, wormhole);
	rotation.rotation = interpolate(quat(), randomDirectionQuat(), 0.01);
	monsterReflectMutation(wormhole, special);
	soundEffect(HashString("degrid/monster/wormhole.ogg"), spawnPosition);
}

