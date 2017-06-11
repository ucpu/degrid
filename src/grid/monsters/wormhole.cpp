#include "monsters.h"

namespace grid
{
	namespace
	{
		struct wormholeUpdateStruct
		{
			transformStruct &tr;
			renderStruct &rnd;
			monsterStruct &mo;
			wormholeStruct &wh;
			const uint32 myName;

			wormholeUpdateStruct(entityClass *e) :
				tr(e->value<transformStruct>(transformStruct::component)),
				rnd(e->value<renderStruct>(renderStruct::component)),
				mo(e->value<monsterStruct>(monsterStruct::component)),
				wh(e->value<wormholeStruct>(wormholeStruct::component)),
				myName(e->getName())
			{
				spatialQuery->sphere(tr.position, tr.scale * 30);
				const uint32 *res = spatialQuery->resultArray();
				for (uint32 i = 0, e = spatialQuery->resultCount(); i != e; i++)
					test(res[i]);
				tr.scale += 0.001;
				mo.speed += normalize(player.monstersTarget - tr.position) * wh.acceleration;
				mo.speed = mo.speed.normalize() * min(mo.speed.length(), wh.maxSpeed);
				ENGINE_GET_COMPONENT(animatedTexture, at, e);
				at.animationSpeed = 0;
				at.animationOffset = 1000000 * (19.f / 20) * (1 - mo.life / wh.maxLife);
			}

			const real distance(entityClass *e)
			{
				ENGINE_GET_COMPONENT(transform, tre, e);
				return max(tre.position.distance(tr.position) - tr.scale - tre.scale + 1, 0);
			}

			void pull(entityClass *e)
			{
				ENGINE_GET_COMPONENT(transform, tre, e);
				vec3 dir = normalize(tr.position - tre.position);
				if (e->hasComponent(wormholeStruct::component))
					dir = dir * quat(degs(), degs(80), degs());
				real force = tr.scale / tre.scale / sqrt(max(distance(e), 1));
				tre.position += dir * force;
			}

			void test(uint32 otherName)
			{
				if (otherName == myName || !entities()->hasEntity(otherName))
					return;
				entityClass *e = entities()->getEntity(otherName);

				if (e->hasComponent(monsterStruct::component) || e->hasComponent(gridStruct::component))
				{ // monsters and grids are pulled and teleported
					pull(e);
					if (distance(e) < 1 && !e->hasComponent(snakeTailStruct::component))
					{
						ENGINE_GET_COMPONENT(transform, tre, e);
						if (e->hasComponent(wormholeStruct::component))
						{
							if (tre.scale < tr.scale || (tre.scale == tr.scale && e->getName() < myName))
							{ // absorb smaller wormhole
								real otherVolume = real::Pi * tre.scale * tre.scale;
								real myVolume = real::Pi * tr.scale * tr.scale;
								myVolume += otherVolume;
								tr.position = interpolate(tr.position, tre.position, 0.5 * tre.scale / tr.scale);
								tr.scale = sqrt(myVolume / real::Pi);
								e->addGroup(entitiesToDestroy); // destroy other wormhole
							}
							return; // do not teleport wormholes
						}
						rads angle = randomAngle();
						vec3 dir = vec3(cos(angle), 0, sin(angle));
						tre.position = player.position + dir * random(200, 250);
						transformStruct &treh = e->value<transformStruct>(transformStruct::componentHistory);
						ENGINE_GET_COMPONENT(render, r, e);
						environmentExplosion(treh.position, tre.position - treh.position, r.color, 1, tre.scale); // make an explosion in direction in which it was teleported
						treh.position = tre.position;
						if (e->hasComponent(monsterStruct::component))
						{
							GRID_GET_COMPONENT(monster, moe, e);
							if (moe.damage < 100)
							{
								moe.damage *= 2;
								moe.flickeringSpeed += 1e-6;
								moe.flickeringOffset += random();
							}
						}
					}
					return;
				}

				if (e->hasComponent(effectStruct::component) || e->hasComponent(shotStruct::component) || e == player.playerEntity)
				{ // player, effects and shots are pulled, but are not destroyed by the blackhole
					pull(e);
					return;
				}

				if (e->hasComponent(turretStruct::component) || e->hasComponent(decoyStruct::component) || e->hasComponent(powerupStruct::component))
				{ // powerups are pulled and eventually destroyed
					pull(e);
					if (distance(e) < 1)
						e->addGroup(entitiesToDestroy);
					return;
				}
			}
		};
	}

	void updateWormhole()
	{
		uint32 count = wormholeStruct::component->getComponentEntities()->entitiesCount();
		entityClass *const *monsters = wormholeStruct::component->getComponentEntities()->entitiesArray();
		for (uint32 i = 0; i < count; i++)
		{
			entityClass *e = monsters[i];
			wormholeUpdateStruct u(e);
		}
	}

	void spawnWormhole(const vec3 &spawnPosition, const vec3 &color)
	{
		statistics.wormholesSpawned++;
		uint32 special = 0;
		entityClass *wormhole = initializeMonster(spawnPosition, color, 3 + 0.5 * spawnSpecial(special), hashString("grid/monster/wormhole.object"), hashString("grid/monster/bum-wormhole.ogg"), real::PositiveInfinity, random(40, 60) + 20 * spawnSpecial(special));
		GRID_GET_COMPONENT(monster, m, wormhole);
		m.dispersion = 0.001;
		GRID_GET_COMPONENT(wormhole, wh, wormhole);
		wh.maxSpeed = 0.03 + 0.01 * spawnSpecial(special);
		wh.acceleration = 0.001;
		wh.maxLife = m.life;
		ENGINE_GET_COMPONENT(animatedTexture, at, wormhole);
		at.animationSpeed = 0;
		ENGINE_GET_COMPONENT(transform, transform, wormhole);
		transform.orientation = randomDirectionQuat();
		if (special > 0)
		{
			transform.scale *= 1.2;
			statistics.monstersSpecial++;
		}
		soundEffect(hashString("grid/monster/wormhole.ogg"), spawnPosition);
	}
}