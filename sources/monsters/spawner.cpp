#include <vector>

#include "monsters.h"

#include <cage-core/color.h>

namespace
{
	struct spawnerComponent
	{
		static entityComponent *component;
		uint32 count;
		uint32 period;
		monsterTypeFlags type;

		spawnerComponent() : count(0), period(0), type(monsterTypeFlags::None)
		{}
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
			DEGRID_COMPONENT(spawner, s, e);
			if (s.count == 0)
			{
				DEGRID_COMPONENT(monster, m, e);
				killMonster(e, false);
			}
			else if ((statistics.updateIteration % s.period) == 0)
			{
				CAGE_COMPONENT_ENGINE(transform, t, e);
				CAGE_COMPONENT_ENGINE(render, r, e);
				spawnGeneral(s.type, t.position, r.color);
				s.count--;
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

	const monsterTypeFlags types[bossesTotalCount + 1] = {
		monsterTypeFlags::Circle | monsterTypeFlags::SmallCube | monsterTypeFlags::SmallTriangle,
		monsterTypeFlags::SmallCube | monsterTypeFlags::SmallTriangle | monsterTypeFlags::LargeCube | monsterTypeFlags::LargeTriangle | monsterTypeFlags::Diamond,
		monsterTypeFlags::LargeCube | monsterTypeFlags::LargeTriangle | monsterTypeFlags::Diamond | monsterTypeFlags::PinWheel | monsterTypeFlags::Shielder,
		monsterTypeFlags::Diamond | monsterTypeFlags::PinWheel | monsterTypeFlags::Shielder | monsterTypeFlags::Shocker | monsterTypeFlags::Rocket,
		monsterTypeFlags::Diamond | monsterTypeFlags::PinWheel | monsterTypeFlags::Shielder | monsterTypeFlags::Shocker | monsterTypeFlags::Rocket,
		monsterTypeFlags::Diamond | monsterTypeFlags::PinWheel | monsterTypeFlags::Shielder | monsterTypeFlags::Shocker | monsterTypeFlags::Rocket,
	};

	monsterTypeFlags pickOne(monsterTypeFlags spawnTypes)
	{
		std::vector<monsterTypeFlags> allowed;
		allowed.reserve(32);
		{
			uint32 bit = 1;
			for (uint32 i = 0; i < 32; i++)
			{
				if ((spawnTypes & (monsterTypeFlags)bit) == (monsterTypeFlags)bit)
					allowed.push_back((monsterTypeFlags)bit);
				bit <<= 1;
			}
		}
		uint32 alSiz = numeric_cast<uint32>(allowed.size());
		CAGE_ASSERT(alSiz > 0);
		return allowed[randomRange(0u, alSiz)];
	}
}

void spawnSpawner(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	entity *spawner = initializeMonster(spawnPosition, color, 4, hashString("degrid/monster/spawner.object"), hashString("degrid/monster/bum-spawner.ogg"), 10, 20 + 5 * monsterMutation(special));
	CAGE_COMPONENT_ENGINE(transform, transform, spawner);
	transform.orientation = randomDirectionQuat();
	CAGE_COMPONENT_ENGINE(skeletalAnimation, sa, spawner);
	sa.startTime = getApplicationTime();
	DEGRID_COMPONENT(monster, m, spawner);
	DEGRID_COMPONENT(spawner, s, spawner);
	s.type = pickOne(types[game.defeatedBosses]);
	s.count = 60 + 15 * monsterMutation(special);
	s.period = numeric_cast<uint32>(25.0 / (3 + monsterMutation(special))) + 1;
	DEGRID_COMPONENT(rotation, rotation, spawner);
	rotation.rotation = interpolate(quat(), randomDirectionQuat(), 0.003);
	monsterReflectMutation(spawner, special);
}

