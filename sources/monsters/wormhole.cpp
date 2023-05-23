#include <cage-core/color.h>
#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

#include <vector>

namespace
{
	struct WormholeComponent
	{
		Real maxSpeed;
		Real acceleration;
	};

	struct MonsterFlickeringComponent
	{
		Vec3 baseColorHsv;
		Real flickeringFrequency;
		Real flickeringOffset;
	};

	void countWormholes(uint32 &positive, uint32 &negative)
	{
		positive = negative = 0;
		entitiesVisitor([&](const WormholeComponent &, GravityComponent &g) {
			if (g.strength > 0)
				positive++;
			else if (g.strength < 0)
				negative++;
		}, engineEntities(), false);
	}

	Entity *pickWormhole(sint32 sgn)
	{
		std::vector<Entity*> candidates;
		entitiesVisitor([&](Entity *e, const WormholeComponent &, GravityComponent &g) {
			if (sign(g.strength) == sgn)
				candidates.push_back(e);
		}, engineEntities(), false);
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
			const bool hadFlickering = oe->has<MonsterFlickeringComponent>();
			MonsterFlickeringComponent &mof = oe->value<MonsterFlickeringComponent>();
			if (!hadFlickering)
				mof.baseColorHsv = colorRgbToHsv(oe->value<RenderComponent>().color);
			mof.flickeringFrequency += 1e-6;
			mof.flickeringOffset += randomChance();
		}
	}

	void wormholeKilled(uint32 name)
	{
		Entity *w = engineEntities()->get(name);
		const GravityComponent &wg = w->value<GravityComponent>();

		// kill one pushing wormhole
		if (wg.strength > 0)
		{
			Entity *ow = pickWormhole(-1);
			if (ow)
				killMonster(ow, true);
		}

		// create temporary opposite gravity effect
		Entity *e = engineEntities()->createUnique();
		e->value<TransformComponent>().position = w->value<TransformComponent>().position;
		e->value<GravityComponent>().strength = -wg.strength * 5;
		e->value<TimeoutComponent>().ttl = 3;
	}

	const auto engineInitListener = controlThread().initialize.listen([]() {
		engineEntities()->defineComponent(WormholeComponent());
		engineEntities()->defineComponent(MonsterFlickeringComponent());
	});

	const auto engineUpdateListener = controlThread().update.listen([]() {
		// flickering
		entitiesVisitor([&](RenderComponent &r, const MonsterFlickeringComponent &m) {
			const Real l = (Real)engineControlTime() * m.flickeringFrequency + m.flickeringOffset;
			const Real s = sin(Rads::Full() * l) * 0.5 + 0.5;
			r.color = colorHsvToRgb(Vec3(m.baseColorHsv[0], s, m.baseColorHsv[2]));
		}, engineEntities(), false);

		if (game.paused)
			return;

		uint32 positive, negative;
		countWormholes(positive, negative);
		const TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
		EntityComponent *history = engineEntities()->componentsByType(detail::typeIndex<TransformComponent>())[1];

		entitiesVisitor([&](Entity *e, TransformComponent &t, GravityComponent &g, const WormholeComponent &w, VelocityComponent &v) {
			const uint32 myName = e->name();

			// move the wormhole
			v.velocity += normalize(game.monstersTarget - t.position) * w.acceleration;
			v.velocity = normalize(v.velocity) * min(length(v.velocity), w.maxSpeed);

			if (g.strength > 0)
			{ // this is sucking wormhole
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
						Rads angle = randomAngle();
						Vec3 dir = Vec3(cos(angle), 0, sin(angle));
						Entity *target = pickWormhole(-1);
						if (target)
						{
							TransformComponent &tt = target->value<TransformComponent>();
							ot.position = tt.position + dir * tt.scale;
						}
						else
							ot.position = playerTransform.position + dir * randomRange(200, 250);
						oe->remove(history);
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
		}, engineEntities(), false);
	});
}

void spawnWormhole(const Vec3 &spawnPosition, const Vec3 &color)
{
	uint32 positive, negative;
	countWormholes(positive, negative);
	statistics.wormholesSpawned++;
	uint32 special = 0;
	Entity *wormhole = initializeMonster(spawnPosition, color, 5, HashString("degrid/monster/wormhole.object"), HashString("degrid/monster/bum-wormhole.ogg"), 200, randomRange(200, 300) + 100 * monsterMutation(special));
	wormhole->value<TransformComponent>().orientation = randomDirectionQuat();
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
	wormhole->value<RenderComponent>().color = Vec3(g.strength < 0 ? 1 : 0);
	wormhole->value<TextureAnimationComponent>().speed *= (randomChance() + 0.5) * 0.05 * sign(g.strength);
	wormhole->value<RotationComponent>().rotation = interpolate(Quat(), randomDirectionQuat(), 0.01);
	monsterReflectMutation(wormhole, special);
	soundEffect(HashString("degrid/monster/wormhole.ogg"), spawnPosition);
}

