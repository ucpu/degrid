#include "monsters.h"

#include <cage-core/color.h>

namespace
{
	void engineUpdate()
	{
		if (game.paused)
			return;

		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, game.playerEntity);
		for (entityClass *e : monsterComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			GRID_GET_COMPONENT(velocity, v, e);
			GRID_GET_COMPONENT(monster, m, e);

			// monster dispersion
			if (m.dispersion > 0)
			{
				uint32 myName = e->name();
				vec3 dispersion;
				spatialQuery->intersection(sphere(t.position, t.scale + 1));
				for (uint32 otherName : spatialQuery->result())
				{
					if (otherName == myName)
						continue;
					entityClass *e = entities()->get(otherName);
					ENGINE_GET_COMPONENT(transform, ot, e);
					vec3 toMonster = t.position - ot.position;
					if (e->has(monsterComponent::component))
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
				if (game.powerups[(uint32)powerupTypeEnum::Shield] > 0 && m.damage < real::PositiveInfinity)
				{
					statistics.shieldStoppedMonsters++;
					statistics.shieldAbsorbedDamage += m.damage;
					vec3 shieldDirection = (t.position - playerTransform.position).normalize();
					vec3 shieldPosition = playerTransform.position + shieldDirection * (playerScale * 1.1);
					environmentExplosion(shieldPosition, shieldDirection * 0.5, vec3(1, 1, 1), min(m.damage, 5) * 2 + 2, 3); // shield sparks
				}
				else
				{
					environmentExplosion(interpolate(t.position, playerTransform.position, 0.5), interpolate(playerVelocity.velocity, v.velocity, 0.5), playerDeathColor, min(m.damage, 5) * 2 + 2, 3); // hull sparks
					statistics.monstersSucceded++;
					game.life -= m.damage;
					if (statistics.monstersFirstHit == 0)
						statistics.monstersFirstHit = statistics.updateIterationIgnorePause;
					statistics.monstersLastHit = statistics.updateIterationIgnorePause;
					if (game.life > 0 && !game.cinematic)
					{
						uint32 sounds[] = {
							hashString("grid/speech/damage/better-to-avoid-next-time.wav"),
							hashString("grid/speech/damage/beware.wav"),
							hashString("grid/speech/damage/critical-damage.wav"),
							hashString("grid/speech/damage/damaged.wav"),
							hashString("grid/speech/damage/dont-do-this.wav"),
							hashString("grid/speech/damage/evasive-maneuvers.wav"),
							hashString("grid/speech/damage/hit.wav"),
							hashString("grid/speech/damage/hull-is-breached.wav"),
							hashString("grid/speech/damage/shields-are-failing.wav"),
							hashString("grid/speech/damage/warning.wav"),
							hashString("grid/speech/damage/we-are-doomed.wav"),
							hashString("grid/speech/damage/we-have-been-hit.wav"),
							0
						};
						soundSpeech(sounds);
					}
				}
				killMonster(e, false);
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

uint32 monsterMutation(uint32 &special)
{
	if (game.cinematic)
		return 0;

	statistics.monstersCurrentMutationIteration++;
	real c = (statistics.monstersCurrentMutationIteration) / 30000;
	c = pow(c, 1.2);
	uint32 m = numeric_cast<uint32>(c);
	real p = pow(c - m, 3);
	uint32 res = randomChance() < p;
	res *= m;
	special += res;
	return res;
}

void monsterReflectMutation(entityClass *e, uint32 special)
{
	if (!special)
		return;
	ENGINE_GET_COMPONENT(transform, transform, e);
	transform.scale *= 1.5;
	statistics.monstersMutated++;
	statistics.monstersMutations += special;
}

entityClass *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life)
{
	statistics.monstersSpawned++;
	entityClass *m = entities()->createUnique();
	ENGINE_GET_COMPONENT(transform, transform, m);
	ENGINE_GET_COMPONENT(render, render, m);
	GRID_GET_COMPONENT(monster, monster, m);
	transform.orientation = quat(degs(), randomAngle(), degs());
	transform.position = spawnPosition;
	transform.position[1] = monster.groundLevel = randomChance() * 2 - 1;
	transform.scale = scale;
	render.object = objectName;
	render.color = colorVariation(color);
	monster.damage = damage;
	monster.life = life;
	monster.defeatedSound = deadSound;
	return m;
}

bool killMonster(entityClass *e, bool allowCallback)
{
	if (e->has(entitiesToDestroy))
		return false;
	e->add(entitiesToDestroy);
	monsterExplosion(e);
	GRID_GET_COMPONENT(monster, m, e);
	m.life = 0;
	game.score += numeric_cast<uint32>(clamp(m.damage, 1, 200));
	if (m.defeatedSound)
	{
		ENGINE_GET_COMPONENT(transform, t, e);
		soundEffect(m.defeatedSound, t.position);
		m.defeatedSound = 0;
	}
	if (!allowCallback)
		return false;
	if (m.defeatedCallback)
	{
		m.defeatedCallback(e->name());
		m.defeatedCallback.clear();
		return false;
	}
	return true;
}
