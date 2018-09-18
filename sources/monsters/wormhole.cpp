#include "monsters.h"

namespace
{
	struct wormholeUpdateStruct
	{
		transformComponent &tr;
		renderComponent &rnd;
		monsterComponent &mo;
		velocityComponent &vl;
		wormholeComponent &wh;
		const uint32 myName;

		wormholeUpdateStruct(entityClass *e) :
			tr(e->value<transformComponent>(transformComponent::component)),
			rnd(e->value<renderComponent>(renderComponent::component)),
			mo(e->value<monsterComponent>(monsterComponent::component)),
			vl(e->value<velocityComponent>(velocityComponent::component)),
			wh(e->value<wormholeComponent>(wormholeComponent::component)),
			myName(e->getName())
		{
			spatialQuery->intersection(sphere(tr.position, tr.scale * 30));
			for (uint32 otherName : spatialQuery->result())
			{
				if (otherName == myName || !entities()->hasEntity(otherName))
					continue;
				entityClass *e = entities()->getEntity(otherName);

				if (e->hasComponent(monsterComponent::component) || e->hasComponent(gridComponent::component))
				{ // monsters and grids are pulled and teleported
					pull(e);
					if (distance(e) < 1 && !e->hasComponent(snakeTailComponent::component))
					{
						ENGINE_GET_COMPONENT(transform, tre, e);
						if (e->hasComponent(wormholeComponent::component))
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
							continue; // do not teleport wormholes
						}
						rads angle = randomAngle();
						vec3 dir = vec3(cos(angle), 0, sin(angle));
						ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
						tre.position = playerTransform.position + dir * random(200, 250);
						transformComponent &treh = e->value<transformComponent>(transformComponent::componentHistory);
						treh.position = tre.position;
						if (e->hasComponent(monsterComponent::component))
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
					continue;
				}

				if (e->hasComponent(shotComponent::component) || e == player.playerEntity)
				{ // player and shots are pulled, but are not destroyed by the blackhole
					pull(e);
					continue;
				}

				if (e->hasComponent(turretComponent::component) || e->hasComponent(decoyComponent::component) || e->hasComponent(powerupComponent::component))
				{ // powerups are pulled and eventually destroyed
					pull(e);
					if (distance(e) < 1)
						e->addGroup(entitiesToDestroy);
					continue;
				}
			}

			tr.scale += 0.001;
			vl.velocity += normalize(player.monstersTarget - tr.position) * wh.acceleration;
			vl.velocity = vl.velocity.normalize() * min(vl.velocity.length(), wh.maxSpeed);
			ENGINE_GET_COMPONENT(animatedTexture, at, e);
			at.speed = 0;
			at.offset = 1000000 * (19.f / 20) * (1 - mo.life / wh.maxLife);
		}

		const real distance(entityClass *oe)
		{
			ENGINE_GET_COMPONENT(transform, tre, oe);
			return max(tre.position.distance(tr.position) - tr.scale - tre.scale + 1, 0);
		}

		void pull(entityClass *oe)
		{
			ENGINE_GET_COMPONENT(transform, tre, oe);
			vec3 dir = normalize(tr.position - tre.position);
			if (oe->hasComponent(wormholeComponent::component))
				dir = dir * quat(degs(), degs(80), degs());
			real force = tr.scale / tre.scale / sqrt(max(distance(oe), 1));
			tre.position += dir * force;
		}
	};

	void engineUpdate()
	{
		if (player.paused)
			return;
		for (entityClass *e : wormholeComponent::component->getComponentEntities()->entities())
			wormholeUpdateStruct u(e);
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
	public:
		callbacksClass()
		{
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
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
	at.speed = 0;
	ENGINE_GET_COMPONENT(transform, transform, wormhole);
	transform.orientation = randomDirectionQuat();
	if (special > 0)
	{
		transform.scale *= 1.2;
		statistics.monstersSpecial++;
	}
	soundEffect(hashString("grid/monster/wormhole.ogg"), spawnPosition);
}

