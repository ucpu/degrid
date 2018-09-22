#include "../game.h"

#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/spatial.h>
#include <cage-core/color.h>
#include <cage-core/hashString.h>

extern configFloat confPlayerShotColorR;
extern configFloat confPlayerShotColorG;
extern configFloat confPlayerShotColorB;

namespace cage
{
	namespace detail
	{
		CAGE_API uint32 hash(uint32);
	}
}

namespace
{
	void shipFiring()
	{
		if (game.shootingCooldown > 0)
		{
			game.shootingCooldown = max(-1, game.shootingCooldown - 1);
			return;
		}
		if (game.fireDirection == vec3())
			return;

		game.shootingCooldown += real(4) * real(1.3).pow(game.powerups[(uint32)powerupTypeEnum::Multishot]) * real(0.7).pow(game.powerups[(uint32)powerupTypeEnum::FiringSpeed]);

		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, game.playerEntity);

		for (real i = game.powerups[(uint32)powerupTypeEnum::Multishot] * -0.5; i < game.powerups[(uint32)powerupTypeEnum::Multishot] * 0.5 + 1e-5; i += 1)
		{
			statistics.shotsFired++;
			entityClass *shot = entities()->newUniqueEntity();
			ENGINE_GET_COMPONENT(transform, transform, shot);
			rads dir = aTan2(-game.fireDirection[2], -game.fireDirection[0]);
			dir += degs(i * 10);
			transform.orientation = quat(degs(), dir, degs());
			ENGINE_GET_COMPONENT(render, render, shot);
			render.object = hashString("grid/player/shot.object");
			if (game.powerups[(uint32)powerupTypeEnum::SuperDamage] > 0)
				render.color = convertHsvToRgb(vec3(randomChance(), 1, 1));
			else
				render.color = game.shotsColor;
			GRID_GET_COMPONENT(velocity, vel, shot);
			vel.velocity = vec3(-sin(dir), 0, -cos(dir)) * (game.powerups[(uint32)powerupTypeEnum::ShotsSpeed] + 1.5) + playerVelocity.velocity * 0.3;
			GRID_GET_COMPONENT(shot, sh, shot);
			sh.damage = game.powerups[(uint32)powerupTypeEnum::ShotsDamage] + (game.powerups[(uint32)powerupTypeEnum::SuperDamage] ? 4 : 1);
			sh.homing = game.powerups[(uint32)powerupTypeEnum::HomingShots] > 0;
			transform.position = playerTransform.position + vel.velocity.normalize() * playerTransform.scale;
			GRID_GET_COMPONENT(timeout, ttl, shot);
			ttl.ttl = shotsTtl;
		}
	}

	void shotsUpdate()
	{
		for (entityClass *e : shotComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);

			uint32 closestMonster = 0;
			uint32 homingMonster = 0;
			real closestDistance = real::PositiveInfinity;
			real homingDistance = real::PositiveInfinity;
			uint32 myName = e->getName();
			GRID_GET_COMPONENT(shot, sh, e);
			GRID_GET_COMPONENT(velocity, vl, e);

			spatialQuery->intersection(sphere(tr.position, vl.velocity.length() + tr.scale + (sh.homing ? 20 : 10)));
			for (uint32 otherName : spatialQuery->result())
			{
				if (otherName == myName)
					continue;

				entityClass *e = entities()->getEntity(otherName);
				ENGINE_GET_COMPONENT(transform, ot, e);
				vec3 toOther = ot.position - tr.position;
				if (e->hasComponent(gridComponent::component))
				{
					GRID_GET_COMPONENT(velocity, og, e);
					og.velocity += vl.velocity.normalize() * (0.2f / max(1, toOther.length()));
					continue;
				}
				if (!e->hasComponent(monsterComponent::component))
					continue;
				GRID_GET_COMPONENT(monster, om, e);
				if (om.life <= 0)
					continue;
				real dist = toOther.length();
				if (dist < closestDistance)
				{
					GRID_GET_COMPONENT(velocity, ov, e);
					if (collisionTest(tr.position, tr.scale, vl.velocity, ot.position, ot.scale, ov.velocity))
					{
						closestMonster = otherName;
						closestDistance = dist;
					}
				}
				if (dist < homingDistance)
				{
					homingMonster = otherName;
					homingDistance = dist;
				}
			}

			if (closestMonster)
			{
				statistics.shotsHit++;
				entityClass *m = entities()->getEntity(closestMonster);
				GRID_GET_COMPONENT(monster, om, m);
				real dmg = sh.damage;
				sh.damage -= om.life;
				om.life -= dmg;
				if (om.life <= 1e-5)
				{
					statistics.shotsKill++;
					if (killMonster(m, true))
					{
						real r = randomChance();
						if (r < game.powerupSpawnChance)
						{
							game.powerupSpawnChance -= 1;
							ENGINE_GET_COMPONENT(transform, mtr, m);
							powerupSpawn(mtr.position);
						}
						game.powerupSpawnChance += interpolate(1.0 / 50, 1.0 / 400, clamp((statistics.powerupsSpawned + 5) / 30.f, 0.f, 1.f));
					}
				}
				if (sh.damage <= 1e-5)
				{
					shotExplosion(e);
					e->addGroup(entitiesToDestroy); // destroy the shot
					return;
				}
				vl.velocity += randomDirection3() * 0.5;
				sh.homing = false;
			}
			else if (sh.homing)
			{
				if (homingMonster)
				{
					entityClass *m = entities()->getEntity(homingMonster);
					ENGINE_GET_COMPONENT(transform, mtr, m);
					vec3 toOther = normalize(mtr.position - tr.position);
					real spd = vl.velocity.length();
					vl.velocity = toOther * spd;
					tr.orientation = quat(degs(), aTan2(-toOther[2], -toOther[0]), degs());
				}
				else
				{
					// homing missiles are shivering
					tr.position += vl.velocity.normalize() * quat(degs(), degs(90), degs()) * sin(rads::Full * statistics.updateIterationWithPause / 10 + degs(detail::hash(myName) % 360)) * (vl.velocity.length() * 0.3);
				}
			}

			vl.velocity[1] = 0;
			tr.position[1] = 0;
		}
	}

	void engineUpdate()
	{
		if (!game.paused)
		{
			shipFiring();
			shotsUpdate();
		}
	}

	void gameStart()
	{
		game.shotsColor = game.cinematic ? convertHsvToRgb(vec3(randomChance(), 1, 1)) : vec3((float)confPlayerShotColorR, (float)confPlayerShotColorG, (float)confPlayerShotColorB);
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
	public:
		callbacksClass()
		{
			engineUpdateListener.attach(controlThread().update, -10);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), -10);
			gameStartListener.bind<&gameStart>();
		}
	} callbacksInstance;
}
