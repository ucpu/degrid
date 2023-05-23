#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

namespace
{
	struct SnakeTailComponent
	{
		uint32 index = 0;
		uint32 follow = 0;
	};

	struct SnakeHeadComponent
	{
		Real speedMin, speedMax;
	};

	const auto engineInitListener = controlThread().initialize.listen([]() {
		engineEntities()->defineComponent(SnakeTailComponent());
		engineEntities()->defineComponent(SnakeHeadComponent());
	});

	void snakeSideMove(Vec3 &p, const Quat &forward, uint32 index, Real dist)
	{
		const Real phase = dist * index / -3;
		p += forward * Vec3(1, 0, 0) * sin(Rads(statistics.updateIteration * 0.2f + phase)) * 0.4;
	}

	const auto engineUpdateListener = controlThread().update.listen([]() {
		if (game.paused)
			return;

		// snake heads
		entitiesVisitor([&](TransformComponent &tr, VelocityComponent &v, const SnakeHeadComponent &snake) {
			v.velocity += randomDirection3() * Vec3(1, 0, 1) * 0.03;
			const Real s = length(v.velocity);
			if (s < snake.speedMin || s > snake.speedMax)
				v.velocity = randomDirection3() * (snake.speedMin + snake.speedMax) * 0.5;
			v.velocity += (game.monstersTarget - tr.position) * 0.0001;
			v.velocity[1] = 0;
			tr.orientation = Quat(v.velocity, Vec3(0, 1, 0));
			snakeSideMove(tr.position, tr.orientation, 0, tr.scale * 2);
		}, engineEntities(), false);

		// snake tails
		EntityComponent *history = engineEntities()->componentsByType(detail::typeIndex<TransformComponent>())[1];
		entitiesVisitor([&](Entity *e, TransformComponent &tr, VelocityComponent &v, MonsterComponent &m, SnakeTailComponent &snake) {
			v.velocity = Vec3();
			if (snake.follow && engineEntities()->has(snake.follow))
			{
				Entity *p = engineEntities()->get(snake.follow);
				const TransformComponent &trp = p->value<TransformComponent>();
				const Vec3 toPrev = trp.position - tr.position;
				const Real r = tr.scale * 2;
				const Real d2 = lengthSquared(toPrev);
				if (d2 > sqr(r) + 0.01)
				{
					if (d2 > sqr(r + 20))
					{ // teleport
						tr.position = trp.position + randomChance3() - 0.5;
						e->remove(history);
					}
					else
					{ // move
						v.velocity = normalize(toPrev) * (length(toPrev) - r);
						tr.orientation = Quat(toPrev, Vec3(0, 1, 0));
					}
				}
			}
			else if (snake.follow)
			{
				snake.follow = 0;
				m.life = 10;
			}
			else
			{
				m.life -= 1;
				if (m.life < 0)
					killMonster(e, true);
			}
			snakeSideMove(tr.position, tr.orientation, snake.index, tr.scale * 2);
		}, engineEntities(), false);
	});

	const auto snakeTailsListener = controlThread().update.listen([]() {
		// snake tails should not count towards monster limit
		const uint32 sub = engineEntities()->component<SnakeTailComponent>()->count();
		statistics.monstersCurrent = sub < statistics.monstersCurrent ? statistics.monstersCurrent - sub : 0;
	}, -59); // right after statistics
}

void spawnSnake(const Vec3 &spawnPosition, const Vec3 &color)
{
	const bool snakeJoke = randomRange(0u, 1000u) == 42;
	if (snakeJoke)
		makeAnnouncement(HashString("announcement/joke-snake"), HashString("announcement-desc/joke-snake"));
	uint32 special = 0;
	uint32 prev = 0;
	Real groundLevel;
	Real scale;
	{ // head
		Entity *head = initializeMonster(spawnPosition, color, 2, HashString("degrid/monster/snakeHead.object"), HashString("degrid/monster/bum-snake-head.ogg"), 5, (snakeJoke ? 100 : 3) + monsterMutation(special));
		SnakeHeadComponent &snake = head->value<SnakeHeadComponent>();
		snake.speedMin = 0.3 + 0.1 * monsterMutation(special);
		snake.speedMax = snake.speedMin + 0.6 + 0.2 * monsterMutation(special);
		monsterReflectMutation(head, special);
		prev = head->name();
		MonsterComponent &monster = head->value<MonsterComponent>();
		monster.dispersion = 0.2;
		groundLevel = monster.groundLevel;
		scale = head->value<TransformComponent>().scale;
		head->value<VelocityComponent>();
	}
	const uint32 pieces = (snakeJoke ? randomRange(80, 100) : randomRange(10, 13)) + monsterMutation(special) * 2;
	const uint64 aniInitOff = randomRange(0, 10000000);
	for (uint32 i = 0; i < pieces; i++)
	{ // tail
		Entity *tail = initializeMonster(spawnPosition + Vec3(randomChance() - 0.5, 0, randomChance() - 0.5) * 10, color, scale, HashString("degrid/monster/snakeTail.object"), HashString("degrid/monster/bum-snake-tail.ogg"), 5, Real::Infinity());
		SnakeTailComponent &snake = tail->value<SnakeTailComponent>();
		snake.index = i + 1;
		snake.follow = prev;
		prev = tail->name();
		tail->value<TextureAnimationComponent>().startTime = engineControlTime() + aniInitOff + i * 1000000;
		MonsterComponent &monster = tail->value<MonsterComponent>();
		monster.dispersion = 0.2;
		TransformComponent &transform = tail->value<TransformComponent>();
		transform.position[1] = monster.groundLevel = groundLevel;
		CAGE_ASSERT(transform.scale == scale);
	}
}
