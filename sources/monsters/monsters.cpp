#include <cage-core/color.h>
#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

namespace
{
	bool wasBoss = false;

	const auto engineUpdateListener = controlThread().update.listen([]() {
		if (game.paused)
			return;

		const TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
		const VelocityComponent &playerVelocity = game.playerEntity->value<VelocityComponent>();

		entitiesVisitor([&](Entity *e, TransformComponent &t, VelocityComponent &v, const MonsterComponent &m) {
			// monster dispersion
			if (m.dispersion > 0)
			{
				uint32 myName = e->name();
				Vec3 dispersion;
				spatialSearchQuery->intersection(Sphere(t.position, t.scale + 1));
				for (uint32 otherName : spatialSearchQuery->result())
				{
					if (otherName == myName)
						continue;
					Entity *e = engineEntities()->get(otherName);
					TransformComponent &ot = e->value<TransformComponent>();
					Vec3 toMonster = t.position - ot.position;
					if (e->has<MonsterComponent>())
					{
						Real d = ot.scale + t.scale;
						if (lengthSquared(toMonster) < d * d)
							dispersion += normalize(toMonster) / length(toMonster);
					}
				}
				if (dispersion != Vec3())
					v.velocity += normalize(dispersion) * m.dispersion;
			}

			// collision with player
			if (collisionTest(playerTransform.position, PlayerScale, playerVelocity.velocity, t.position, t.scale, v.velocity))
			{
				Vec3 enemyDir = normalize(t.position - playerTransform.position);
				if (game.powerups[(uint32)PowerupTypeEnum::Shield] > 0 && m.damage < Real::Infinity())
				{
					statistics.shieldStoppedMonsters++;
					statistics.shieldAbsorbedDamage += m.damage;
					environmentExplosion(playerTransform.position + enemyDir * (PlayerScale * 1.1), playerVelocity.velocity + enemyDir * 0.5, Vec3(1), 1);
				}
				else
				{
					environmentExplosion(playerTransform.position + enemyDir * PlayerScale, playerVelocity.velocity + enemyDir * 0.5, PlayerDeathColor, 1); // hull sparks
					statistics.monstersSucceded++;
					game.life -= lifeDamage(m.damage);
					if (statistics.monstersFirstHit == 0)
						statistics.monstersFirstHit = statistics.updateIterationIgnorePause;
					statistics.monstersLastHit = statistics.updateIterationIgnorePause;
					if (game.life > 0 && !game.cinematic)
					{
						static constexpr const uint32 Sounds[] = {
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
						soundSpeech(Sounds);
					}
				}
				if (m.life < Real::Infinity())
					killMonster(e, false);
			}

			// monster vertical movement
			v.velocity[1] = 0;
			t.position[1] = m.groundLevel;
		}, engineEntities(), false);

		const bool hasBoss = engineEntities()->component<BossComponent>()->count() > 0;
		if (!game.cinematic)
		{
			// finished a boss fight
			if (wasBoss && !hasBoss)
			{
				CAGE_ASSERT(game.defeatedBosses < BossesTotalCount);
				achievementFullfilled(Stringizer() + "boss-" + game.defeatedBosses, true);
				game.defeatedBosses++;
				const uint32 li = min(numeric_cast<uint32>(game.life), 100u);
				game.money += li * game.defeatedBosses / 2;
				game.score += numeric_cast<uint64>(game.score * (li * 0.01 + 0.5));
				game.life += (100 - li) * 0.5;
			}
		}
		wasBoss = hasBoss;
	}, 1);
}

Real lifeDamage(Real damage)
{
	const uint32 armor = game.powerups[(uint32)PowerupTypeEnum::Armor];
	return damage / (armor * 0.5 + 1);
}

uint32 monsterMutation(uint32 &special)
{
	if (game.cinematic)
		return 0;
	static constexpr const float probabilities[] = { 0, 0, 1e-4f, 1e-3f, 0.1f, 0.5f };
	uint32 res = 0;
	Real probability = probabilities[min(game.defeatedBosses, numeric_cast<uint32>(sizeof(probabilities) / sizeof(probabilities[0]) - 1))];
	while (randomChance() < probability)
		res++;
	special += res ? 1 : 0;
	return res;
}

void monsterReflectMutation(Entity *e, uint32 special)
{
	if (!special)
		return;
	e->value<TransformComponent>().scale *= 1.3;
	statistics.monstersMutated++;
	statistics.monstersMutations += special;
	achievementFullfilled("mutated");
}

Entity *initializeMonster(const Vec3 &spawnPosition, const Vec3 &color, Real scale, uint32 objectName, uint32 deadSound, Real damage, Real life)
{
	statistics.monstersSpawned++;
	Entity *m = engineEntities()->createUnique();
	TransformComponent &Transform = m->value<TransformComponent>();
	RenderComponent &render = m->value<RenderComponent>();
	MonsterComponent &monster = m->value<MonsterComponent>();
	Transform.orientation = Quat(Degs(), randomAngle(), Degs());
	Transform.position = spawnPosition;
	Transform.position[1] = monster.groundLevel = randomChance() * 2 - 1;
	Transform.scale = scale;
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
	MonsterComponent &m = e->value<MonsterComponent>();
	m.life = 0;
	game.score += numeric_cast<uint32>(clamp(m.damage, 1, 200));
	if (m.defeatedSound)
	{
		soundEffect(m.defeatedSound, e->value<TransformComponent>().position);
		m.defeatedSound = 0;
	}
	if (!allowCallback)
		return false;
	if (m.defeatedCallback)
	{
		auto clb = m.defeatedCallback;
		m.defeatedCallback.clear();
		clb(e->name());
		return false;
	}
	return true;
}
