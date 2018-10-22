#include "monsters.h"

namespace
{
	struct snakeTailComponent
	{
		static componentClass *component;
		uint32 index;
		uint32 follow;
		snakeTailComponent() : index(0), follow(0) {}
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

	void snakeSideMove(vec3 &p, const quat &forward, uint32 index, real dist)
	{
		real phase = dist * index / -3;
		p += forward * vec3(1, 0, 0) * sin(rads(statistics.updateIterationWithPause * 0.2f + phase)) * 0.4;
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);

		// snake heads
		for (entityClass *e : snakeHeadComponent::component->entities())
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
			tr.orientation = quat(v.velocity, vec3(0, 1, 0));
			snakeSideMove(tr.position, tr.orientation, 0, tr.scale * 2);
		}

		// snake tails
		for (entityClass *e : snakeTailComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			GRID_GET_COMPONENT(velocity, v, e);
			GRID_GET_COMPONENT(monster, m, e);
			GRID_GET_COMPONENT(snakeTail, snake, e);

			v.velocity = vec3();
			if (snake.follow && entities()->has(snake.follow))
			{
				entityClass *p = entities()->get(snake.follow);
				ENGINE_GET_COMPONENT(transform, trp, p);
				vec3 toPrev = trp.position - tr.position;
				real r = tr.scale * 2;
				if (toPrev.squaredLength() > r * r + 0.01)
				{
					v.velocity = toPrev.normalize() * (toPrev.length() - r);
					tr.orientation = quat(toPrev, vec3(0, 1, 0));
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
	if (randomRange(0u, 1000u) == 42)
	{
		CAGE_LOG(severityEnum::Info, "joke", "JOKE: monster snake");
		pieces = randomRange(80, 100);
	}
	else
		pieces = randomRange(10, 13) + monsterMutation(special) * 2;
	uint32 prev = 0;
	real groundLevel;
	real scale;
	{ // head
		entityClass *head = initializeMonster(spawnPosition, color, 2, hashString("grid/monster/snakeHead.object"), hashString("grid/monster/bum-snake-head.ogg"), 5, 3 + monsterMutation(special));
		GRID_GET_COMPONENT(snakeHead, snake, head);
		snake.speedMin = 0.3 + 0.1 * monsterMutation(special);
		snake.speedMax = snake.speedMin + 0.6 + 0.2 * monsterMutation(special);
		monsterReflectMutation(head, special);
		prev = head->name();
		GRID_GET_COMPONENT(monster, monster, head);
		groundLevel = monster.groundLevel;
		ENGINE_GET_COMPONENT(transform, transform, head);
		scale = transform.scale;
	}
	uint64 aniInitOff = randomRange(0, 10000000);
	for (uint32 i = 0; i < pieces; i++)
	{ // tail
		entityClass *tail = initializeMonster(spawnPosition + vec3(randomChance() - 0.5, 0, randomChance() - 0.5), color, scale, hashString("grid/monster/snakeTail.object"), hashString("grid/monster/bum-snake-tail.ogg"), 5, real::PositiveInfinity);
		GRID_GET_COMPONENT(snakeTail, snake, tail);
		snake.index = i + 1;
		snake.follow = prev;
		prev = tail->name();
		ENGINE_GET_COMPONENT(animatedTexture, aniTex, tail);
		aniTex.startTime = currentControlTime() + aniInitOff + i * 1000000;
		aniTex.speed = 0.4;
		GRID_GET_COMPONENT(monster, monster, tail);
		ENGINE_GET_COMPONENT(transform, transform, tail);
		transform.position[1] = monster.groundLevel = groundLevel;
		CAGE_ASSERT_RUNTIME(transform.scale == scale);
	}
}
