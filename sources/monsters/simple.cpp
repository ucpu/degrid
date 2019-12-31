#include "monsters.h"

EntityComponent *SimpleMonsterComponent::component;

namespace
{
	void engineInit()
	{
		SimpleMonsterComponent::component = entities()->defineComponent(SimpleMonsterComponent(), true);
	}

	void spawnSmallCube(uint32 originalEntity)
	{
		Entity *e = entities()->get(originalEntity);
		CAGE_COMPONENT_ENGINE(Transform, t, e);
		CAGE_COMPONENT_ENGINE(Render, r, e);
		for (uint32 i = 0; i < 2; i++)
			spawnSimple(MonsterTypeFlags::SmallCube, t.position + vec3(randomChance() - 0.5, 0, randomChance() - 0.5), r.color);
	}

	void spawnSmallTriangle(uint32 originalEntity)
	{
		Entity *e = entities()->get(originalEntity);
		CAGE_COMPONENT_ENGINE(Transform, t, e);
		CAGE_COMPONENT_ENGINE(Render, r, e);
		for (uint32 i = 0; i < 2; i++)
			spawnSimple(MonsterTypeFlags::SmallTriangle, t.position + vec3(randomChance() - 0.5, 0, randomChance() - 0.5), r.color);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("simple monsters");

		if (game.paused)
			return;

		for (Entity *e : SimpleMonsterComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, tr, e);
			DEGRID_COMPONENT(Velocity, mv, e);
			DEGRID_COMPONENT(SimpleMonster, sm, e);

			if (lengthSquared(mv.velocity) > sm.maxSpeed * sm.maxSpeed + 1e-4)
				mv.velocity = normalize(mv.velocity) * max(sm.maxSpeed, length(mv.velocity) - sm.acceleration);
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
					rads ang = real(e->name() % 2) * rads::Full() * 0.5 + real(((e->name() / 2) % 30) / 30.0) * rads::Full() + rads(currentControlTime() * 1e-6);
					vec3 dir = quat(degs(), ang, degs()) * vec3(0, 0, -1);
					will = interpolate(will, dir, sm.circling);
				}

				// closest shot
				uint32 closestShot = 0;
				{
					real closestDistance = real::Infinity();
					uint32 myName = e->name();
					SpatialSearchQuery->intersection(sphere(tr.position, 15));
					for (uint32 otherName : SpatialSearchQuery->result())
					{
						if (otherName == myName)
							continue;

						Entity *e = entities()->get(otherName);

						if (!e->has(ShotComponent::component))
							continue;

						CAGE_COMPONENT_ENGINE(Transform, ot, e);
						vec3 toMonster = tr.position - ot.position;

						// test whether other is closer
						if (lengthSquared(toMonster) >= closestDistance * closestDistance)
							continue;

						DEGRID_COMPONENT(Velocity, ov, e);

						// test its direction
						if (dot(normalize(toMonster), normalize(ov.velocity)) < 0)
							continue;

						closestShot = otherName;
						closestDistance = length(toMonster);
					}
				}

				// avoidance
				if (closestShot)
				{
					Entity *s = entities()->get(closestShot);
					CAGE_COMPONENT_ENGINE(Transform, ot, s);
					DEGRID_COMPONENT(Velocity, ov, s);
					vec3 a = tr.position - ot.position;
					vec3 b = normalize(ov.velocity);
					vec3 avoid = normalize(a - dot(a, b) * b);
					will = interpolate(will, avoid, sm.avoidance);
				}

				mv.velocity += will * sm.acceleration;
				if (lengthSquared(mv.velocity) > sm.maxSpeed * sm.maxSpeed)
					mv.velocity = normalize(mv.velocity) * sm.maxSpeed;
			}

			CAGE_ASSERT(mv.velocity.valid(), mv.velocity, tr.position, tr.orientation, tr.scale);
		}
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}

Entity *initializeSimple(const vec3 &spawnPosition, const vec3 &color, real scale, uint32 objectName, uint32 deadSound, real damage, real life, real maxSpeed, real accelerationFraction, real avoidance, real dispersion, real circling, real spiraling, const quat &animation)
{
	Entity *e = initializeMonster(spawnPosition, color, scale, objectName, deadSound, damage, life);
	DEGRID_COMPONENT(Velocity, v, e);
	DEGRID_COMPONENT(Monster, m, e);
	DEGRID_COMPONENT(SimpleMonster, s, e);
	DEGRID_COMPONENT(Rotation, rot, e);
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

void spawnSimple(MonsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color)
{
	Entity *e = nullptr;
	uint32 special = 0;
	switch (type)
	{
	case MonsterTypeFlags::Circle:
		e = initializeSimple(spawnPosition, color, 2, HashString("degrid/monster/smallCircle.object"), HashString("degrid/monster/bum-circle.ogg"), 2, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 2, 0, 1, 0, 0.3, quat());
		break;
	case MonsterTypeFlags::SmallTriangle:
		e = initializeSimple(spawnPosition, color, 2.5, HashString("degrid/monster/smallTriangle.object"), HashString("degrid/monster/bum-triangle.ogg"), 3, 1 + monsterMutation(special), 0.4 + 0.1 * monsterMutation(special), 50, 0, 0.02, 0.8, 0.1, quat(degs(), randomAngle() / 50, degs()));
		break;
	case MonsterTypeFlags::SmallCube:
		e = initializeSimple(spawnPosition, color, 2.5, HashString("degrid/monster/smallCube.object"), HashString("degrid/monster/bum-cube.ogg"), 3, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 3, 1, 0.2, 0, 0.3, interpolate(quat(), randomDirectionQuat(), 0.01));
		break;
	case MonsterTypeFlags::LargeTriangle:
		e = initializeSimple(spawnPosition, color, 3, HashString("degrid/monster/largeTriangle.object"), HashString("degrid/monster/bum-triangle.ogg"), 4, 1 + monsterMutation(special), 0.4 + 0.1 * monsterMutation(special), 50, 0, 0.02, 0.1, 0.4, quat(degs(), randomAngle() / 50, degs()));
		{
			DEGRID_COMPONENT(Monster, m, e);
			m.defeatedCallback.bind<&spawnSmallTriangle>();
		}
		break;
	case MonsterTypeFlags::LargeCube:
		e = initializeSimple(spawnPosition, color, 3, HashString("degrid/monster/largeCube.object"), HashString("degrid/monster/bum-cube.ogg"), 4, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 3, 1, 0.2, 0, 0, interpolate(quat(), randomDirectionQuat(), 0.01));
		{
			DEGRID_COMPONENT(Monster, m, e);
			m.defeatedCallback.bind<&spawnSmallCube>();
		}
		break;
	case MonsterTypeFlags::PinWheel:
		e = initializeSimple(spawnPosition, color, 3.5, HashString("degrid/monster/pinWheel.object"), HashString("degrid/monster/bum-pinwheel.ogg"), 4, 1 + monsterMutation(special), 2 + 0.4 * monsterMutation(special), 50, 0, 0.005, 0, 0, quat(degs(), degs(20), degs()));
		break;
	case MonsterTypeFlags::Diamond:
		e = initializeSimple(spawnPosition, color, 3, HashString("degrid/monster/largeDiamond.object"), HashString("degrid/monster/bum-diamond.ogg"), 4, 1 + monsterMutation(special), 0.7 + 0.15 * monsterMutation(special), 1, 0.9, 0.2, 0, 0, interpolate(quat(), randomDirectionQuat(), 0.01));
		break;
	default: CAGE_THROW_CRITICAL(Exception, "invalid monster type");
	}
	monsterReflectMutation(e, special);
}
