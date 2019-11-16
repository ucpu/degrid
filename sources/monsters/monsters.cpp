#include "monsters.h"

#include <cage-core/color.h>

namespace
{
	bool wasBoss = false;

	void engineUpdate()
	{
		OPTICK_EVENT("monsters");

		if (game.paused)
			return;

		CAGE_COMPONENT_ENGINE(transform, playerTransform, game.playerEntity);
		DEGRID_COMPONENT(velocity, playerVelocity, game.playerEntity);

		for (entity *e : monsterComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, t, e);
			DEGRID_COMPONENT(velocity, v, e);
			DEGRID_COMPONENT(monster, m, e);

			// monster dispersion
			if (m.dispersion > 0)
			{
				uint32 myName = e->name();
				vec3 dispersion;
				spatialSearchQuery->intersection(sphere(t.position, t.scale + 1));
				for (uint32 otherName : spatialSearchQuery->result())
				{
					if (otherName == myName)
						continue;
					entity *e = entities()->get(otherName);
					CAGE_COMPONENT_ENGINE(transform, ot, e);
					vec3 toMonster = t.position - ot.position;
					if (e->has(monsterComponent::component))
					{
						real d = ot.scale + t.scale;
						if (lengthSquared(toMonster) < d*d)
							dispersion += normalize(toMonster) / length(toMonster);
					}
				}
				if (dispersion != vec3())
					v.velocity += normalize(dispersion) * m.dispersion;
			}

			// collision with player
			if (collisionTest(playerTransform.position, playerScale, playerVelocity.velocity, t.position, t.scale, v.velocity))
			{
				if (game.powerups[(uint32)powerupTypeEnum::Shield] > 0 && m.damage < real::Infinity())
				{
					statistics.shieldStoppedMonsters++;
					statistics.shieldAbsorbedDamage += m.damage;
					vec3 shieldDirection = normalize(t.position - playerTransform.position);
					vec3 shieldPosition = playerTransform.position + shieldDirection * (playerScale * 1.1);
					environmentExplosion(shieldPosition, shieldDirection * 0.5, vec3(1, 1, 1), min(m.damage, 5) * 2 + 2, 3); // shield sparks
				}
				else
				{
					environmentExplosion(interpolate(t.position, playerTransform.position, 0.5), interpolate(playerVelocity.velocity, v.velocity, 0.5), playerDeathColor, min(m.damage, 5) * 2 + 2, 3); // hull sparks
					statistics.monstersSucceded++;
					game.life -= lifeDamage(m.damage);
					if (statistics.monstersFirstHit == 0)
						statistics.monstersFirstHit = statistics.updateIterationIgnorePause;
					statistics.monstersLastHit = statistics.updateIterationIgnorePause;
					if (game.life > 0 && !game.cinematic)
					{
						uint32 sounds[] = {
							hashString("degrid/speech/damage/better-to-avoid-next-time.wav"),
							hashString("degrid/speech/damage/beware.wav"),
							hashString("degrid/speech/damage/critical-damage.wav"),
							hashString("degrid/speech/damage/damaged.wav"),
							hashString("degrid/speech/damage/dont-do-this.wav"),
							hashString("degrid/speech/damage/evasive-maneuvers.wav"),
							hashString("degrid/speech/damage/hit.wav"),
							hashString("degrid/speech/damage/hull-is-breached.wav"),
							hashString("degrid/speech/damage/shields-are-failing.wav"),
							hashString("degrid/speech/damage/warning.wav"),
							hashString("degrid/speech/damage/we-are-doomed.wav"),
							hashString("degrid/speech/damage/we-have-been-hit.wav"),
							0
						};
						soundSpeech(sounds);
					}
				}
				if (m.life < real::Infinity())
					killMonster(e, false);
			}

			// monster vertical movement
			v.velocity[1] = 0;
			t.position[1] = m.groundLevel;
		}

		bool hasBoss = bossComponent::component->group()->count() > 0;
		if (!game.cinematic)
		{
			// finished a boss fight
			if (wasBoss && !hasBoss)
			{
				CAGE_ASSERT(game.defeatedBosses < bossesTotalCount);
				achievementFullfilled(stringizer() + "boss-" + game.defeatedBosses, true);
				game.defeatedBosses++;
				game.money += numeric_cast<uint32>(game.life * 2);
				game.score += numeric_cast<uint64>(game.score * (double)game.life.value / 100);
				game.life += (100 - game.life) * 0.5;
			}
		}
		wasBoss = hasBoss;
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
	public:
		callbacksClass() : engineUpdateListener("monsters")
		{
			engineUpdateListener.attach(controlThread().update, 1);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

real lifeDamage(real damage)
{
	uint32 armor = game.powerups[(uint32)powerupTypeEnum::Armor];
	return damage / (armor * 0.5 + 1);
}

uint32 monsterMutation(uint32 &special)
{
	if (game.cinematic)
		return 0;
	static const real probabilities[] = { real(), real(), real(1e-4), real(1e-3), real(0.1), real(0.5) };
	uint32 res = 0;
	real probability = probabilities[min(game.defeatedBosses, numeric_cast<uint32>(sizeof(probabilities) / sizeof(probabilities[0]) - 1))];
	while (randomChance() < probability)
		res++;
	special += res ? 1 : 0;
	return res;
}

void monsterReflectMutation(entity *e, uint32 special)
{
	if (!special)
		return;
	CAGE_COMPONENT_ENGINE(transform, transform, e);
	transform.scale *= 1.3;
	statistics.monstersMutated++;
	statistics.monstersMutations += special;
	achievementFullfilled("mutated");
}

entity *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life)
{
	statistics.monstersSpawned++;
	entity *m = entities()->createUnique();
	CAGE_COMPONENT_ENGINE(transform, transform, m);
	CAGE_COMPONENT_ENGINE(render, render, m);
	DEGRID_COMPONENT(monster, monster, m);
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

bool killMonster(entity *e, bool allowCallback)
{
	if (e->has(entitiesToDestroy))
		return false;
	e->add(entitiesToDestroy);
	monsterExplosion(e);
	DEGRID_COMPONENT(monster, m, e);
	m.life = 0;
	game.score += numeric_cast<uint32>(clamp(m.damage, 1, 200));
	if (m.defeatedSound)
	{
		CAGE_COMPONENT_ENGINE(transform, t, e);
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
