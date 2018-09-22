#include "monsters.h"

namespace
{
	struct snakeTailComponent
	{
		static componentClass *component;
		uint32 follow;
		snakeTailComponent() : follow(0) {}
	};

	struct snakeHeadComponent
	{
		static componentClass *component;
		real speedMin, speedMax;
	};

	componentClass *snakeHeadComponent::component;
	componentClass *snakeTailComponent::component;

	void engineInit()
	{
		snakeTailComponent::component = entities()->defineComponent(snakeTailComponent(), true);
		snakeHeadComponent::component = entities()->defineComponent(snakeHeadComponent(), true);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);

		// snake heads
		for (entityClass *e : snakeHeadComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			GRID_GET_COMPONENT(velocity, v, e);
			GRID_GET_COMPONENT(snakeHead, snake, e);
			if (tr.position.distance(playerTransform.position) > 250)
				v.velocity = (game.monstersTarget - tr.position).normalize() * (snake.speedMin + snake.speedMax) * 0.5;
			else
			{
				v.velocity += randomDirection3() * vec3(1, 0, 1) * 0.03;
				real s = v.velocity.length();
				if (s < snake.speedMin || s > snake.speedMax)
					v.velocity = randomDirection3() * (snake.speedMin + snake.speedMax) * 0.5;
			}
			v.velocity[1] = 0;
			tr.orientation = quat(degs(), aTan2(-v.velocity[2], -v.velocity[0]), degs());
		}

		// snake tails
		for (entityClass *e : snakeTailComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			GRID_GET_COMPONENT(velocity, v, e);
			GRID_GET_COMPONENT(monster, m, e);
			GRID_GET_COMPONENT(snakeTail, snake, e);

			if (snake.follow && entities()->hasEntity(snake.follow))
			{
				entityClass *p = entities()->getEntity(snake.follow);
				ENGINE_GET_COMPONENT(transform, trp, p);
				vec3 toPrev = trp.position - tr.position;
				real r = trp.scale + tr.scale;
				if (toPrev.squaredLength() > r * r + 0.01)
				{
					v.velocity = toPrev.normalize() * (toPrev.length() - r);
					tr.orientation = quat(degs(), aTan2(-v.velocity[2], -v.velocity[0]), degs());
				}
				else
					v.velocity = vec3();
			}
			else
			{
				if (m.life > 1e5)
				{
					snake.follow = 0;
					v.velocity = vec3();
					m.life = 10;
				}
				else
				{
					m.life -= 1;
					if (m.life < 0)
					{
						e->addGroup(entitiesToDestroy);
						soundEffect(m.defeatedSound, tr.position);
					}
				}
			}
		}
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
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
		aniTex.startTime = currentControlTime() + aniInitOff + i * 1000000;
		aniTex.speed = 0.4;
		GRID_GET_COMPONENT(monster, monster, tail);
		ENGINE_GET_COMPONENT(transform, transform, tail);
		transform.position[1] = monster.groundLevel = groundLevel;
	}
}
