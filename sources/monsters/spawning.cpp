#include <cage-core/color.h>

#include "monsters.h"

#include <algorithm>
#include <vector>

namespace
{
	Vec3 aroundPosition(Real index, Real radius, Vec3 center)
	{
		const Rads angle = index * Rads::Full();
		const Vec3 dir = Vec3(cos(angle), 0, sin(angle));
		return center + dir * radius;
	}

	enum class PlacingPolicyEnum
	{
		Random,
		Around,
		Grouped,
		Line,
	};

	struct SpawnDefinition
	{
		// spawned monsters
		uint32 spawnCountMin = 1, spawnCountMax = 1;
		Real distanceMin = 200, distanceMax = 250;
		MonsterTypeFlags spawnTypes = MonsterTypeFlags::None;
		PlacingPolicyEnum placingPolicy = PlacingPolicyEnum::Random;

		// priority
		Real priorityCurrent; 
		Real priorityChange;
		Real priorityAdditive;
		Real priorityMultiplier = 1;

		// statistics
		uint32 iteration = 0;
		uint32 spawned = 0;
		String name;

		explicit SpawnDefinition(const String &name);
		bool operator < (const SpawnDefinition &other) const { return priorityCurrent < other.priorityCurrent; }
		void perform();
		void performSimulation();
		void updatePriority();
		void spawn();
	};

	SpawnDefinition::SpawnDefinition(const String &name) : name(name)
	{}

	uint32 monstersLimit()
	{
		if (game.cinematic)
			return 100;
		return 150 + 20 * game.defeatedBosses;
	}

	void SpawnDefinition::perform()
	{
		spawn();
		updatePriority();
		statistics.monstersCurrentSpawningPriority = priorityCurrent;
	}

	void SpawnDefinition::updatePriority()
	{
		iteration++;
		priorityCurrent += max(priorityChange, 1e-5);
		priorityChange += priorityAdditive;
		priorityChange *= priorityMultiplier;
	}

	void SpawnDefinition::performSimulation()
	{
		updatePriority();
		if (any(spawnTypes & MonsterTypeFlags::BossEgg))
			game.defeatedBosses++;
	}

	void SpawnDefinition::spawn()
	{
		CAGE_ASSERT(spawnCountMin <= spawnCountMax && spawnCountMin > 0);
		CAGE_ASSERT(distanceMin <= distanceMax && distanceMin > 0);
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
		const uint32 spawnCount = randomRange(spawnCountMin, spawnCountMax + 1);
		spawned += spawnCount;
		const Vec3 color = colorHsvToRgb(Vec3(randomChance(), sqrt(randomChance()) * 0.5 + 0.5, sqrt(randomChance()) * 0.5 + 0.5));
		const Vec3 playerPosition = game.playerEntity->value<TransformComponent>().position;
		switch (placingPolicy)
		{
		case PlacingPolicyEnum::Random:
		{
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[randomRange(0u, alSiz)], aroundPosition(randomChance(), randomRange(distanceMin, distanceMax), playerPosition), color);
		} break;
		case PlacingPolicyEnum::Around:
		{
			const Real angularOffset = randomChance();
			const Real radius = randomRange(distanceMin, distanceMax);
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[randomRange(0u, alSiz)], aroundPosition(angularOffset + (randomChance() * 0.3 + i) / (Real)spawnCount, radius, playerPosition), color);
		} break;
		case PlacingPolicyEnum::Grouped:
		{
			const Real radius = (distanceMax - distanceMin) * 0.5;
			const Vec3 center = aroundPosition(randomChance(), distanceMin + radius, playerPosition);
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[randomRange(0u, alSiz)], aroundPosition((Real)i / (Real)spawnCount, radius, center), color);
		} break;
		case PlacingPolicyEnum::Line:
		{
			Vec3 origin = aroundPosition(randomChance(), randomRange(distanceMin, distanceMax), playerPosition);
			const Vec3 span = cross(normalize(origin - playerPosition), Vec3(0, 1, 0)) * 10;
			origin -= span * spawnCount * 0.5;
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[randomRange(0u, alSiz)], origin + i * span, color);
		} break;
		default: CAGE_THROW_CRITICAL(Exception, "invalid placing policy");
		}
	}

	std::vector<SpawnDefinition> definitions;

	const auto engineUpdateListener = controlThread().update.listen([]() {
		if (game.paused)
			return;

		if (engineEntities()->component<BossComponent>()->count() > 0)
			return;

		const uint32 limit = monstersLimit();
		Real probability = 1.f - (Real)statistics.monstersCurrent / (Real)limit;
		if (probability <= 0)
			return;

		probability = pow(100, probability - 1);
		if (randomChance() > probability)
			return;

		definitions[0].perform();
		std::nth_element(definitions.begin(), definitions.begin(), definitions.end());
	});

	void announceJokeMap()
	{
		game.jokeMap = true;
		makeAnnouncement(HashString("announcement/joke-map"), HashString("announcement-desc/joke-map"), 120 * 30);
	}

	const auto gameStartListener = gameStartEvent().listen([]() {
		definitions.clear();
		definitions.reserve(30);

		if (game.cinematic)
		{ // cinematic spawns
			SpawnDefinition d("cinematic");
			d.spawnTypes = (MonsterTypeFlags::Circle | MonsterTypeFlags::SmallTriangle | MonsterTypeFlags::SmallCube | MonsterTypeFlags::LargeTriangle | MonsterTypeFlags::LargeCube);
			definitions.push_back(d);
			return;
		}

		///////////////////////////////////////////////////////////////////////////
		// jokes
		///////////////////////////////////////////////////////////////////////////

		if (randomRange(0u, 1000u) == 42)
		{
			CAGE_LOG(SeverityEnum::Info, "joke", "JOKE: snakes only map");
			SpawnDefinition d("snakes only");
			d.spawnTypes = (MonsterTypeFlags::Snake);
			definitions.push_back(d);
			announceJokeMap();
			return;
		}

		// todo monkeys joke

		///////////////////////////////////////////////////////////////////////////
		// individually spawned monsters
		///////////////////////////////////////////////////////////////////////////

		monstersSpawnInitial();

		{ // small monsters individually
			SpawnDefinition d("individual small monsters");
			d.spawnTypes = (MonsterTypeFlags::Circle | MonsterTypeFlags::SmallTriangle | MonsterTypeFlags::SmallCube);
			d.priorityCurrent = 0;
			d.priorityChange = 1;
			d.priorityAdditive = 0.001;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // large monsters individually
			SpawnDefinition d("individual large monsters");
			d.spawnTypes = (MonsterTypeFlags::LargeTriangle | MonsterTypeFlags::LargeCube);
			d.priorityCurrent = 50;
			d.priorityChange = 1;
			d.priorityAdditive = 0.001;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // pin wheels individually
			SpawnDefinition d("individual pinwheels");
			d.spawnTypes = (MonsterTypeFlags::PinWheel);
			d.priorityCurrent = randomRange(200, 400);
			d.priorityChange = 27;
			d.priorityAdditive = 0.01;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // diamonds individually
			SpawnDefinition d("individual diamonds");
			d.spawnTypes = (MonsterTypeFlags::Diamond);
			d.priorityCurrent = randomRange(300, 500);
			d.priorityChange = 42;
			d.priorityAdditive = 0.1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // snakes individually
			SpawnDefinition d("individual snakes");
			d.spawnTypes = (MonsterTypeFlags::Snake);
			d.priorityCurrent = randomRange(1000, 2500);
			d.priorityChange = 133;
			d.priorityAdditive = 1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // shielders individually
			SpawnDefinition d("individual shielders");
			d.spawnTypes = (MonsterTypeFlags::Shielder);
			d.priorityCurrent = randomRange(1000, 2500);
			d.priorityChange = 127;
			d.priorityAdditive = 1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // shockers individually
			SpawnDefinition d("individual shockers");
			d.spawnTypes = (MonsterTypeFlags::Shocker);
			d.priorityCurrent = randomRange(2500, 3500);
			d.priorityChange = 143;
			d.priorityAdditive = 1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // rockets individually
			SpawnDefinition d("individual rockets");
			d.spawnTypes = (MonsterTypeFlags::Rocket);
			d.priorityCurrent = randomRange(2500, 3500);
			d.priorityChange = 111;
			d.priorityAdditive = 1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // spawners individually
			SpawnDefinition d("individual spawners");
			d.spawnTypes = (MonsterTypeFlags::Spawner);
			d.priorityCurrent = randomRange(2500, 3500);
			d.priorityChange = 122;
			d.priorityAdditive = 1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// groups
		///////////////////////////////////////////////////////////////////////////

		{ // circle groups
			SpawnDefinition d("circle groups");
			d.spawnCountMin = 10;
			d.spawnCountMax = 20;
			d.spawnTypes = (MonsterTypeFlags::Circle);
			d.placingPolicy = PlacingPolicyEnum::Grouped;
			d.priorityCurrent = randomRange(400, 600);
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(1, 10);
			definitions.push_back(d);
		}

		{ // large triangle groups
			SpawnDefinition d("large triangle groups");
			d.spawnCountMin = 5;
			d.spawnCountMax = 15;
			d.spawnTypes = (MonsterTypeFlags::LargeTriangle);
			d.placingPolicy = PlacingPolicyEnum::Grouped;
			d.priorityCurrent = randomRange(1000, 1500);
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(1, 10);
			definitions.push_back(d);
		}

		{ // large cube groups
			SpawnDefinition d("large cube groups");
			d.spawnCountMin = 5;
			d.spawnCountMax = 15;
			d.spawnTypes = (MonsterTypeFlags::LargeCube);
			d.placingPolicy = PlacingPolicyEnum::Grouped;
			d.priorityCurrent = randomRange(1000, 1500);
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(1, 10);
			definitions.push_back(d);
		}

		{ // pin wheel groups
			SpawnDefinition d("pinwheel groups");
			d.spawnCountMin = 2;
			d.spawnCountMax = 4;
			d.spawnTypes = (MonsterTypeFlags::PinWheel);
			d.placingPolicy = PlacingPolicyEnum::Grouped;
			d.priorityCurrent = randomRange(2000, 3000);
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(5, 15);
			definitions.push_back(d);
		}

		{ // diamond groups
			SpawnDefinition d("diamond groups");
			d.spawnCountMin = 3;
			d.spawnCountMax = 7;
			d.spawnTypes = (MonsterTypeFlags::Diamond);
			d.placingPolicy = PlacingPolicyEnum::Grouped;
			d.priorityCurrent = randomRange(2000, 3000);
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(5, 15);
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// formations (after first boss)
		///////////////////////////////////////////////////////////////////////////

		{ // mixed group
			SpawnDefinition d("mixed group");
			d.spawnCountMin = 20;
			d.spawnCountMax = 25;
			d.spawnTypes = (MonsterTypeFlags::LargeTriangle | MonsterTypeFlags::LargeCube | MonsterTypeFlags::Diamond);
			d.placingPolicy = PlacingPolicyEnum::Grouped;
			d.priorityCurrent = randomRange(4000, 6000);
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(5, 15);
			definitions.push_back(d);
		}

		{ // mixed around
			SpawnDefinition d("mixed around");
			d.spawnCountMin = 5;
			d.spawnCountMax = 10;
			d.spawnTypes = (MonsterTypeFlags::PinWheel | MonsterTypeFlags::Snake | MonsterTypeFlags::Shielder | MonsterTypeFlags::Shocker | MonsterTypeFlags::Rocket);
			d.placingPolicy = PlacingPolicyEnum::Around;
			d.distanceMin = 160;
			d.distanceMax = 190;
			d.priorityCurrent = randomRange(5000, 7000);
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(5, 15);
			definitions.push_back(d);
		}

		{ // rockets wall
			SpawnDefinition d("rockets wall");
			d.spawnCountMin = 10;
			d.spawnCountMax = 15;
			d.spawnTypes = (MonsterTypeFlags::Rocket);
			d.placingPolicy = PlacingPolicyEnum::Line;
			d.priorityCurrent = randomRange(5000, 7000);
			d.priorityChange = randomRange(800, 1200);
			d.priorityAdditive = randomRange(20, 60);
			definitions.push_back(d);
		}

		{ // shielders wall
			SpawnDefinition d("shielders wall");
			d.spawnCountMin = 10;
			d.spawnCountMax = 15;
			d.spawnTypes = (MonsterTypeFlags::Shielder);
			d.placingPolicy = PlacingPolicyEnum::Line;
			d.priorityCurrent = randomRange(6000, 8000);
			d.priorityChange = randomRange(800, 1200);
			d.priorityAdditive = randomRange(20, 60);
			definitions.push_back(d);
		}

		{ // rockets circle
			SpawnDefinition d("rockets circle");
			d.spawnCountMin = 30;
			d.spawnCountMax = 50;
			d.spawnTypes = (MonsterTypeFlags::Rocket);
			d.placingPolicy = PlacingPolicyEnum::Around;
			d.priorityCurrent = randomRange(7000, 9000);
			d.priorityChange = randomRange(800, 1200);
			d.priorityAdditive = randomRange(20, 60);
			definitions.push_back(d);
		}

		{ // saturated simple
			SpawnDefinition d("saturated circles");
			d.spawnCountMin = 100;
			d.spawnCountMax = 200;
			d.spawnTypes = (MonsterTypeFlags::Circle | MonsterTypeFlags::SmallCube | MonsterTypeFlags::SmallTriangle);
			d.placingPolicy = PlacingPolicyEnum::Random;
			d.distanceMin = 120;
			d.distanceMax = 180;
			d.priorityCurrent = randomRange(7000, 9000);
			d.priorityChange = randomRange(1000, 1500);
			d.priorityAdditive = randomRange(25, 75);
			definitions.push_back(d);
		}

		{ // wormholes
			SpawnDefinition d("wormholes");
			d.spawnCountMin = 1;
			d.spawnCountMax = 3;
			d.spawnTypes = (MonsterTypeFlags::Wormhole);
			d.placingPolicy = PlacingPolicyEnum::Around;
			d.distanceMin = 160;
			d.distanceMax = 190;
			d.priorityCurrent = randomRange(5000, 7000);
			d.priorityChange = randomRange(4000, 6000);
			d.priorityAdditive = randomRange(100, 300);
			definitions.push_back(d);
		}

		{ // spawners
			SpawnDefinition d("spawners");
			d.spawnCountMin = 2;
			d.spawnCountMax = 4;
			d.spawnTypes = (MonsterTypeFlags::Spawner);
			d.placingPolicy = PlacingPolicyEnum::Around;
			d.distanceMin = 160;
			d.distanceMax = 190;
			d.priorityCurrent = randomRange(6000, 8000);
			d.priorityChange = randomRange(1000, 1500);
			d.priorityAdditive = randomRange(25, 75);
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// bosses
		///////////////////////////////////////////////////////////////////////////

		{ // bosses
			SpawnDefinition d("bosses");
			d.spawnCountMin = 1;
			d.spawnCountMax = 1;
			d.spawnTypes = (MonsterTypeFlags::BossEgg);
			d.placingPolicy = PlacingPolicyEnum::Around;
			d.distanceMin = 300;
			d.distanceMax = 350;
#if 0
			d.priorityChange = 1;
#else
			d.priorityCurrent = 4000;
			d.priorityChange = 10000;
			d.priorityMultiplier = 1.5;
#endif
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// testing
		///////////////////////////////////////////////////////////////////////////

#if 0
		std::nth_element(definitions.begin(), definitions.begin(), definitions.end());
		while (definitions[0].priorityCurrent < 50000)
		{
			definitions[0].performSimulation();
			std::nth_element(definitions.begin(), definitions.begin(), definitions.end());
		}
#endif
	});

	const auto gameStopListener = gameStopEvent().listen([]() {
#ifdef DEGRID_TESTING
		for (auto &d : definitions)
		{
			if (d.iteration > 0)
				CAGE_LOG(SeverityEnum::Info, "statistics", Stringizer() + "spawn '" + d.name + "', iteration: " + d.iteration + ", spawned: " + d.spawned + ", priority: " + d.priorityCurrent + ", change: " + d.priorityChange);
		}
#endif // DEGRID_TESTING

		definitions.clear();
	});
}

void spawnGeneral(MonsterTypeFlags type, const Vec3 &spawnPosition, const Vec3 &color)
{
	switch (type)
	{
	case MonsterTypeFlags::Snake: return spawnSnake(spawnPosition, color);
	case MonsterTypeFlags::Shielder: return spawnShielder(spawnPosition, color);
	case MonsterTypeFlags::Shocker: return spawnShocker(spawnPosition, color);
	case MonsterTypeFlags::Wormhole: return spawnWormhole(spawnPosition, color);
	case MonsterTypeFlags::Rocket: return spawnRocket(spawnPosition, color);
	case MonsterTypeFlags::Spawner: return spawnSpawner(spawnPosition, color);
	case MonsterTypeFlags::BossEgg: return spawnBossEgg(spawnPosition, color);
	default: return spawnSimple(type, spawnPosition, color);
	}
}

void monstersSpawnInitial()
{
	{
		SpawnDefinition d("initial 1");
		d.spawnTypes = MonsterTypeFlags::Circle;
		d.spawnCountMin = monstersLimit();
		d.spawnCountMax = d.spawnCountMin + 10;
		d.spawn();
	}
	{
		SpawnDefinition d("initial 2");
		d.spawnTypes = MonsterTypeFlags::Circle;
		d.placingPolicy = PlacingPolicyEnum::Grouped;
		d.distanceMin = 80;
		d.distanceMax = 100;
		d.spawnCountMin = 1;
		d.spawnCountMax = 3;
		d.spawn();
	}
}
