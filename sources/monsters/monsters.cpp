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

		CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
		DEGRID_COMPONENT(Velocity, playerVelocity, game.playerEntity);

		for (Entity *e : MonsterComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, t, e);
			DEGRID_COMPONENT(Velocity, v, e);
			DEGRID_COMPONENT(Monster, m, e);

			// monster dispersion
			if (m.dispersion > 0)
			{
				uint32 myName = e->name();
				vec3 dispersion;
				SpatialSearchQuery->intersection(sphere(t.position, t.scale + 1));
				for (uint32 otherName : SpatialSearchQuery->result())
				{
					if (otherName == myName)
						continue;
					Entity *e = engineEntities()->get(otherName);
					CAGE_COMPONENT_ENGINE(Transform, ot, e);
					vec3 toMonster = t.position - ot.position;
					if (e->has(MonsterComponent::component))
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
				vec3 enemyDir = normalize(t.position - playerTransform.position);
				if (game.powerups[(uint32)PowerupTypeEnum::Shield] > 0 && m.damage < real::Infinity())
				{
					statistics.shieldStoppedMonsters++;
					statistics.shieldAbsorbedDamage += m.damage;
					environmentExplosion(playerTransform.position + enemyDir * (playerScale * 1.1), playerVelocity.velocity + enemyDir * 0.5, vec3(1), 1); // shield sparks
				}
				else
				{
					environmentExplosion(playerTransform.position + enemyDir * playerScale, playerVelocity.velocity + enemyDir * 0.5, playerDeathColor, 1); // hull sparks
					statistics.monstersSucceded++;
					game.life -= lifeDamage(m.damage);
					if (statistics.monstersFirstHit == 0)
						statistics.monstersFirstHit = statistics.updateIterationIgnorePause;
					statistics.monstersLastHit = statistics.updateIterationIgnorePause;
					if (game.life > 0 && !game.cinematic)
					{
						uint32 sounds[] = {
							HashString("degrid/speech/damage/better-to-avoid-next-time.wav"),
							HashString("degrid/speech/damage/beware.wav"),
							HashString("degrid/speech/damage/critical-damage.wav"),
							HashString("degrid/speech/damage/damaged.wav"),
							HashString("degrid/speech/damage/dont-do-this.wav"),
							HashString("degrid/speech/damage/evasive-maneuvers.wav"),
							HashString("degrid/speech/damage/hit.wav"),
							HashString("degrid/speech/damage/hull-is-breached.wav"),
							HashString("degrid/speech/damage/shields-are-failing.wav"),
							HashString("degrid/speech/damage/warning.wav"),
							HashString("degrid/speech/damage/we-are-doomed.wav"),
							HashString("degrid/speech/damage/we-have-been-hit.wav"),
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

		bool hasBoss = BossComponent::component->group()->count() > 0;
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

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks() : engineUpdateListener("monsters")
		{
			engineUpdateListener.attach(controlThread().update, 1);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

real lifeDamage(real damage)
{
	uint32 armor = game.powerups[(uint32)PowerupTypeEnum::Armor];
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

void monsterReflectMutation(Entity *e, uint32 special)
{
	if (!special)
		return;
	CAGE_COMPONENT_ENGINE(Transform, transform, e);
	transform.scale *= 1.3;
	statistics.monstersMutated++;
	statistics.monstersMutations += special;
	achievementFullfilled("mutated");
}

Entity *initializeMonster(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life)
{
	statistics.monstersSpawned++;
	Entity *m = engineEntities()->createUnique();
	CAGE_COMPONENT_ENGINE(Transform, transform, m);
	CAGE_COMPONENT_ENGINE(Render, render, m);
	DEGRID_COMPONENT(Monster, monster, m);
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

bool killMonster(Entity *e, bool allowCallback)
{
	if (e->has(entitiesToDestroy))
		return false;
	e->add(entitiesToDestroy);
	monsterExplosion(e);
	DEGRID_COMPONENT(Monster, m, e);
	m.life = 0;
	game.score += numeric_cast<uint32>(clamp(m.damage, 1, 200));
	if (m.defeatedSound)
	{
		CAGE_COMPONENT_ENGINE(Transform, t, e);
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
