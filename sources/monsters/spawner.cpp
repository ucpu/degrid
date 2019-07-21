#include <vector>

#include "monsters.h"

#include <cage-core/color.h>

namespace
{
	struct spawnerComponent
	{
		static entityComponent *component;
	};

	entityComponent *spawnerComponent::component;

	void engineInit()
	{
		spawnerComponent::component = entities()->defineComponent(spawnerComponent(), true);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("spawner");

		if (game.paused)
			return;

		for (entity *e : spawnerComponent::component->entities())
		{
			// todo
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

void spawnSpawner(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	entity *spawner = initializeMonster(spawnPosition, color, 4, hashString("degrid/monster/spawner.object"), hashString("degrid/monster/bum-spawner.ogg"), 10, 20);
	CAGE_COMPONENT_ENGINE(transform, transform, spawner);
	transform.orientation = randomDirectionQuat();
	DEGRID_COMPONENT(monster, m, spawner);
	m.dispersion = 0.001;
	DEGRID_COMPONENT(spawner, sp, spawner);
	DEGRID_COMPONENT(rotation, rotation, spawner);
	rotation.rotation = interpolate(quat(), randomDirectionQuat(), 0.01);
	monsterReflectMutation(spawner, special);
}

