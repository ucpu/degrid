#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/color.h>
#include <cage-core/hashString.h>

#include "../game.h"

extern ConfigFloat confPlayerShotColorR;
extern ConfigFloat confPlayerShotColorG;
extern ConfigFloat confPlayerShotColorB;

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

		game.shootingCooldown += real(4) * pow(1.3, game.powerups[(uint32)PowerupTypeEnum::Multishot]) * pow(0.7, game.powerups[(uint32)PowerupTypeEnum::FiringSpeed]);

		CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
		DEGRID_COMPONENT(Velocity, playerVelocity, game.playerEntity);

		for (real i = game.powerups[(uint32)PowerupTypeEnum::Multishot] * -0.5; i < game.powerups[(uint32)PowerupTypeEnum::Multishot] * 0.5 + 1e-5; i += 1)
		{
			statistics.shotsFired++;
			Entity *shot = engineEntities()->createUnique();
			CAGE_COMPONENT_ENGINE(Transform, transform, shot);
			rads dir = atan2(-game.fireDirection[2], -game.fireDirection[0]);
			dir += degs(i * 10);
			vec3 dirv = vec3(-sin(dir), 0, -cos(dir));
			transform.position = playerTransform.position + dirv * 2;
			transform.orientation = quat(degs(), dir, degs());
			CAGE_COMPONENT_ENGINE(Render, render, shot);
			render.object = HashString("degrid/player/shot.object");
			if (game.powerups[(uint32)PowerupTypeEnum::SuperDamage] > 0)
				render.color = colorHsvToRgb(vec3(randomChance(), 1, 1));
			else
				render.color = game.shotsColor;
			DEGRID_COMPONENT(Velocity, vel, shot);
			vel.velocity = dirv * (game.powerups[(uint32)PowerupTypeEnum::ShotsSpeed] + 1.5) + playerVelocity.velocity * 0.3;
			DEGRID_COMPONENT(Shot, sh, shot);
			sh.damage = game.powerups[(uint32)PowerupTypeEnum::ShotsDamage] + (game.powerups[(uint32)PowerupTypeEnum::SuperDamage] ? 4 : 1);
			sh.homing = game.powerups[(uint32)PowerupTypeEnum::HomingShots] > 0;
			DEGRID_COMPONENT(Timeout, ttl, shot);
			ttl.ttl = ShotsTtl;
		}
	}

	void shotsUpdate()
	{
		for (Entity *e : ShotComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);

			uint32 closestMonster = 0;
			uint32 homingMonster = 0;
			real closestDistance = real::Infinity();
			real homingDistance = real::Infinity();
			uint32 myName = e->name();
			DEGRID_COMPONENT(Shot, sh, e);
			DEGRID_COMPONENT(Velocity, vl, e);

			spatialSearchQuery->intersection(sphere(tr.position, length(vl.velocity) + tr.scale + (sh.homing ? 20 : 10)));
			for (uint32 otherName : spatialSearchQuery->result())
			{
				if (otherName == myName)
					continue;

				Entity *e = engineEntities()->get(otherName);
				CAGE_COMPONENT_ENGINE(Transform, ot, e);
				vec3 toOther = ot.position - tr.position;
				if (e->has(GridComponent::component))
				{
					DEGRID_COMPONENT(Velocity, og, e);
					og.velocity += normalize(vl.velocity) * (0.2f / max(1, length(toOther)));
					continue;
				}
				if (!e->has(MonsterComponent::component))
					continue;
				DEGRID_COMPONENT(Monster, om, e);
				if (om.life <= 0)
					continue;
				real dist = length(toOther);
				if (dist < closestDistance)
				{
					DEGRID_COMPONENT(Velocity, ov, e);
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
				Entity *m = engineEntities()->get(closestMonster);
				DEGRID_COMPONENT(Monster, om, m);
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
							CAGE_COMPONENT_ENGINE(Transform, mtr, m);
							powerupSpawn(mtr.position);
						}
						game.powerupSpawnChance += 0.01;
					}
				}
				if (sh.damage <= 1e-5)
				{
					shotExplosion(e);
					e->add(entitiesToDestroy); // destroy the shot
					return;
				}
				vl.velocity += randomDirection3() * 0.5;
				sh.homing = false;
			}
			else if (sh.homing)
			{
				if (homingMonster)
				{
					Entity *m = engineEntities()->get(homingMonster);
					CAGE_COMPONENT_ENGINE(Transform, mtr, m);
					vec3 toOther = normalize(mtr.position - tr.position);
					real spd = length(vl.velocity);
					vl.velocity = toOther * spd;
					tr.orientation = quat(degs(), atan2(-toOther[2], -toOther[0]), degs());
				}
				else
				{
					// homing missiles are shivering
					tr.position += normalize(vl.velocity) * quat(degs(), degs(90), degs()) * sin(rads::Full() * statistics.updateIteration / 10 + degs(hash(myName) % 360)) * (length(vl.velocity) * 0.3);
				}
			}

			vl.velocity[1] = 0;
			tr.position[1] = 0;
		}
	}

	void engineUpdate()
	{
		OPTICK_EVENT("shots");

		if (!game.paused)
		{
			shipFiring();
			shotsUpdate();
		}
	}

	void gameStart()
	{
		game.shotsColor = game.cinematic ? colorHsvToRgb(vec3(randomChance(), 1, 1)) : vec3((float)confPlayerShotColorR, (float)confPlayerShotColorG, (float)confPlayerShotColorB);
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
		EventListener<void()> gameStartListener;
	public:
		Callbacks() : engineUpdateListener("shots"), gameStartListener("shots")
		{
			engineUpdateListener.attach(controlThread().update, -10);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), -10);
			gameStartListener.bind<&gameStart>();
		}
	} callbacksInstance;
}
