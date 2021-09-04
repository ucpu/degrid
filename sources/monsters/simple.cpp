#include "monsters.h"

namespace
{
	void engineInit()
	{
		engineEntities()->defineComponent(SimpleMonsterComponent());
	}

	void spawnSmallCube(uint32 originalEntity)
	{
		Entity *e = engineEntities()->get(originalEntity);
		TransformComponent &t = e->value<TransformComponent>();
		RenderComponent &r = e->value<RenderComponent>();
		for (uint32 i = 0; i < 2; i++)
			spawnSimple(MonsterTypeFlags::SmallCube, t.position + Vec3(randomChance() - 0.5, 0, randomChance() - 0.5), r.color);
	}

	void spawnSmallTriangle(uint32 originalEntity)
	{
		Entity *e = engineEntities()->get(originalEntity);
		TransformComponent &t = e->value<TransformComponent>();
		RenderComponent &r = e->value<RenderComponent>();
		for (uint32 i = 0; i < 2; i++)
			spawnSimple(MonsterTypeFlags::SmallTriangle, t.position + Vec3(randomChance() - 0.5, 0, randomChance() - 0.5), r.color);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		for (Entity *e : engineEntities()->component<SimpleMonsterComponent>()->entities())
		{
			TransformComponent &tr = e->value<TransformComponent>();
			VelocityComponent &mv = e->value<VelocityComponent>();
			SimpleMonsterComponent &sm = e->value<SimpleMonsterComponent>();

			if (lengthSquared(mv.velocity) > sm.maxSpeed * sm.maxSpeed + 1e-4)
				mv.velocity = normalize(mv.velocity) * max(sm.maxSpeed, length(mv.velocity) - sm.acceleration);
			else
			{
				Vec3 will;

				// spiraling
				{
					Vec3 direct = normalize(game.monstersTarget - tr.position);
					Vec3 side = normalize(cross(direct, Vec3(0, 1, 0)));
					if ((e->name() % 13) == 0)
						side *= -1;
					will = interpolate(direct, side, sm.spiraling);
				}

				// circling
				{
					Rads ang = Real(e->name() % 2) * Rads::Full() * 0.5 + Real(((e->name() / 2) % 30) / 30.0) * Rads::Full() + Rads(engineControlTime() * 1e-6);
					Vec3 dir = Quat(Degs(), ang, Degs()) * Vec3(0, 0, -1);
					will = interpolate(will, dir, sm.circling);
				}

				// closest shot
				uint32 closestShot = 0;
				{
					Real closestDistance = Real::Infinity();
					uint32 myName = e->name();
					spatialSearchQuery->intersection(Sphere(tr.position, 15));
					for (uint32 otherName : spatialSearchQuery->result())
					{
						if (otherName == myName)
							continue;

						Entity *e = engineEntities()->get(otherName);

						if (!e->has<ShotComponent>())
							continue;

						TransformComponent &ot = e->value<TransformComponent>();
						Vec3 toMonster = tr.position - ot.position;

						// test whether other is closer
						if (lengthSquared(toMonster) >= closestDistance * closestDistance)
							continue;

						VelocityComponent &ov = e->value<VelocityComponent>();

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
					Entity *s = engineEntities()->get(closestShot);
					TransformComponent &ot = s->value<TransformComponent>();
					VelocityComponent &ov = s->value<VelocityComponent>();
					Vec3 a = tr.position - ot.position;
					Vec3 b = normalize(ov.velocity);
					Vec3 avoid = normalize(a - dot(a, b) * b);
					will = interpolate(will, avoid, sm.avoidance);
				}

				mv.velocity += will * sm.acceleration;
				if (lengthSquared(mv.velocity) > sm.maxSpeed * sm.maxSpeed)
					mv.velocity = normalize(mv.velocity) * sm.maxSpeed;
			}

			CAGE_ASSERT(mv.velocity.valid());
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

Entity *initializeSimple(const Vec3 &spawnPosition, const Vec3 &color, Real scale, uint32 objectName, uint32 deadSound, Real damage, Real life, Real maxSpeed, Real accelerationFraction, Real avoidance, Real dispersion, Real circling, Real spiraling, const Quat &animation)
{
	Entity *e = initializeMonster(spawnPosition, color, scale, objectName, deadSound, damage, life);
	VelocityComponent &v = e->value<VelocityComponent>();
	MonsterComponent &m = e->value<MonsterComponent>();
	SimpleMonsterComponent &s = e->value<SimpleMonsterComponent>();
	RotationComponent &rot = e->value<RotationComponent>();
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

void spawnSimple(MonsterTypeFlags type, const Vec3 &spawnPosition, const Vec3 &color)
{
	Entity *e = nullptr;
	uint32 special = 0;
	switch (type)
	{
	case MonsterTypeFlags::Circle:
		e = initializeSimple(spawnPosition, color, 2, HashString("degrid/monster/smallCircle.object"), HashString("degrid/monster/bum-circle.ogg"), 2, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 2, 0, 1, 0, 0.3, Quat());
		break;
	case MonsterTypeFlags::SmallTriangle:
		e = initializeSimple(spawnPosition, color, 2.5, HashString("degrid/monster/smallTriangle.object"), HashString("degrid/monster/bum-triangle.ogg"), 3, 1 + monsterMutation(special), 0.4 + 0.1 * monsterMutation(special), 50, 0, 0.02, 0.8, 0.1, Quat(Degs(), randomAngle() / 50, Degs()));
		break;
	case MonsterTypeFlags::SmallCube:
		e = initializeSimple(spawnPosition, color, 2.5, HashString("degrid/monster/smallCube.object"), HashString("degrid/monster/bum-cube.ogg"), 3, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 3, 1, 0.2, 0, 0.3, interpolate(Quat(), randomDirectionQuat(), 0.01));
		break;
	case MonsterTypeFlags::LargeTriangle:
		e = initializeSimple(spawnPosition, color, 3, HashString("degrid/monster/largeTriangle.object"), HashString("degrid/monster/bum-triangle.ogg"), 4, 1 + monsterMutation(special), 0.4 + 0.1 * monsterMutation(special), 50, 0, 0.02, 0.1, 0.4, Quat(Degs(), randomAngle() / 50, Degs()));
		{
			MonsterComponent &m = e->value<MonsterComponent>();
			m.defeatedCallback.bind<&spawnSmallTriangle>();
		}
		break;
	case MonsterTypeFlags::LargeCube:
		e = initializeSimple(spawnPosition, color, 3, HashString("degrid/monster/largeCube.object"), HashString("degrid/monster/bum-cube.ogg"), 4, 1 + monsterMutation(special), 0.3 + 0.1 * monsterMutation(special), 3, 1, 0.2, 0, 0, interpolate(Quat(), randomDirectionQuat(), 0.01));
		{
			MonsterComponent &m = e->value<MonsterComponent>();
			m.defeatedCallback.bind<&spawnSmallCube>();
		}
		break;
	case MonsterTypeFlags::PinWheel:
		e = initializeSimple(spawnPosition, color, 3.5, HashString("degrid/monster/pinWheel.object"), HashString("degrid/monster/bum-pinwheel.ogg"), 4, 1 + monsterMutation(special), 2 + 0.4 * monsterMutation(special), 50, 0, 0.005, 0, 0, Quat(Degs(), Degs(20), Degs()));
		break;
	case MonsterTypeFlags::Diamond:
		e = initializeSimple(spawnPosition, color, 3, HashString("degrid/monster/largeDiamond.object"), HashString("degrid/monster/bum-diamond.ogg"), 4, 1 + monsterMutation(special), 0.7 + 0.15 * monsterMutation(special), 1, 0.9, 0.2, 0, 0, interpolate(Quat(), randomDirectionQuat(), 0.01));
		break;
	default: CAGE_THROW_CRITICAL(Exception, "invalid monster type");
	}
	monsterReflectMutation(e, special);
}
