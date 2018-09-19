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
		if (player.shootingCooldown > 0)
		{
			player.shootingCooldown = max(-1, player.shootingCooldown - 1);
			return;
		}
		if (player.fireDirection == vec3())
			return;

		player.shootingCooldown += real(4) * real(1.3).pow(player.powerups[(uint32)powerupTypeEnum::Multishot]) * real(0.7).pow(player.powerups[(uint32)powerupTypeEnum::FiringSpeed]);

		ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, player.playerEntity);

		for (real i = player.powerups[(uint32)powerupTypeEnum::Multishot] * -0.5; i < player.powerups[(uint32)powerupTypeEnum::Multishot] * 0.5 + 1e-5; i += 1)
		{
			statistics.shotsFired++;
			entityClass *shot = entities()->newUniqueEntity();
			ENGINE_GET_COMPONENT(transform, transform, shot);
			rads dir = aTan2(-player.fireDirection[2], -player.fireDirection[0]);
			dir += degs(i * 10);
			transform.orientation = quat(degs(), dir, degs());
			if (player.powerups[(uint32)powerupTypeEnum::SuperDamage] > 0)
				transform.scale *= 1.2;
			ENGINE_GET_COMPONENT(render, render, shot);
			render.object = hashString("grid/player/shot.object");
			render.color = player.shotsColor;
			GRID_GET_COMPONENT(velocity, vel, shot);
			vel.velocity = vec3(-sin(dir), 0, -cos(dir)) * (player.powerups[(uint32)powerupTypeEnum::ShotsSpeed] + 1.5) + playerVelocity.velocity * 0.3;
			GRID_GET_COMPONENT(shot, sh, shot);
			sh.damage = player.powerups[(uint32)powerupTypeEnum::ShotsDamage] + (player.powerups[(uint32)powerupTypeEnum::SuperDamage] ? 4 : 1);
			sh.homing = player.powerups[(uint32)powerupTypeEnum::HomingShots] > 0;
			transform.position = playerTransform.position + vel.velocity.normalize() * playerTransform.scale;
		}
	}

	void shotsUpdate()
	{
		for (entityClass *e : shotComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
			if (tr.position.squaredDistance(playerTransform.position) > 500 * 500)
			{
				e->addGroup(entitiesToDestroy); // destroy itself
				statistics.shotsDissipated++;
				continue;
			}

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
				if (otherName == myName || !entities()->hasEntity(otherName))
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
					ENGINE_GET_COMPONENT(transform, mtr, m);
					m->addGroup(entitiesToDestroy); // destroy the monster
					monsterExplosion(m);
					if (om.destroyedSound)
						soundEffect(om.destroyedSound, mtr.position);
					if (om.shotDownCallback)
					{
						om.shotDownCallback(closestMonster);
						om.shotDownCallback.clear();
					}
					else
					{
						real r = cage::random();
						if (r < player.powerupSpawnChance)
						{
							player.powerupSpawnChance -= 1;
							powerupSpawn(mtr.position);
						}
						player.powerupSpawnChance += interpolate(1.0 / 50, 1.0 / 400, clamp((statistics.powerupsSpawned + 5) / 30.f, 0.f, 1.f));
					}
					player.score += numeric_cast<uint32>(clamp(om.damage, 1, 200));
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
		if (!player.paused)
		{
			shipFiring();
			shotsUpdate();
		}
	}

	void gameStart()
	{
		player.shotsColor = player.cinematic ? convertHsvToRgb(vec3(cage::random(), 1, 1)) : vec3((float)confPlayerShotColorR, (float)confPlayerShotColorG, (float)confPlayerShotColorB);
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
