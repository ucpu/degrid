#include "monsters.h"

namespace grid
{
	namespace
	{
		struct shieldUpdateStruct
		{
			transformComponent &tr;
			shieldStruct &sh;
			uint32 myName;
			vec3 forward;

			shieldUpdateStruct(entityClass *e) :
				tr(e->value<transformComponent>(transformComponent::component)),
				sh(e->value<shieldStruct>(shieldStruct::component)),
				myName(e->getName()), forward(tr.orientation * vec3(0, 0, -1))
			{
				if (sh.active)
				{
					ENGINE_GET_COMPONENT(render, render, e);
					render.object = hashString("grid/monster/shield.object");
				}
				else
				{
					e->removeComponent(renderComponent::component);
					return;
				}

				spatialQuery->intersection(sphere(tr.position + tr.orientation * vec3(0, 0, -1) * (tr.scale + 3), 5));
				const uint32 *res = spatialQuery->resultArray();
				for (uint32 i = 0, e = spatialQuery->resultCount(); i != e; i++)
					test(res[i]);
			}

			void test(uint32 otherName)
			{
				if (otherName == myName || !entities()->hasEntity(otherName))
					return;
				entityClass *e = entities()->getEntity(otherName);
				if (!e->hasComponent(shotStruct::component))
					return;
				ENGINE_GET_COMPONENT(transform, ot, e);
				GRID_GET_COMPONENT(shot, os, e);
				(void)os;
				vec3 toShot = ot.position - tr.position;
				real lenShot = toShot.length();
				if (lenShot < tr.scale + 1 || lenShot > tr.scale + 5)
					return;
				vec3 dirShot = toShot.normalize();
				if (dirShot.dot(forward) < cos(degs(45)))
					return;
				e->addGroup(entitiesToDestroy);
				statistics.shielderStoppedShots++;
				shotExplosion(e);
			}
		};
	}

	void updateShielder()
	{
		{ // update shielders
			for (entityClass *e : shielderStruct::component->getComponentEntities()->entities())
			{
				ENGINE_GET_COMPONENT(transform, tr, e);
				GRID_GET_COMPONENT(monster, ms, e);
				GRID_GET_COMPONENT(shielder, sh, e);
				entityClass *se = entities()->getEntity(sh.shieldEntity);
				ENGINE_GET_COMPONENT(transform, tre, se);
				GRID_GET_COMPONENT(shield, sse, se);

				if (sse.active)
				{ // charging
					if (sh.stepsLeft)
					{
						vec3 t = normalize(player.monstersTarget - tr.position);
						tr.orientation = interpolate(tr.orientation, quat(t, vec3(0, 1, 0)), 0.02);
						vec3 f = tr.orientation * vec3(0, 0, -1);
						ms.speed = f * sh.movementSpeed;
					}
					else
					{
						sse.active = false;
						sh.stepsLeft = sh.turningSteps;
					}
				}
				else
				{ // turning
					ms.speed = vec3();
					if (sh.stepsLeft)
					{
						vec3 t = normalize(player.monstersTarget - tr.position);
						tr.orientation = interpolate(tr.orientation, quat(t, vec3(0, 1, 0)), 0.95 / sh.stepsLeft);
					}
					else
					{
						sse.active = true;
						sh.stepsLeft = sh.chargingSteps;
					}
				}

				CAGE_ASSERT_RUNTIME(sh.stepsLeft > 0);
				sh.stepsLeft--;
				tre = tr;
			}
		}
		{ // update shields
			for (entityClass *e : shieldStruct::component->getComponentEntities()->entities())
				shieldUpdateStruct u(e);
		}
	}

	void spawnShielder(const vec3 &spawnPosition, const vec3 &color)
	{
		uint32 special = 0;
		entityClass *shielder = initializeMonster(spawnPosition, color, 3, hashString("grid/monster/shielder.object"), hashString("grid/monster/bum-shielder.ogg"), 5, 3 + spawnSpecial(special));
		entityClass *shield = entities()->newUniqueEntity();
		{
			GRID_GET_COMPONENT(shielder, sh, shielder);
			sh.shieldEntity = shield->getName();
			sh.movementSpeed = 0.8 + 0.2 * spawnSpecial(special);
			sh.turningSteps = random(20u, 30u);
			sh.chargingSteps = random(60u, 180u);
			sh.stepsLeft = sh.turningSteps;
			GRID_GET_COMPONENT(monster, m, shielder);
			m.dispersion = 0.2;
			if (special > 0)
			{
				ENGINE_GET_COMPONENT(transform, transform, shielder);
				transform.scale *= 1.2;
				statistics.monstersSpecial++;
			}
		}
		{
			ENGINE_GET_COMPONENT(transform, transform, shield);
			transform.position = spawnPosition;
			GRID_GET_COMPONENT(shield, sh, shield);
			sh.active = false;
		}
	}
}
