#include <vector>

#include "monsters.h"

#include <cage-core/color.h>

namespace
{
	struct wormholeComponent
	{
		static entityComponent *component;
		real maxSpeed;
		real acceleration;
	};

	struct monsterFlickeringComponent
	{
		static entityComponent *component;
		vec3 baseColorHsv;
		real flickeringFrequency;
		real flickeringOffset;
	};

	entityComponent *wormholeComponent::component;
	entityComponent *monsterFlickeringComponent::component;

	void countWormholes(uint32 &positive, uint32 &negative)
	{
		positive = negative = 0;
		for (entity *e : wormholeComponent::component->entities())
		{
			DEGRID_COMPONENT(gravity, g, e);
			if (g.strength > 0)
				positive++;
			else if (g.strength < 0)
				negative++;
		}
	}

	entity *pickWormhole(sint32 sgn)
	{
		std::vector<entity*> candidates;
		for (entity *e : wormholeComponent::component->entities())
		{
			DEGRID_COMPONENT(gravity, g, e);
			if (sign(g.strength) == sgn)
				candidates.push_back(e);
		}
		if (candidates.empty())
			return nullptr;
		return candidates[randomRange(0u, numeric_cast<uint32>(candidates.size()))];
	}

	void updateMonsterFlickering(entity *oe)
	{
		DEGRID_COMPONENT(monster, om, oe);
		if (om.damage < 100)
		{
			om.damage *= 2;
			bool hadFlickering = oe->has(monsterFlickeringComponent::component);
			DEGRID_COMPONENT(monsterFlickering, mof, oe);
			if (!hadFlickering)
			{
				CAGE_COMPONENT_ENGINE(render, render, oe);
				mof.baseColorHsv = convertRgbToHsv(render.color);
			}
			mof.flickeringFrequency += 1e-6;
			mof.flickeringOffset += randomChance();
		}
	}

	void wormholeKilled(uint32 name)
	{
		entity *w = entities()->get(name);
		CAGE_COMPONENT_ENGINE(transform, wt, w);
		DEGRID_COMPONENT(gravity, wg, w);

		// kill one pushing wormhole
		if (wg.strength > 0)
		{
			entity *ow = pickWormhole(-1);
			if (ow)
				killMonster(ow, true);
		}

		// create temporary opposite gravity effect
		entity *e = entities()->createUnique();
		CAGE_COMPONENT_ENGINE(transform, et, e);
		DEGRID_COMPONENT(gravity, eg, e);
		et.position = wt.position;
		eg.strength = -wg.strength * 5;
		DEGRID_COMPONENT(timeout, ttl, e);
		ttl.ttl = 3;
	}

	void engineInit()
	{
		wormholeComponent::component = entities()->defineComponent(wormholeComponent(), true);
		monsterFlickeringComponent::component = entities()->defineComponent(monsterFlickeringComponent(), true);
	}

	void engineUpdate()
	{
		{ // flickering
			for (entity *e : monsterFlickeringComponent::component->entities())
			{
				CAGE_COMPONENT_ENGINE(render, r, e);
				DEGRID_COMPONENT(monsterFlickering, m, e);
				real l = (real)currentControlTime() * m.flickeringFrequency + m.flickeringOffset;
				real s = sin(rads::Full() * l) * 0.5 + 0.5;
				r.color = convertHsvToRgb(vec3(m.baseColorHsv[0], s, m.baseColorHsv[2]));
			}
		}

		if (game.paused)
			return;

		uint32 positive, negative;
		countWormholes(positive, negative);

		for (entity *e : wormholeComponent::component->entities())
		{
			uint32 myName = e->name();
			CAGE_COMPONENT_ENGINE(transform, t, e);
			DEGRID_COMPONENT(gravity, g, e);

			// move the wormhole
			{
				DEGRID_COMPONENT(wormhole, w, e);
				DEGRID_COMPONENT(velocity, v, e);
				v.velocity += normalize(game.monstersTarget - t.position) * w.acceleration;
				v.velocity = v.velocity.normalize() * min(v.velocity.length(), w.maxSpeed);
			}

			if (g.strength > 0)
			{ // this is sucking wormhole
				CAGE_COMPONENT_ENGINE(transform, playerTransform, game.playerEntity);
				spatialSearchQuery->intersection(sphere(t.position, t.scale + 0.1));
				for (uint32 otherName : spatialSearchQuery->result())
				{
					if (otherName == myName)
						continue;
					entity *oe = entities()->get(otherName);

					// gravity irrelevant entities
					if (!oe->has(velocityComponent::component))
						continue;

					// shots
					if (oe->has(shotComponent::component))
						continue;

					bool teleport = false;

					// player
					if (oe == game.playerEntity)
					{
						if (game.powerups[(uint32)powerupTypeEnum::Shield] > 0)
						{
							teleport = true;
							statistics.wormholeJumps++;
							achievementFullfilled("spacetime-travel");
						}
						else
							continue;
					}

					// grids
					if (oe->has(gridComponent::component))
						teleport = true;

					// monsters
					if (oe->has(monsterComponent::component))
					{
						updateMonsterFlickering(oe);
						teleport = true;
					}

					if (teleport)
					{
						CAGE_COMPONENT_ENGINE(transform, ot, oe);
						rads angle = randomAngle();
						vec3 dir = vec3(cos(angle), 0, sin(angle));
						entity *target = pickWormhole(-1);
						if (target)
						{
							CAGE_COMPONENT_ENGINE(transform, tt, target);
							ot.position = tt.position + dir * tt.scale;
						}
						else
							ot.position = playerTransform.position + dir * randomRange(200, 250);
						oe->remove(transformComponent::componentHistory);
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

void spawnWormhole(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 positive, negative;
	countWormholes(positive, negative);
	statistics.wormholesSpawned++;
	uint32 special = 0;
	entity *wormhole = initializeMonster(spawnPosition, color, 5, hashString("degrid/monster/wormhole.object"), hashString("degrid/monster/bum-wormhole.ogg"), 200, randomRange(200, 300) + 100 * monsterMutation(special));
	CAGE_COMPONENT_ENGINE(transform, transform, wormhole);
	transform.orientation = randomDirectionQuat();
	DEGRID_COMPONENT(monster, m, wormhole);
	m.dispersion = 0.001;
	m.defeatedCallback.bind<&wormholeKilled>();
	DEGRID_COMPONENT(wormhole, wh, wormhole);
	wh.maxSpeed = 0.03 + 0.01 * monsterMutation(special);
	wh.acceleration = 0.001;
	DEGRID_COMPONENT(gravity, g, wormhole);
	g.strength = 10 + 3 * monsterMutation(special);
	if (positive > 0 && (negative == 0 || randomChance() < 0.5))
		g.strength *= -1;
	CAGE_COMPONENT_ENGINE(textureAnimation, at, wormhole);
	at.speed *= (randomChance() + 0.5) * 0.05 * sign(g.strength);
	DEGRID_COMPONENT(rotation, rotation, wormhole);
	rotation.rotation = interpolate(quat(), randomDirectionQuat(), 0.01);
	monsterReflectMutation(wormhole, special);
	soundEffect(hashString("degrid/monster/wormhole.ogg"), spawnPosition);
}

