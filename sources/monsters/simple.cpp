#include "monsters.h"

namespace grid
{
	namespace
	{
		struct monsterUpdateStruct
		{
			transformComponent &tr;
			monsterComponent &ms;
			simpleMonsterComponent &sm;
			uint32 closestShot;
			real closestDistance;
			uint32 myName;

			monsterUpdateStruct(entityClass *e) :
				tr(e->value<transformComponent>(transformComponent::component)),
				ms(e->value<monsterComponent>(monsterComponent::component)),
				sm(e->value<simpleMonsterComponent>(simpleMonsterComponent::component)),
				closestShot(0), closestDistance(real::PositiveInfinity), myName(e->getName())
			{
				if (ms.speed.squaredLength() > sm.maxSpeed * sm.maxSpeed + 0.0001)
					ms.speed = ms.speed.normalize() * max(sm.maxSpeed, ms.speed.length() - sm.acceleration);
				else
				{
					spatialQuery->intersection(sphere(tr.position, 15));
					const uint32 *res = spatialQuery->resultArray();
					for (uint32 i = 0, e = spatialQuery->resultCount(); i != e; i++)
						test(res[i]);

					vec3 will = normalize(player.monstersTarget - tr.position);

					if (closestShot)
					{
						entityClass *s = entities()->getEntity(closestShot);
						ENGINE_GET_COMPONENT(transform, ot, s);
						GRID_GET_COMPONENT(shot, os, s);
						vec3 a = tr.position - ot.position;
						vec3 b = os.speed.normalize();
						vec3 avoid = normalize(a - dot(a, b) * b);
						will = interpolate(will, avoid, sm.avoidance);
					}

					ms.speed += will * sm.acceleration;
					if (ms.speed.squaredLength() > sm.maxSpeed * sm.maxSpeed)
						ms.speed = ms.speed.normalize() * sm.maxSpeed;
				}

				tr.orientation = sm.animation * tr.orientation;
			}

			void test(uint32 otherName)
			{
				if (otherName == myName || !entities()->hasEntity(otherName))
					return;

				entityClass *e = entities()->getEntity(otherName);
				ENGINE_GET_COMPONENT(transform, ot, e);
				vec3 toMonster = tr.position - ot.position;

				if (!e->hasComponent(shotComponent::component))
					return;

				// test whether other is closer
				if (toMonster.squaredLength() >= closestDistance * closestDistance)
					return;

				GRID_GET_COMPONENT(shot, os, e);

				// test its direction
				if (dot(toMonster.normalize(), os.speed.normalize()) < 0)
					return;

				closestShot = otherName;
				closestDistance = toMonster.length();
			}
		};

		void spawnSmallCube(uint32 originalEntity)
		{
			entityClass *e = entities()->getEntity(originalEntity);
			ENGINE_GET_COMPONENT(transform, t, e);
			ENGINE_GET_COMPONENT(render, r, e);
			for (uint32 i = 0; i < 2; i++)
				spawnSimple(monsterTypeFlags::SmallCube, t.position + vec3(random() - 0.5, 0, random() - 0.5), r.color);
		}

		void spawnSmallTriangle(uint32 originalEntity)
		{
			entityClass *e = entities()->getEntity(originalEntity);
			ENGINE_GET_COMPONENT(transform, t, e);
			ENGINE_GET_COMPONENT(render, r, e);
			for (uint32 i = 0; i < 2; i++)
				spawnSimple(monsterTypeFlags::SmallTriangle, t.position + vec3(random() - 0.5, 0, random() - 0.5), r.color);
		}

		entityClass *initializeSimple(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life, real maxSpeed, real accelerationFraction, real avoidance, real dispersion, const quat &animation)
		{
			entityClass *e = initializeMonster(spawnPosition, color, scale, objectName, deadSound, damage, life);
			GRID_GET_COMPONENT(monster, m, e);
			GRID_GET_COMPONENT(simpleMonster, s, e);
			m.speed = randomDirection3();
			m.speed[1] = 0;
			m.dispersion = dispersion;
			s.avoidance = avoidance;
			s.animation = animation;
			s.maxSpeed = maxSpeed;
			s.acceleration = s.maxSpeed / accelerationFraction;
			return e;
		}
	}

	void spawnSimple(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color)
	{
		entityClass *e = nullptr;
		uint32 special = 0;
		switch (type)
		{
		case monsterTypeFlags::Circle:
			e = initializeSimple(spawnPosition, color, 2, hashString("grid/monster/smallCircle.object"), hashString("grid/monster/bum-circle.ogg"), 2, 1 + spawnSpecial(special), 0.3 + 0.1 * spawnSpecial(special), 2, 0, 1, quat());
			break;
		case monsterTypeFlags::SmallTriangle:
			e = initializeSimple(spawnPosition, color, 2.5, hashString("grid/monster/smallTriangle.object"), hashString("grid/monster/bum-triangle.ogg"), 3, 1 + spawnSpecial(special), 0.4 + 0.1 * spawnSpecial(special), 50, 0, 0.02, quat(degs(), randomAngle() / 50, degs()));
			break;
		case monsterTypeFlags::SmallCube:
			e = initializeSimple(spawnPosition, color, 2.5, hashString("grid/monster/smallCube.object"), hashString("grid/monster/bum-cube.ogg"), 3, 1 + spawnSpecial(special), 0.3 + 0.1 * spawnSpecial(special), 3, 1, 0.2, quat(randomAngle() / 100, randomAngle() / 100, randomAngle() / 100));
			break;
		case monsterTypeFlags::LargeTriangle:
			e = initializeSimple(spawnPosition, color, 3, hashString("grid/monster/largeTriangle.object"), hashString("grid/monster/bum-triangle.ogg"), 4, 1 + spawnSpecial(special), 0.4 + 0.1 * spawnSpecial(special), 50, 0, 0.02, quat(degs(), randomAngle() / 50, degs()));
			{
				GRID_GET_COMPONENT(monster, m, e);
				m.shotDownCallback.bind<&spawnSmallTriangle>();
			}
			break;
		case monsterTypeFlags::LargeCube:
			e = initializeSimple(spawnPosition, color, 3, hashString("grid/monster/largeCube.object"), hashString("grid/monster/bum-cube.ogg"), 4, 1 + spawnSpecial(special), 0.3 + 0.1 * spawnSpecial(special), 3, 1, 0.2, quat(randomAngle() / 100, randomAngle() / 100, randomAngle() / 100));
			{
				GRID_GET_COMPONENT(monster, m, e);
				m.shotDownCallback.bind<&spawnSmallCube>();
			}
			break;
		case monsterTypeFlags::PinWheel:
			e = initializeSimple(spawnPosition, color, 3.5, hashString("grid/monster/pinWheel.object"), hashString("grid/monster/bum-pinwheel.ogg"), 4, 1 + spawnSpecial(special), 2 + 0.4 * spawnSpecial(special), 50, 0, 0, quat(degs(), degs(20), degs()));
			break;
		case monsterTypeFlags::Diamond:
			e = initializeSimple(spawnPosition, color, 3, hashString("grid/monster/largeDiamond.object"), hashString("grid/monster/bum-diamond.ogg"), 4, 1 + spawnSpecial(special), 0.7 + 0.15 * spawnSpecial(special), 1, 0.9, 0.2, quat(randomAngle() / 100, randomAngle() / 100, randomAngle() / 100));
			break;
		default: CAGE_THROW_CRITICAL(exception, "invalid monster type");
		}
		if (special > 0)
		{
			ENGINE_GET_COMPONENT(transform, transform, e);
			transform.scale *= 1.2;
			statistics.monstersSpecial++;
		}
	}

	void updateSimple()
	{
		for (entityClass *e : simpleMonsterComponent::component->getComponentEntities()->entities())
			monsterUpdateStruct u(e);
	}
}
