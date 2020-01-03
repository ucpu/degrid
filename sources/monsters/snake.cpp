#include "monsters.h"

namespace
{
	struct SnakeTailComponent
	{
		static EntityComponent *component;
		uint32 index;
		uint32 follow;
		SnakeTailComponent() : index(0), follow(0) {}
	};

	struct SnakeHeadComponent
	{
		static EntityComponent *component;
		real speedMin, speedMax;
	};

	EntityComponent *SnakeHeadComponent::component;
	EntityComponent *SnakeTailComponent::component;

	void engineInit()
	{
		SnakeTailComponent::component = engineEntities()->defineComponent(SnakeTailComponent(), true);
		SnakeHeadComponent::component = engineEntities()->defineComponent(SnakeHeadComponent(), true);
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

		// snake heads
		for (Entity *e : SnakeHeadComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(Velocity, v, e);
			DEGRID_COMPONENT(SnakeHead, snake, e);
			v.velocity += randomDirection3() * vec3(1, 0, 1) * 0.03;
			real s = length(v.velocity);
			if (s < snake.speedMin || s > snake.speedMax)
				v.velocity = randomDirection3() * (snake.speedMin + snake.speedMax) * 0.5;
			v.velocity += (game.monstersTarget - tr.position) * 0.0001;
			v.velocity[1] = 0;
			tr.orientation = quat(v.velocity, vec3(0, 1, 0));
			snakeSideMove(tr.position, tr.orientation, 0, tr.scale * 2);
		}

		// snake tails
		for (Entity *e : SnakeTailComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(Velocity, v, e);
			DEGRID_COMPONENT(Monster, m, e);
			DEGRID_COMPONENT(SnakeTail, snake, e);

			v.velocity = vec3();
			if (snake.follow && engineEntities()->has(snake.follow))
			{
				Entity *p = engineEntities()->get(snake.follow);
				CAGE_COMPONENT_ENGINE(Transform, trp, p);
				vec3 toPrev = trp.position - tr.position;
				real r = tr.scale * 2;
				real d2 = lengthSquared(toPrev);
				if (d2 > sqr(r) + 0.01)
				{
					if (d2 > sqr(r + 20))
					{ // teleport
						tr.position = trp.position + randomChance3() - 0.5;
						e->remove(TransformComponent::componentHistory);
					}
					else
					{ // move
						v.velocity = normalize(toPrev) * (length(toPrev) - r);
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
		uint32 sub = SnakeTailComponent::component->group()->count();
		statistics.monstersCurrent = sub < statistics.monstersCurrent ? statistics.monstersCurrent - sub : 0;
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
		EventListener<void()> snakeTailsListener;
	public:
		Callbacks()
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
		makeAnnouncement(HashString("announcement/joke-snake"), HashString("announcement-desc/joke-snake"));
	uint32 special = 0;
	uint32 prev = 0;
	real groundLevel;
	real scale;
	{ // head
		Entity *head = initializeMonster(spawnPosition, color, 2, HashString("degrid/monster/snakeHead.object"), HashString("degrid/monster/bum-snake-head.ogg"), 5, (snakeJoke ? 100 : 3) + monsterMutation(special));
		DEGRID_COMPONENT(SnakeHead, snake, head);
		snake.speedMin = 0.3 + 0.1 * monsterMutation(special);
		snake.speedMax = snake.speedMin + 0.6 + 0.2 * monsterMutation(special);
		monsterReflectMutation(head, special);
		prev = head->name();
		DEGRID_COMPONENT(Monster, monster, head);
		monster.dispersion = 0.2;
		groundLevel = monster.groundLevel;
		CAGE_COMPONENT_ENGINE(Transform, transform, head);
		scale = transform.scale;
	}
	uint32 pieces = (snakeJoke ? randomRange(80, 100) : randomRange(10, 13)) + monsterMutation(special) * 2;
	uint64 aniInitOff = randomRange(0, 10000000);
	for (uint32 i = 0; i < pieces; i++)
	{ // tail
		Entity *tail = initializeMonster(spawnPosition + vec3(randomChance() - 0.5, 0, randomChance() - 0.5), color, scale, HashString("degrid/monster/snakeTail.object"), HashString("degrid/monster/bum-snake-tail.ogg"), 5, real::Infinity());
		DEGRID_COMPONENT(SnakeTail, snake, tail);
		snake.index = i + 1;
		snake.follow = prev;
		prev = tail->name();
		CAGE_COMPONENT_ENGINE(TextureAnimation, aniTex, tail);
		aniTex.startTime = engineControlTime() + aniInitOff + i * 1000000;
		DEGRID_COMPONENT(Monster, monster, tail);
		monster.dispersion = 0.2;
		CAGE_COMPONENT_ENGINE(Transform, transform, tail);
		transform.position[1] = monster.groundLevel = groundLevel;
		CAGE_ASSERT(transform.scale == scale);
	}
}
