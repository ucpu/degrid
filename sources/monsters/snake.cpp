#include "monsters.h"

namespace grid
{
	void updateSnake()
	{
		{ // snake heads
			for (entityClass *e : snakeHeadStruct::component->getComponentEntities()->entities())
			{
				ENGINE_GET_COMPONENT(transform, tr, e);
				GRID_GET_COMPONENT(monster, m, e);
				GRID_GET_COMPONENT(snakeHead, snake, e);
				if (tr.position.distance(player.position) > 250)
					m.speed = (player.monstersTarget - tr.position).normalize() * (snake.speedMin + snake.speedMax) * 0.5;
				else
				{
					m.speed += randomDirection3() * vec3(1, 0, 1) * 0.03;
					real s = m.speed.length();
					if (s < snake.speedMin || s > snake.speedMax)
						m.speed = randomDirection3() * (snake.speedMin + snake.speedMax) * 0.5;
				}
				m.speed[1] = 0;
				tr.orientation = quat(degs(), aTan2(-m.speed[2], -m.speed[0]), degs());
			}
		}
		{ // snake tails
			for (entityClass *e : snakeTailStruct::component->getComponentEntities()->entities())
			{
				ENGINE_GET_COMPONENT(transform, tr, e);
				GRID_GET_COMPONENT(monster, m, e);
				GRID_GET_COMPONENT(snakeTail, snake, e);

				if (entities()->hasEntity(snake.follow))
				{
					entityClass *p = entities()->getEntity(snake.follow);
					ENGINE_GET_COMPONENT(transform, trp, p);
					vec3 toPrev = trp.position - tr.position;
					real r = trp.scale + tr.scale;
					if (toPrev.squaredLength() > r * r + 0.01)
					{
						m.speed = toPrev.normalize() * (toPrev.length() - r);
						tr.orientation = quat(degs(), aTan2(-m.speed[2], -m.speed[0]), degs());
					}
					else
						m.speed = vec3();
				}
				else
				{
					if (m.life > 1e5)
					{
						m.speed = vec3();
						m.life = 10;
					}
					else
					{
						m.life -= 1;
						if (m.life < 0)
						{
							e->addGroup(entitiesToDestroy);
							soundEffect(m.destroyedSound, tr.position);
						}
					}
				}
			}
		}
	}

	void spawnSnake(const vec3 &spawnPosition, const vec3 &color)
	{
		uint32 special = 0;
		uint32 pieces = 0;
		if (random() < 0.0001)
		{
			CAGE_LOG(severityEnum::Info, "joke", "JOKE: monster snake");
			pieces = random(100, 150);
		}
		else
			pieces = random(8, 12) + spawnSpecial(special);
		uint32 prev = 0;
		real groundLevel;
		{ // head
			entityClass *head = initializeMonster(spawnPosition, color, 2, hashString("grid/monster/snakeHead.object"), hashString("grid/monster/bum-snake-head.ogg"), 5, 3 + spawnSpecial(special));
			GRID_GET_COMPONENT(snakeHead, snake, head);
			snake.speedMin = 0.3 + 0.1 * spawnSpecial(special);
			snake.speedMax = snake.speedMin + 0.6 + 0.2 * spawnSpecial(special);
			if (special > 0)
			{
				ENGINE_GET_COMPONENT(transform, transform, head);
				transform.scale *= 1.2;
				statistics.monstersSpecial++;
			}
			prev = head->getName();
			GRID_GET_COMPONENT(monster, monster, head);
			groundLevel = monster.groundLevel;
		}
		uint64 aniInitOff = random(0, 10000000);
		for (uint32 i = 0; i < pieces; i++)
		{ // tail
			entityClass *tail = initializeMonster(spawnPosition + vec3(random() - 0.5, 0, random() - 0.5), color, special ? 2.4 : 2, hashString("grid/monster/snakeTail.object"), hashString("grid/monster/bum-snake-tail.ogg"), 5, real::PositiveInfinity);
			GRID_GET_COMPONENT(snakeTail, snake, tail);
			snake.follow = prev;
			prev = tail->getName();
			ENGINE_GET_COMPONENT(animatedTexture, aniTex, tail);
			aniTex.startTime = player.updateTime + aniInitOff + i * 1000000;
			aniTex.speed = 0.4;
			GRID_GET_COMPONENT(monster, monster, tail);
			ENGINE_GET_COMPONENT(transform, transform, tail);
			transform.position[1] = monster.groundLevel = groundLevel;
		}
	}
}
