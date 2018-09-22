#include <vector>

#include "monsters.h"

#include <cage-core/color.h>

namespace
{
	struct wormholeComponent
	{
		static componentClass *component;
		real maxSpeed;
		real acceleration;
	};

	struct monsterFlickeringComponent
	{
		static componentClass *component;
		vec3 baseColorHsv;
		real flickeringFrequency;
		real flickeringOffset;
	};

	componentClass *wormholeComponent::component;
	componentClass *monsterFlickeringComponent::component;

	void countWormholes(uint32 &positive, uint32 &negative)
	{
		positive = negative = 0;
		for (entityClass *e : wormholeComponent::component->getComponentEntities()->entities())
		{
			GRID_GET_COMPONENT(gravity, g, e);
			if (g.strength > 0)
				positive++;
			else if (g.strength < 0)
				negative++;
		}
	}

	entityClass *pickWormhole(sint32 sgn)
	{
		std::vector<entityClass*> candidates;
		for (entityClass *e : wormholeComponent::component->getComponentEntities()->entities())
		{
			GRID_GET_COMPONENT(gravity, g, e);
			if (sign(g.strength) == sgn)
				candidates.push_back(e);
		}
		if (candidates.empty())
			return nullptr;
		return candidates[randomRange(0u, numeric_cast<uint32>(candidates.size()))];
	}

	void updateMonsterFlickering(entityClass *oe)
	{
		GRID_GET_COMPONENT(monster, om, oe);
		if (om.damage < 100)
		{
			om.damage *= 2;
			bool hadFlickering = oe->hasComponent(monsterFlickeringComponent::component);
			GRID_GET_COMPONENT(monsterFlickering, mof, oe);
			if (!hadFlickering)
			{
				ENGINE_GET_COMPONENT(render, render, oe);
				mof.baseColorHsv = convertRgbToHsv(render.color);
			}
			mof.flickeringFrequency += 1e-6;
			mof.flickeringOffset += randomChance();
		}
	}

	void wormholeKilled(uint32 name)
	{
		entityClass *w = entities()->getEntity(name);
		ENGINE_GET_COMPONENT(transform, wt, w);
		GRID_GET_COMPONENT(gravity, wg, w);

		// kill one pushing wormhole
		if (wg.strength > 0)
		{
			entityClass *ow = pickWormhole(-1);
			if (ow)
				killMonster(ow, true);
		}

		// create temporary oposite gravity effect
		entityClass *e = entities()->newUniqueEntity();
		ENGINE_GET_COMPONENT(transform, et, e);
		GRID_GET_COMPONENT(gravity, eg, e);
		et.position = wt.position;
		eg.strength = -wg.strength * 5;
		GRID_GET_COMPONENT(timeout, ttl, e);
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
			for (entityClass *e : monsterFlickeringComponent::component->getComponentEntities()->entities())
			{
				ENGINE_GET_COMPONENT(render, r, e);
				GRID_GET_COMPONENT(monsterFlickering, m, e);
				real l = (real)currentControlTime() * m.flickeringFrequency + m.flickeringOffset;
				real s = sin(rads::Full * l) * 0.5 + 0.5;
				r.color = convertHsvToRgb(vec3(m.baseColorHsv[0], s, m.baseColorHsv[2]));
			}
		}

		if (game.paused)
			return;

		uint32 positive, negative;
		countWormholes(positive, negative);

		for (entityClass *e : wormholeComponent::component->getComponentEntities()->entities())
		{
			uint32 myName = e->getName();
			ENGINE_GET_COMPONENT(transform, t, e);
			GRID_GET_COMPONENT(gravity, g, e);

			// move the wormhole
			{
				GRID_GET_COMPONENT(wormhole, w, e);
				GRID_GET_COMPONENT(velocity, v, e);
				v.velocity += normalize(game.monstersTarget - t.position) * w.acceleration;
				v.velocity = v.velocity.normalize() * min(v.velocity.length(), w.maxSpeed);
			}

			if (g.strength > 0)
			{ // this is sucking wormhole
				ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
				spatialQuery->intersection(sphere(t.position, t.scale + 0.1));
				for (uint32 otherName : spatialQuery->result())
				{
					if (otherName == myName)
						continue;
					entityClass *oe = entities()->getEntity(otherName);

					// gravity irrelevant entities
					if (!oe->hasComponent(velocityComponent::component))
						continue;

					// shots
					if (oe->hasComponent(shotComponent::component))
						continue;

					bool teleport = false;

					// player
					if (oe == game.playerEntity)
					{
						if (game.powerups[(uint32)powerupTypeEnum::Shield] > 0)
						{
							teleport = true;
							statistics.wormholeJumps++;
						}
						else
							continue;
					}

					// grids
					if (oe->hasComponent(gridComponent::component))
						teleport = true;

					// monsters
					if (oe->hasComponent(monsterComponent::component))
					{
						updateMonsterFlickering(oe);
						teleport = true;
					}

					if (teleport)
					{
						ENGINE_GET_COMPONENT(transform, ot, oe);
						transformComponent &oth = oe->value<transformComponent>(transformComponent::componentHistory);
						rads angle = randomAngle();
						vec3 dir = vec3(cos(angle), 0, sin(angle));
						entityClass *target = pickWormhole(-1);
						if (target)
						{
							ENGINE_GET_COMPONENT(transform, tt, target);
							ot.position = tt.position + dir * tt.scale;
						}
						else
							ot.position = playerTransform.position + dir * randomRange(200, 250);
						oth.position = ot.position;
					}
					else
						oe->addGroup(entitiesToDestroy);
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
			t.scale += 0.001;
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
	entityClass *wormhole = initializeMonster(spawnPosition, color, 3 + 0.5 * monsterMutation(special), hashString("grid/monster/wormhole.object"), hashString("grid/monster/bum-wormhole.ogg"), 200, randomRange(200, 300) + 100 * monsterMutation(special));
	ENGINE_GET_COMPONENT(transform, transform, wormhole);
	transform.scale = 3;
	transform.orientation = randomDirectionQuat();
	GRID_GET_COMPONENT(monster, m, wormhole);
	m.dispersion = 0.001;
	m.defeatedCallback.bind<&wormholeKilled>();
	GRID_GET_COMPONENT(wormhole, wh, wormhole);
	wh.maxSpeed = 0.03 + 0.01 * monsterMutation(special);
	wh.acceleration = 0.001;
	GRID_GET_COMPONENT(gravity, g, wormhole);
	g.strength = 10 + 3 * monsterMutation(special);
	if (positive > 0 && (negative == 0 || randomChance() < 0.5))
		g.strength *= -1;
	ENGINE_GET_COMPONENT(animatedTexture, at, wormhole);
	at.speed *= (randomChance() + 0.5) * 0.05 * sign(g.strength);
	transform.orientation = randomDirectionQuat();
	monsterReflectMutation(wormhole, special);
	soundEffect(hashString("grid/monster/wormhole.ogg"), spawnPosition);
}

