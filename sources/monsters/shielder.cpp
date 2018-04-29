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
			uint32 count = shielderStruct::component->getComponentEntities()->entitiesCount();
			entityClass *const *monsters = shielderStruct::component->getComponentEntities()->entitiesArray();
			for (uint32 i = 0; i < count; i++)
			{
				entityClass *e = monsters[i];
				ENGINE_GET_COMPONENT(transform, tr, e);
				GRID_GET_COMPONENT(monster, ms, e);
				GRID_GET_COMPONENT(shielder, sh, e);
				entityClass *se = entities()->getEntity(sh.shieldEntity);
				ENGINE_GET_COMPONENT(transform, tre, se);
				GRID_GET_COMPONENT(shield, sse, se);

				vec3 t = normalize(player.monstersTarget - tr.position);
				vec3 f = tr.orientation * vec3(0, 0, -1);
				real df = dot(t, f);
				if (df > 0.97)
				{
					ms.speed = f * sh.movementSpeed;
				}
				else
				{
					ms.speed = vec3();
					vec3 s = tr.orientation * vec3(1, 0, 0);
					tr.orientation = tr.orientation * quat(degs(), sh.turningSpeed * -sign(dot(t, s)), degs());
				}

				sse.active = df > 0.92;
				tre = tr;
			}
		}
		{ // update shields
			uint32 count = shieldStruct::component->getComponentEntities()->entitiesCount();
			entityClass *const *monsters = shieldStruct::component->getComponentEntities()->entitiesArray();
			for (uint32 i = 0; i < count; i++)
			{
				entityClass *e = monsters[i];
				shieldUpdateStruct u(e);
			}
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
			sh.turningSpeed = degs(0.5 + 0.3 * spawnSpecial(special));
			sh.movementSpeed = 0.6 + 0.15 * spawnSpecial(special);
			if (special > 0)
			{
				ENGINE_GET_COMPONENT(transform, transform, shielder);
				transform.scale *= 1.2;
				statistics.monstersSpecial++;
			}
			GRID_GET_COMPONENT(monster, m, shielder);
			m.dispersion = 0.2;
		}
		{
			ENGINE_GET_COMPONENT(transform, transform, shield);
			transform.position = spawnPosition;
			GRID_GET_COMPONENT(shield, sh, shield);
			sh.active = false;
		}
	}
}
