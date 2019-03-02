#include "monsters.h"

componentClass *simpleMonsterComponent::component;

namespace
{
	void engineInit()
	{
		simpleMonsterComponent::component = entities()->defineComponent(simpleMonsterComponent(), true);
	}

	void spawnSmallCube(uint32 originalEntity)
	{
		entityClass *e = entities()->get(originalEntity);
		ENGINE_GET_COMPONENT(transform, t, e);
		ENGINE_GET_COMPONENT(render, r, e);
		for (uint32 i = 0; i < 2; i++)
			spawnSimple(monsterTypeFlags::SmallCube, t.position + vec3(randomChance() - 0.5, 0, randomChance() - 0.5), r.color);
	}

	void spawnSmallTriangle(uint32 originalEntity)
	{
		entityClass *e = entities()->get(originalEntity);
		ENGINE_GET_COMPONENT(transform, t, e);
		ENGINE_GET_COMPONENT(render, r, e);
		for (uint32 i = 0; i < 2; i++)
			spawnSimple(monsterTypeFlags::SmallTriangle, t.position + vec3(randomChance() - 0.5, 0, randomChance() - 0.5), r.color);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		for (entityClass *e : simpleMonsterComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, tr, e);
			DEGRID_GET_COMPONENT(velocity, mv, e);
			DEGRID_GET_COMPONENT(simpleMonster, sm, e);

			if (mv.velocity.squaredLength() > sm.maxSpeed * sm.maxSpeed + 1e-4)
				mv.velocity = mv.velocity.normalize() * max(sm.maxSpeed, mv.velocity.length() - sm.acceleration);
			else
			{
				vec3 will;
				// spiraling
				{
					vec3 direct = normalize(game.monstersTarget - tr.position);
					vec3 side = normalize(cross(direct, vec3(0, 1, 0)));
					if ((e->name() % 13) == 0)
						side *= -1;
					will = interpolate(direct, side, sm.spiraling);
				}

				// circling
				{
					rads ang = real(e->name() % 2) * rads::Stright + real(((e->name() / 2) % 30) / 30.0) * rads::Full + rads(currentControlTime() * 1e-6);
					vec3 dir = quat(degs(), ang, degs()) * vec3(0, 0, -1);
					will = interpolate(will, dir, sm.circling);
				}

				// closest shot
				uint32 closestShot = 0;
				{
					real closestDistance = real::PositiveInfinity;
					uint32 myName = e->name();
					spatialQuery->intersection(sphere(tr.position, 15));
					for (uint32 otherName : spatialQuery->result())
					{
						if (otherName == myName)
							continue;

						entityClass *e = entities()->get(otherName);

						if (!e->has(shotComponent::component))
							continue;

						ENGINE_GET_COMPONENT(transform, ot, e);
						vec3 toMonster = tr.position - ot.position;

						// test whether other is closer
						if (toMonster.squaredLength() >= closestDistance * closestDistance)
							continue;

						DEGRID_GET_COMPONENT(velocity, ov, e);

						// test its direction
						if (dot(toMonster.normalize(), ov.velocity.normalize()) < 0)
							continue;

						closestShot = otherName;
						closestDistance = toMonster.length();
					}
				}

				// avoidance
				if (closestShot)
				{
					entityClass *s = entities()->get(closestShot);
					ENGINE_GET_COMPONENT(transform, ot, s);
					DEGRID_GET_COMPONENT(velocity, ov, s);
					vec3 a = tr.position - ot.position;
					vec3 b = ov.velocity.normalize();
					vec3 avoid = normalize(a - dot(a, b) * b);
					will = interpolate(will, avoid, sm.avoidance);
				}

				mv.velocity += will * sm.acceleration;
				if (mv.velocity.squaredLength() > sm.maxSpeed * sm.maxSpeed)
					mv.velocity = mv.velocity.normalize() * sm.maxSpeed;
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

entityClass *initializeSimple(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life, real maxSpeed, real accelerationFraction, real avoidance, real dispersion, real circling, real spiraling, const quat &animation)
{
	entityClass *e = initializeMonster(spawnPosition, color, scale, objectName, deadSound, damage, life);
	DEGRID_GET_COMPONENT(velocity, v, e);
	DEGRID_GET_COMPONENT(monster, m, e);
	DEGRID_GET_COMPONENT(simpleMonster, s, e);
	DEGRID_GET_COMPONENT(rotation, rot, e);
	v.velocity = randomDirection3();
	v.velocity[1] = 0;
	m.dispersion = dispersion;
	s.avoidance = avoidance;
	rot.rotation = animation;
	s.maxSpeed = maxSpeed;
	s.circling = circling;
	s.spiraling = spiraling;
	s.acceleration = s.maxSpeed / accelerationFraction;
	return e;
}

void spawnSimple(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color)
{
	entityClass *e = nullptr;
	uint32 special = 0;
	switch (type)
	{
	case monsterTypeFlags::Circle:
		e = initializeSimple(spawnPosition, color, 2, hashString("degrid/monster/smallCircle.object"), hashString("degrid/monster/bum-circle.ogg"), 2, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 2, 0, 1, 0, 0.3, quat());
		break;
	case monsterTypeFlags::SmallTriangle:
		e = initializeSimple(spawnPosition, color, 2.5, hashString("degrid/monster/smallTriangle.object"), hashString("degrid/monster/bum-triangle.ogg"), 3, 1 + monsterMutation(special), 0.4 + 0.1 * monsterMutation(special), 50, 0, 0.02, 0.8, 0.1, quat(degs(), randomAngle() / 50, degs()));
		break;
	case monsterTypeFlags::SmallCube:
		e = initializeSimple(spawnPosition, color, 2.5, hashString("degrid/monster/smallCube.object"), hashString("degrid/monster/bum-cube.ogg"), 3, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 3, 1, 0.2, 0, 0.3, interpolate(quat(), randomDirectionQuat(), 0.01));
		break;
	case monsterTypeFlags::LargeTriangle:
		e = initializeSimple(spawnPosition, color, 3, hashString("degrid/monster/largeTriangle.object"), hashString("degrid/monster/bum-triangle.ogg"), 4, 1 + monsterMutation(special), 0.4 + 0.1 * monsterMutation(special), 50, 0, 0.02, 0.1, 0.4, quat(degs(), randomAngle() / 50, degs()));
		{
			DEGRID_GET_COMPONENT(monster, m, e);
			m.defeatedCallback.bind<&spawnSmallTriangle>();
		}
		break;
	case monsterTypeFlags::LargeCube:
		e = initializeSimple(spawnPosition, color, 3, hashString("degrid/monster/largeCube.object"), hashString("degrid/monster/bum-cube.ogg"), 4, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 3, 1, 0.2, 0, 0, interpolate(quat(), randomDirectionQuat(), 0.01));
		{
			DEGRID_GET_COMPONENT(monster, m, e);
			m.defeatedCallback.bind<&spawnSmallCube>();
		}
		break;
	case monsterTypeFlags::PinWheel:
		e = initializeSimple(spawnPosition, color, 3.5, hashString("degrid/monster/pinWheel.object"), hashString("degrid/monster/bum-pinwheel.ogg"), 4, 1 + monsterMutation(special), 2 + 0.4 * monsterMutation(special), 50, 0, 0.005, 0, 0, quat(degs(), degs(20), degs()));
		break;
	case monsterTypeFlags::Diamond:
		e = initializeSimple(spawnPosition, color, 3, hashString("degrid/monster/largeDiamond.object"), hashString("degrid/monster/bum-diamond.ogg"), 4, 1 + monsterMutation(special), 0.7 + 0.15 * monsterMutation(special), 1, 0.9, 0.2, 0, 0, interpolate(quat(), randomDirectionQuat(), 0.01));
		break;
	default: CAGE_THROW_CRITICAL(exception, "invalid monster type");
	}
	monsterReflectMutation(e, special);
}

