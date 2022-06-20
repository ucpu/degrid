#include <cage-core/geometry.h>
#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/entitiesVisitor.h>
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
		if (game.fireDirection == Vec3())
			return;

		game.shootingCooldown += Real(4) * pow(1.3, game.powerups[(uint32)PowerupTypeEnum::Multishot]) * pow(0.7, game.powerups[(uint32)PowerupTypeEnum::FiringSpeed]);

		const TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
		const VelocityComponent &playerVelocity = game.playerEntity->value<VelocityComponent>();

		for (Real i = game.powerups[(uint32)PowerupTypeEnum::Multishot] * -0.5; i < game.powerups[(uint32)PowerupTypeEnum::Multishot] * 0.5 + 1e-5; i += 1)
		{
			statistics.shotsFired++;
			Entity *shot = engineEntities()->createUnique();
			TransformComponent &transform = shot->value<TransformComponent>();
			Rads dir = atan2(-game.fireDirection[2], -game.fireDirection[0]);
			dir += Degs(i * 10);
			Vec3 dirv = Vec3(-sin(dir), 0, -cos(dir));
			transform.position = playerTransform.position + dirv * 2;
			transform.orientation = Quat(Degs(), dir, Degs());
			RenderComponent &render = shot->value<RenderComponent>();
			render.object = HashString("degrid/player/shot.object");
			if (game.powerups[(uint32)PowerupTypeEnum::SuperDamage] > 0)
				render.color = colorHsvToRgb(Vec3(randomChance(), 1, 1));
			else
				render.color = game.shotsColor;
			shot->value<VelocityComponent>().velocity = dirv * (game.powerups[(uint32)PowerupTypeEnum::ShotsSpeed] + 1.5) + playerVelocity.velocity * 0.3;
			ShotComponent &sh = shot->value<ShotComponent>();
			sh.damage = game.powerups[(uint32)PowerupTypeEnum::ShotsDamage] + (game.powerups[(uint32)PowerupTypeEnum::SuperDamage] ? 4 : 1);
			sh.homing = game.powerups[(uint32)PowerupTypeEnum::HomingShots] > 0;
			shot->value<TimeoutComponent>().ttl = ShotsTtl;
		}
	}

	void shotsUpdate()
	{
		entitiesVisitor([&](Entity *e, TransformComponent &tr, ShotComponent &sh, VelocityComponent &vl) {
			const uint32 myName = e->name();
			uint32 closestMonster = 0;
			uint32 homingMonster = 0;
			Real closestDistance = Real::Infinity();
			Real homingDistance = Real::Infinity();

			spatialSearchQuery->intersection(Sphere(tr.position, length(vl.velocity) + tr.scale + (sh.homing ? 20 : 10)));
			for (uint32 otherName : spatialSearchQuery->result())
			{
				if (otherName == myName)
					continue;

				Entity *e = engineEntities()->get(otherName);
				const TransformComponent &ot = e->value<TransformComponent>();
				const Vec3 toOther = ot.position - tr.position;
				if (e->has<GridComponent>())
				{
					e->value<VelocityComponent>().velocity += normalize(vl.velocity) * (0.2f / max(1, length(toOther)));
					continue;
				}
				if (!e->has<MonsterComponent>() || e->value<MonsterComponent>().life <= 0)
					continue;
				const Real dist = length(toOther);
				if (dist < closestDistance)
				{
					if (collisionTest(tr.position, tr.scale, vl.velocity, ot.position, ot.scale, e->value<VelocityComponent>().velocity))
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
				MonsterComponent &om = m->value<MonsterComponent>();
				const Real dmg = sh.damage;
				sh.damage -= om.life;
				om.life -= dmg;
				if (om.life <= 1e-5)
				{
					statistics.shotsKill++;
					if (killMonster(m, true))
					{
						const Real r = randomChance();
						if (r < game.powerupSpawnChance)
						{
							game.powerupSpawnChance -= 1;
							powerupSpawn(m->value<TransformComponent>().position);
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
					const Vec3 toOther = normalize(m->value<TransformComponent>().position - tr.position);
					const Real spd = length(vl.velocity);
					vl.velocity = toOther * spd;
					tr.orientation = Quat(Degs(), atan2(-toOther[2], -toOther[0]), Degs());
				}
				else
				{
					// homing missiles are shivering
					tr.position += normalize(vl.velocity) * Quat(Degs(), Degs(90), Degs()) * sin(Rads::Full() * statistics.updateIteration / 10 + Degs(hash(myName) % 360)) * (length(vl.velocity) * 0.3);
				}
			}

			vl.velocity[1] = 0;
			tr.position[1] = 0;
		}, engineEntities(), false);
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
		game.shotsColor = game.cinematic ? colorHsvToRgb(Vec3(randomChance(), 1, 1)) : Vec3((float)confPlayerShotColorR, (float)confPlayerShotColorG, (float)confPlayerShotColorB);
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
		EventListener<void()> gameStartListener;
	public:
		Callbacks()
		{
			engineUpdateListener.attach(controlThread().update, -10);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), -10);
			gameStartListener.bind<&gameStart>();
		}
	} callbacksInstance;
}
