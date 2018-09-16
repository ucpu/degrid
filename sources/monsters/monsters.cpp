#include "monsters.h"

#include <cage-core/utility/color.h>

namespace
{
	void engineUpdate()
	{
		{ // flickering
			for (entityClass *e : monsterComponent::component->getComponentEntities()->entities())
			{
				GRID_GET_COMPONENT(monster, m, e);
				if (m.flickeringSpeed == 0)
					continue;
				ENGINE_GET_COMPONENT(render, r, e);
				real l = (real)currentControlTime() * m.flickeringSpeed + m.flickeringOffset;
				real s = sin(rads::Full * l) * 0.5 + 0.5;
				r.color = convertHsvToRgb(vec3(m.baseColorHsv[0], s, m.baseColorHsv[2]));
			}
		}

		if (player.paused)
			return;

		ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, player.playerEntity);
		for (entityClass *e : monsterComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			GRID_GET_COMPONENT(velocity, v, e);
			GRID_GET_COMPONENT(monster, m, e);

			// monster dispersion
			if (m.dispersion > 0)
			{
				uint32 myName = e->getName();
				vec3 dispersion;
				spatialQuery->intersection(sphere(t.position, t.scale + 1));
				for (uint32 otherName : spatialQuery->result())
				{
					if (otherName == myName || !entities()->hasEntity(otherName))
						continue;
					entityClass *e = entities()->getEntity(otherName);
					ENGINE_GET_COMPONENT(transform, ot, e);
					vec3 toMonster = t.position - ot.position;
					if (e->hasComponent(monsterComponent::component))
					{
						real d = ot.scale + t.scale;
						if (toMonster.squaredLength() < d*d)
							dispersion += toMonster.normalize() / toMonster.length();
					}
				}
				if (dispersion != vec3())
					v.velocity += dispersion.normalize() * m.dispersion;
			}

			// collision with player
			if (collisionTest(playerTransform.position, playerScale, playerVelocity.velocity, t.position, t.scale, v.velocity))
			{
				if (player.powerups[(uint32)powerupTypeEnum::Shield] > 0 && m.damage < real::PositiveInfinity)
				{
					statistics.shieldStoppedMonsters++;
					statistics.shieldAbsorbedDamage += m.damage;
					vec3 shieldDirection = (t.position - playerTransform.position).normalize();
					vec3 shieldPosition = playerTransform.position + shieldDirection * (playerScale * 1.1);
					environmentExplosion(shieldPosition, shieldDirection * 0.5, vec3(1, 1, 1), min(m.damage, 5) * 2 + 2, 3); // shield sparks
					player.score += clamp(numeric_cast<uint32>(m.damage), 1u, 200u);
				}
				else
				{
					environmentExplosion(interpolate(t.position, playerTransform.position, 0.5), interpolate(playerVelocity.velocity, v.velocity, 0.5), playerDeathColor, min(m.damage, 5) * 2 + 2, 3); // hull sparks
					statistics.monstersSucceded++;
					player.life -= m.damage;
					if (statistics.monstersFirstHit == 0)
						statistics.monstersFirstHit = statistics.updateIterationIgnorePause;
					statistics.monstersLastHit = statistics.updateIterationIgnorePause;
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
			}

			// monster vertical movement
			v.velocity[1] = 0;
			t.position[1] = m.groundLevel;
		}
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

uint32 spawnSpecial(uint32 &special)
{
	if (player.cinematic)
		return 0;
	static const uint32 treshold = 33333;
	uint32 mul = player.score / treshold;
	real off = (real)player.score / (real)treshold - mul;
	real prob = pow(off, 3) * 0.5;
	uint32 res = mul * (random() < prob ? 1 : 0);
	if (res)
		res = numeric_cast<uint32>(sqrt(res));
	special = max(special, res);
	//CAGE_LOG(severityEnum::Info, "special", string() + "score:\t" + player.score + ", mul:\t" + mul + ", off:\t" + off + ", prob:\t" + prob + ", res:\t" + res);
	return res;
}

entityClass *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life)
{
	statistics.monstersSpawned++;
	entityClass *m = entities()->newUniqueEntity();
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

