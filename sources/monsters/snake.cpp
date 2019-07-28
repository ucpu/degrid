#include "monsters.h"

namespace
{
	struct snakeTailComponent
	{
		static entityComponent *component;
		uint32 index;
		uint32 follow;
		snakeTailComponent() : index(0), follow(0) {}
	};

	struct snakeHeadComponent
	{
		static entityComponent *component;
		real speedMin, speedMax;
	};

	entityComponent *snakeHeadComponent::component;
	entityComponent *snakeTailComponent::component;

	void engineInit()
	{
		snakeTailComponent::component = entities()->defineComponent(snakeTailComponent(), true);
		snakeHeadComponent::component = entities()->defineComponent(snakeHeadComponent(), true);
	}

	void snakeSideMove(vec3 &p, const quat &forward, uint32 index, real dist)
	{
		real phase = dist * index / -3;
		p += forward * vec3(1, 0, 0) * sin(rads(statistics.updateIteration * 0.2f + phase)) * 0.4;
	}

	void engineUpdate()
	{
		OPTICK_EVENT("snake");

		if (game.paused)
			return;

		CAGE_COMPONENT_ENGINE(transform, playerTransform, game.playerEntity);

		// snake heads
		for (entity *e : snakeHeadComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, tr, e);
			DEGRID_COMPONENT(velocity, v, e);
			DEGRID_COMPONENT(snakeHead, snake, e);
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
		for (entity *e : snakeTailComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, tr, e);
			DEGRID_COMPONENT(velocity, v, e);
			DEGRID_COMPONENT(monster, m, e);
			DEGRID_COMPONENT(snakeTail, snake, e);

			v.velocity = vec3();
			if (snake.follow && entities()->has(snake.follow))
			{
				entity *p = entities()->get(snake.follow);
				CAGE_COMPONENT_ENGINE(transform, trp, p);
				vec3 toPrev = trp.position - tr.position;
				real r = tr.scale * 2;
				real d2 = toPrev.squaredLength();
				if (d2 > sqr(r) + 0.01)
				{
					if (d2 > sqr(r + 20))
					{ // teleport
						tr.position = trp.position + randomChance3() - 0.5;
						e->remove(transformComponent::componentHistory);
					}
					else
					{ // move
						v.velocity = toPrev.normalize() * (toPrev.length() - r);
						tr.orientation = quat(toPrev, vec3(0, 1, 0));
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
		}
	}

	void subtractSnakeTails()
	{
		// snake tails should not count towards monster limit
		uint32 sub = snakeTailComponent::component->group()->count();
		statistics.monstersCurrent = sub < statistics.monstersCurrent ? statistics.monstersCurrent - sub : 0;
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
		eventListener<void()> snakeTailsListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
			snakeTailsListener.attach(controlThread().update, -59); // right after statistics
			snakeTailsListener.bind<&subtractSnakeTails>();
		}
	} callbacksInstance;
}

void spawnSnake(const vec3 &spawnPosition, const vec3 &color)
{
	bool snakeJoke = randomRange(0u, 1000u) == 42;
	if (snakeJoke)
		makeAnnouncement(hashString("announcement/joke-snake"), hashString("announcement-desc/joke-snake"));
	uint32 special = 0;
	uint32 prev = 0;
	real groundLevel;
	real scale;
	{ // head
		entity *head = initializeMonster(spawnPosition, color, 2, hashString("degrid/monster/snakeHead.object"), hashString("degrid/monster/bum-snake-head.ogg"), 5, (snakeJoke ? 100 : 3) + monsterMutation(special));
		DEGRID_COMPONENT(snakeHead, snake, head);
		snake.speedMin = 0.3 + 0.1 * monsterMutation(special);
		snake.speedMax = snake.speedMin + 0.6 + 0.2 * monsterMutation(special);
		monsterReflectMutation(head, special);
		prev = head->name();
		DEGRID_COMPONENT(monster, monster, head);
		monster.dispersion = 0.2;
		groundLevel = monster.groundLevel;
		CAGE_COMPONENT_ENGINE(transform, transform, head);
		scale = transform.scale;
	}
	uint32 pieces = (snakeJoke ? randomRange(80, 100) : randomRange(10, 13)) + monsterMutation(special) * 2;
	uint64 aniInitOff = randomRange(0, 10000000);
	for (uint32 i = 0; i < pieces; i++)
	{ // tail
		entity *tail = initializeMonster(spawnPosition + vec3(randomChance() - 0.5, 0, randomChance() - 0.5), color, scale, hashString("degrid/monster/snakeTail.object"), hashString("degrid/monster/bum-snake-tail.ogg"), 5, real::Infinity());
		DEGRID_COMPONENT(snakeTail, snake, tail);
		snake.index = i + 1;
		snake.follow = prev;
		prev = tail->name();
		CAGE_COMPONENT_ENGINE(textureAnimation, aniTex, tail);
		aniTex.startTime = currentControlTime() + aniInitOff + i * 1000000;
		DEGRID_COMPONENT(monster, monster, tail);
		monster.dispersion = 0.2;
		CAGE_COMPONENT_ENGINE(transform, transform, tail);
		transform.position[1] = monster.groundLevel = groundLevel;
		CAGE_ASSERT_RUNTIME(transform.scale == scale);
	}
}
