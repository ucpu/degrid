#include <cage-core/color.h>

#include "monsters.h"

#include <vector>

namespace
{
	struct SpawnerComponent
	{
		static EntityComponent *component;
		uint32 count = 0;
		uint32 period = 0;
		MonsterTypeFlags type = MonsterTypeFlags::None;
	};

	EntityComponent *SpawnerComponent::component;

	void engineInit()
	{
		SpawnerComponent::component = engineEntities()->defineComponent(SpawnerComponent());
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		for (Entity *e : SpawnerComponent::component->entities())
		{
			SpawnerComponent &s = e->value<SpawnerComponent>();
			if (s.count == 0)
			{
				MonsterComponent &m = e->value<MonsterComponent>();
				killMonster(e, false);
			}
			else if ((statistics.updateIteration % s.period) == 0)
			{
				TransformComponent &t = e->value<TransformComponent>();
				RenderComponent &r = e->value<RenderComponent>();
				spawnGeneral(s.type, t.position, r.color);
				s.count--;
			}
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
		uint32 alSiz = numeric_cast<uint32>(allowed.size());
		CAGE_ASSERT(alSiz > 0);
		return allowed[randomRange(0u, alSiz)];
	}
}

void spawnSpawner(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	Entity *spawner = initializeMonster(spawnPosition, color, 6, HashString("degrid/monster/spawner.object"), HashString("degrid/monster/bum-spawner.ogg"), 10, 20 + 5 * monsterMutation(special));
	TransformComponent &transform = spawner->value<TransformComponent>();
	transform.orientation = randomDirectionQuat();
	SkeletalAnimationComponent &sa = spawner->value<SkeletalAnimationComponent>();
	sa.startTime = applicationTime();
	MonsterComponent &m = spawner->value<MonsterComponent>();
	SpawnerComponent &s = spawner->value<SpawnerComponent>();
	s.type = pickOne(Types[game.defeatedBosses]);
	s.count = 60 + 15 * monsterMutation(special);
	s.period = numeric_cast<uint32>(25.0 / (3 + monsterMutation(special))) + 1;
	RotationComponent &rotation = spawner->value<RotationComponent>();
	rotation.rotation = interpolate(quat(), randomDirectionQuat(), 0.003);
	monsterReflectMutation(spawner, special);
}

