#include "monsters.h"

namespace grid
{
	namespace
	{
		struct monsterUpdateStruct
		{
			transformComponent &tr;
			vec3 dispersion;
			uint32 myName;

			monsterUpdateStruct(entityClass *e) :
				tr(e->value<transformComponent>(transformComponent::component)),
				myName(e->getName())
			{
				spatialQuery->intersection(sphere(tr.position, tr.scale + 1));
				const uint32 *res = spatialQuery->resultArray();
				for (uint32 i = 0, e = spatialQuery->resultCount(); i != e; i++)
					test(res[i]);
				if (dispersion != vec3())
				{
					GRID_GET_COMPONENT(monster, m, e);
					m.speed += dispersion.normalize() * m.dispersion;
				}
			}

			void test(uint32 otherName)
			{
				if (otherName == myName || !entities()->hasEntity(otherName))
					return;
				entityClass *e = entities()->getEntity(otherName);
				ENGINE_GET_COMPONENT(transform, ot, e);
				vec3 toMonster = tr.position - ot.position;
				if (e->hasComponent(monsterStruct::component))
				{
					real d = ot.scale + tr.scale;
					if (toMonster.squaredLength() < d*d)
					{
						GRID_GET_COMPONENT(monster, om, e);
                        (void)om;
						dispersion += toMonster.normalize() / toMonster.length();
					}
				}
			}
		};

		void updateGeneralMonsters()
		{
			{ // flickering
				uint32 count = monsterStruct::component->getComponentEntities()->entitiesCount();
				entityClass *const *monsters = monsterStruct::component->getComponentEntities()->entitiesArray();
				for (uint32 i = 0; i < count; i++)
				{
					entityClass *e = monsters[i];
					GRID_GET_COMPONENT(monster, m, e);
					if (m.flickeringSpeed == 0)
						continue;
					ENGINE_GET_COMPONENT(render, r, e);
					real l = (real)player.updateTime * m.flickeringSpeed + m.flickeringOffset;
					real s = sin(rads::Full * l) * 0.5 + 0.5;
					r.color = convertHsvToRgb(vec3(m.baseColorHsv[0], s, m.baseColorHsv[2]));
				}
			}

			if (!player.paused)
			{
				uint32 count = monsterStruct::component->getComponentEntities()->entitiesCount();
				entityClass *const *monsters = monsterStruct::component->getComponentEntities()->entitiesArray();
				for (uint32 i = 0; i < count; i++)
				{
					entityClass *e = monsters[i];
					ENGINE_GET_COMPONENT(transform, t, e);
					GRID_GET_COMPONENT(monster, m, e);
					ENGINE_GET_COMPONENT(render, r, e);
                    (void)r;
					if (m.dispersion > 0)
					{
						monsterUpdateStruct mu(e);
					}
					if (collisionTest(player.position, player.scale, player.speed, t.position, t.scale, m.speed))
					{
						if (player.powerups[puShield] > 0 && m.damage < real::PositiveInfinity)
						{
							statistics.shieldStoppedMonsters++;
							statistics.shieldAbsorbedDamage += m.damage;
							vec3 shieldDirection = (t.position - player.position).normalize();
							vec3 shieldPosition = player.position + shieldDirection * (player.scale * 1.1);
							environmentExplosion(shieldPosition, shieldDirection * 0.5, vec3(1, 1, 1), min(m.damage, 5) * 2 + 2, 3); // shield sparks
						}
						else
						{
							environmentExplosion(interpolate(t.position, player.position, 0.5), interpolate(player.speed, m.speed, 0.5), player.deathColor, min(m.damage, 5) * 2 + 2, 3); // hull sparks
							statistics.monstersSucceded++;
							player.life -= m.damage;
							if (statistics.monstersFirstHit == 0)
								statistics.monstersFirstHit = statistics.updateIterationNoPause;
							statistics.monstersLastHit = statistics.updateIterationNoPause;
							if (player.life > 0 && !player.cinematic)
							{
								uint32 sounds[] = {
									hashString("grid/speech/lost/better-to-avoid-next-time.wav"),
									hashString("grid/speech/lost/beware.wav"),
									hashString("grid/speech/lost/critical-damage.wav"),
									hashString("grid/speech/lost/damaged.wav"),
									hashString("grid/speech/lost/dont-do-this.wav"),
									hashString("grid/speech/lost/evasive-maneuvers.wav"),
									hashString("grid/speech/lost/hit.wav"),
									hashString("grid/speech/lost/hull-is-breached.wav"),
									hashString("grid/speech/lost/shields-are-failing.wav"),
									hashString("grid/speech/lost/warning.wav"),
									hashString("grid/speech/lost/we-are-doomed.wav"),
									hashString("grid/speech/lost/we-have-been-hit.wav"),
									0 };
								soundSpeech(sounds);
							}
						}
						monsterExplosion(e);
						e->addGroup(entitiesToDestroy);
						if (m.destroyedSound)
							soundEffect(m.destroyedSound, t.position);
						continue;
					}
					m.speed[1] = 0;
					t.position += m.speed;
					t.position[1] = m.groundLevel;
				}
			}
		}
	}

	void monstersUpdate()
	{
		if (!player.paused)
		{
			updateSimple();
			updateSnake();
			updateWormhole();
			updateShielder();
			spawnUpdate();
		}
		updateGeneralMonsters();
	}

	void monstersInit()
	{
		spawnInit();
	}

	void monstersDone()
	{
		spawnDone();
	}

	const uint32 spawnSpecial(uint32 &special)
	{
		if (player.cinematic)
			return 0;
		static const uint32 treshold = 33333;
		uint32 mul = player.score / treshold;
		real off = (real)player.score / (real)treshold - mul;
		real prob = pow(off, 3) * 0.5;
		uint32 res = mul * (random() < prob ? 1 : 0);
		if (res) res = numeric_cast<uint32>(sqrt(res));
		special = max(special, res);
		//CAGE_LOG(severityEnum::Info, "special", string() + "score:\t" + player.score + ", mul:\t" + mul + ", off:\t" + off + ", prob:\t" + prob + ", res:\t" + res);
		return res;
	}

	entityClass *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life)
	{
		statistics.monstersSpawned++;
		entityClass *m = entities()->newEntity(entities()->generateUniqueName());
		ENGINE_GET_COMPONENT(transform, transform, m);
		ENGINE_GET_COMPONENT(render, render, m);
		GRID_GET_COMPONENT(monster, monster, m);
		transform.orientation = quat(degs(), randomAngle(), degs());
		transform.position = spawnPosition;
		transform.position[1] = monster.groundLevel = random() * 2 - 1;
		transform.scale = scale;
		render.object = objectName;
		render.color = colorVariation(color);
		monster.baseColorHsv = convertRgbToHsv(render.color);
		monster.damage = damage;
		monster.life = life;
		monster.destroyedSound = deadSound;
		return m;
	}
}
