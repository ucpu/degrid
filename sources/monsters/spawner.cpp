#include <cage-core/color.h>
#include <cage-core/entitiesVisitor.h>

#include "monsters.h"

#include <vector>

namespace
{
	struct SpawnerComponent
	{
		uint32 count = 0;
		uint32 period = 0;
		MonsterTypeFlags type = MonsterTypeFlags::None;
	};

	void engineInit()
	{
		engineEntities()->defineComponent(SpawnerComponent());
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		entitiesVisitor([&](Entity *e, SpawnerComponent &s) {
			if (s.count == 0)
			{
				MonsterComponent &m = e->value<MonsterComponent>();
				killMonster(e, false);
			}
			else if ((statistics.updateIteration % s.period) == (e->name() % s.period))
			{
				spawnGeneral(s.type, e->value<TransformComponent>().position, e->value<RenderComponent>().color);
				s.count--;
			}
		}, engineEntities(), false);
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

	constexpr const MonsterTypeFlags Types[BossesTotalCount + 1] = {
		MonsterTypeFlags::Circle | MonsterTypeFlags::SmallCube | MonsterTypeFlags::SmallTriangle,
		MonsterTypeFlags::SmallCube | MonsterTypeFlags::SmallTriangle | MonsterTypeFlags::LargeCube | MonsterTypeFlags::LargeTriangle | MonsterTypeFlags::Diamond,
		MonsterTypeFlags::LargeCube | MonsterTypeFlags::LargeTriangle | MonsterTypeFlags::Diamond | MonsterTypeFlags::PinWheel | MonsterTypeFlags::Shielder,
		MonsterTypeFlags::Diamond | MonsterTypeFlags::PinWheel | MonsterTypeFlags::Shielder | MonsterTypeFlags::Shocker | MonsterTypeFlags::Rocket,
		MonsterTypeFlags::Diamond | MonsterTypeFlags::PinWheel | MonsterTypeFlags::Shielder | MonsterTypeFlags::Shocker | MonsterTypeFlags::Rocket,
		MonsterTypeFlags::Diamond | MonsterTypeFlags::PinWheel | MonsterTypeFlags::Shielder | MonsterTypeFlags::Shocker | MonsterTypeFlags::Rocket,
	};

	MonsterTypeFlags pickOne(MonsterTypeFlags spawnTypes)
	{
		std::vector<MonsterTypeFlags> allowed;
		allowed.reserve(32);
		{
			uint32 bit = 1;
			for (uint32 i = 0; i < 32; i++)
			{
				if ((spawnTypes & (MonsterTypeFlags)bit) == (MonsterTypeFlags)bit)
					allowed.push_back((MonsterTypeFlags)bit);
				bit <<= 1;
			}
		}
		const uint32 alSiz = numeric_cast<uint32>(allowed.size());
		CAGE_ASSERT(alSiz > 0);
		return allowed[randomRange(0u, alSiz)];
	}
}

void spawnSpawner(const Vec3 &spawnPosition, const Vec3 &color)
{
	uint32 special = 0;
	Entity *spawner = initializeMonster(spawnPosition, color, 6, HashString("degrid/monster/spawner.object"), HashString("degrid/monster/bum-spawner.ogg"), 10, 20 + 5 * monsterMutation(special));
	spawner->value<TransformComponent>().orientation = randomDirectionQuat();
	spawner->value<SkeletalAnimationComponent>().startTime = applicationTime();
	spawner->value<MonsterComponent>();
	SpawnerComponent &s = spawner->value<SpawnerComponent>();
	s.type = pickOne(Types[game.defeatedBosses]);
	s.count = 60 + 15 * monsterMutation(special);
	s.period = numeric_cast<uint32>(25.0 / (3 + monsterMutation(special))) + 1;
	spawner->value<RotationComponent>().rotation = interpolate(Quat(), randomDirectionQuat(), 0.003);
	monsterReflectMutation(spawner, special);
}

