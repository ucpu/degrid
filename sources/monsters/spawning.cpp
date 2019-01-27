#include <algorithm>
#include <vector>

#include "monsters.h"

#include <cage-core/color.h>

namespace
{
	vec3 playerPosition;

	vec3 aroundPosition(real index, real radius, vec3 center)
	{
		rads angle = index * rads::Full;
		vec3 dir = vec3(cos(angle), 0, sin(angle));
		return center + dir * radius;
	}

	enum class placingPolicyEnum
	{
		Random,
		Around,
		Grouped,
	};

	struct spawnDefinitionStruct
	{
		// spawned monsters
		uint32 spawnCountMin, spawnCountMax;
		real distanceMin, distanceMax;
		monsterTypeFlags spawnTypes;
		placingPolicyEnum placingPolicy;

		// priority
		real priorityCurrent;
		real priorityChange;
		real priorityAdditive;
		real priorityMultiplier;

		// statistics
		uint32 iteration;
		uint32 spawned;
		string name;

		spawnDefinitionStruct(const string &name);
		bool operator < (const spawnDefinitionStruct &other) const { return priorityCurrent < other.priorityCurrent; }
		void perform();
		void performSimulation();
		void updatePriority();
		void spawn();
	};

	spawnDefinitionStruct::spawnDefinitionStruct(const string &name) :
		spawnTypes((monsterTypeFlags)0),
		spawnCountMin(1), spawnCountMax(1),
		distanceMin(200), distanceMax(250),
		placingPolicy(placingPolicyEnum::Random),
		priorityCurrent(0), priorityChange(0), priorityAdditive(0), priorityMultiplier(1),
		iteration(0), spawned(0), name(name)
	{}

	uint32 monstersLimit()
	{
#ifdef CAGE_DEBUG
		return 50;
#else
		if (game.cinematic)
			return 100;
		uint32 t = statistics.updateIteration;
		return 100 + min(t / 200, 50u) + min(t / 1000, 50u);
#endif // CAGE_DEBUG
	}

	std::vector<spawnDefinitionStruct> definitions;

	void spawnDefinitionStruct::perform()
	{
		spawn();
		updatePriority();
		statistics.monstersCurrentSpawningPriority = priorityCurrent;
	}

	void spawnDefinitionStruct::updatePriority()
	{
		iteration++;
		priorityCurrent += max(priorityChange, 1e-5);
		priorityChange += priorityAdditive;
		priorityChange *= priorityMultiplier;
	}

	void spawnDefinitionStruct::performSimulation()
	{
		updatePriority();
		statistics.monstersCurrentMutationIteration += 2; // simulate average mutation growth
	}

	void spawnDefinitionStruct::spawn()
	{
		{ // update player position
			ENGINE_GET_COMPONENT(transform, p, game.playerEntity);
			playerPosition = p.position;
		}

		CAGE_ASSERT_RUNTIME(spawnCountMin <= spawnCountMax && spawnCountMin > 0, spawnCountMin, spawnCountMax);
		CAGE_ASSERT_RUNTIME(distanceMin <= distanceMax && distanceMin > 0, distanceMin, distanceMax);
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
		CAGE_ASSERT_RUNTIME(alSiz > 0);
		uint32 spawnCount = randomRange(spawnCountMin, spawnCountMax + 1);
		spawned += spawnCount;
		vec3 color = convertHsvToRgb(vec3(randomChance(), sqrt(randomChance()) * 0.5 + 0.5, sqrt(randomChance()) * 0.5 + 0.5));
		switch (placingPolicy)
		{
		case placingPolicyEnum::Random:
		{
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[randomRange(0u, alSiz)], aroundPosition(randomChance(), randomRange(distanceMin, distanceMax), playerPosition), color);
		} break;
		case placingPolicyEnum::Around:
		{
			real angularOffset = randomChance();
			real radius = randomRange(distanceMin, distanceMax);
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[randomRange(0u, alSiz)], aroundPosition(angularOffset + (randomChance() * 0.3 + i) / (real)spawnCount, radius, playerPosition), color);
		} break;
		case placingPolicyEnum::Grouped:
		{
			real radius = (distanceMax - distanceMin) * 0.5;
			vec3 center = aroundPosition(randomChance(), distanceMin + radius, playerPosition);
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[randomRange(0u, alSiz)], aroundPosition((real)i / (real)spawnCount, radius, center), color);
		} break;
		default: CAGE_THROW_CRITICAL(exception, "invalid placing policy");
		}
	}

	void engineUpdate()
	{
		if (game.paused)
			return;

		if (bossComponent::component->group()->count() > 0)
			return;

		uint32 limit = monstersLimit();
		real probability = 1.f - (real)statistics.monstersCurrent / (real)limit;
		if (probability <= 0)
			return;

		probability = pow(100, probability - 1);
		if (randomChance() > probability)
			return;

		definitions[0].perform();
		std::nth_element(definitions.begin(), definitions.begin(), definitions.end());
	}

	void announceJokeMap()
	{
		game.jokeMap = true;
		makeAnnouncement(hashString("announcement/joke-map"), hashString("announcement-desc/joke-map"), 120 * 30);
	}

	void gameStart()
	{
		definitions.clear();
		definitions.reserve(30);

		if (game.cinematic)
		{ // cinematic spawns
			spawnDefinitionStruct d("cinematic");
			d.spawnTypes = (monsterTypeFlags::Circle | monsterTypeFlags::SmallTriangle | monsterTypeFlags::SmallCube | monsterTypeFlags::LargeTriangle | monsterTypeFlags::LargeCube);
			definitions.push_back(d);
			return;
		}

		///////////////////////////////////////////////////////////////////////////
		// jokes
		///////////////////////////////////////////////////////////////////////////

		if (randomRange(0u, 1000u) == 42)
		{
			CAGE_LOG(severityEnum::Info, "joke", "JOKE: snakes only map");
			spawnDefinitionStruct d("snakes only");
			d.spawnTypes = (monsterTypeFlags::Snake);
			d.priorityCurrent = 0;
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
			spawnDefinitionStruct d("individual small monsters");
			d.spawnTypes = (monsterTypeFlags::Circle | monsterTypeFlags::SmallTriangle | monsterTypeFlags::SmallCube);
			d.priorityCurrent = 0;
			d.priorityChange = 1;
			d.priorityAdditive = 0.001;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // large monsters individually
			spawnDefinitionStruct d("individual large monsters");
			d.spawnTypes = (monsterTypeFlags::LargeTriangle | monsterTypeFlags::LargeCube);
			d.priorityCurrent = 50;
			d.priorityChange = 1;
			d.priorityAdditive = 0.001;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // pin wheels individually
			spawnDefinitionStruct d("individual pinwheels");
			d.spawnTypes = (monsterTypeFlags::PinWheel);
			d.priorityCurrent = 300;
			d.priorityChange = 23;
			d.priorityAdditive = 0.01;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // diamonds individually
			spawnDefinitionStruct d("individual diamonds");
			d.spawnTypes = (monsterTypeFlags::Diamond);
			d.priorityCurrent = 500;
			d.priorityChange = 37;
			d.priorityAdditive = 0.1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // snakes individually
			spawnDefinitionStruct d("individual snakes");
			d.spawnTypes = (monsterTypeFlags::Snake);
			d.priorityCurrent = 2000;
			d.priorityChange = 133;
			d.priorityAdditive = 1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // shielders individually
			spawnDefinitionStruct d("individual shielders");
			d.spawnTypes = (monsterTypeFlags::Shielder);
			d.priorityCurrent = 2500;
			d.priorityChange = 127;
			d.priorityAdditive = 1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // shockers individually
			spawnDefinitionStruct d("individual shockers");
			d.spawnTypes = (monsterTypeFlags::Shocker);
			d.priorityCurrent = 3000;
			d.priorityChange = 143;
			d.priorityAdditive = 1;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// groups and circles
		///////////////////////////////////////////////////////////////////////////

		{ // circle groups
			spawnDefinitionStruct d("circle groups");
			d.spawnCountMin = 10;
			d.spawnCountMax = 20;
			d.spawnTypes = (monsterTypeFlags::Circle);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 700;
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(1, 10);
			definitions.push_back(d);
		}

		{ // large triangle groups
			spawnDefinitionStruct d("large triangle groups");
			d.spawnCountMin = 5;
			d.spawnCountMax = 15;
			d.spawnTypes = (monsterTypeFlags::LargeTriangle);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 1500;
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(1, 10);
			definitions.push_back(d);
		}

		{ // large cube groups
			spawnDefinitionStruct d("large cube groups");
			d.spawnCountMin = 5;
			d.spawnCountMax = 15;
			d.spawnTypes = (monsterTypeFlags::LargeCube);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 1500;
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(1, 10);
			definitions.push_back(d);
		}

		{ // pin wheel groups
			spawnDefinitionStruct d("pinwheel groups");
			d.spawnCountMin = 2;
			d.spawnCountMax = 3;
			d.spawnTypes = (monsterTypeFlags::PinWheel);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 2500;
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(5, 15);
			definitions.push_back(d);
		}

		{ // diamond groups
			spawnDefinitionStruct d("diamond groups");
			d.spawnCountMin = 3;
			d.spawnCountMax = 7;
			d.spawnTypes = (monsterTypeFlags::Diamond);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 3000;
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(5, 15);
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// bosses
		///////////////////////////////////////////////////////////////////////////

		{ // bosses
			spawnDefinitionStruct d("bosses");
			d.spawnCountMin = 1;
			d.spawnCountMax = 1;
			d.spawnTypes = (monsterTypeFlags::BossEgg);
			d.placingPolicy = placingPolicyEnum::Around;
			d.distanceMin = 300;
			d.distanceMax = 350;
#if 1
			d.priorityCurrent = 4000;
			d.priorityChange = 20000;
			d.priorityMultiplier = 2;
#endif
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// final stage
		///////////////////////////////////////////////////////////////////////////

		{ // mixed group
			spawnDefinitionStruct d("mixed group");
			d.spawnCountMin = 20;
			d.spawnCountMax = 25;
			d.spawnTypes = (monsterTypeFlags::LargeTriangle | monsterTypeFlags::LargeCube | monsterTypeFlags::Diamond);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 5000;
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(5, 15);
			definitions.push_back(d);
		}

		{ // mixed around
			spawnDefinitionStruct d("mixed around");
			d.spawnCountMin = 5;
			d.spawnCountMax = 15;
			d.spawnTypes = (monsterTypeFlags::PinWheel | monsterTypeFlags::Snake | monsterTypeFlags::Shielder | monsterTypeFlags::Shocker);
			d.placingPolicy = placingPolicyEnum::Around;
			d.distanceMin = 160;
			d.distanceMax = 190;
			d.priorityCurrent = 5500;
			d.priorityChange = randomRange(200, 300);
			d.priorityAdditive = randomRange(5, 15);
			definitions.push_back(d);
		}

		{ // wormholes
			spawnDefinitionStruct d("wormholes");
			d.spawnCountMin = 1;
			d.spawnCountMax = 3;
			d.spawnTypes = (monsterTypeFlags::Wormhole);
			d.placingPolicy = placingPolicyEnum::Around;
			d.distanceMin = 160;
			d.distanceMax = 190;
			d.priorityCurrent = 7000;
			d.priorityChange = randomRange(6000, 8000);
			d.priorityAdditive = randomRange(300, 500);
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// testing
		///////////////////////////////////////////////////////////////////////////

#ifdef GRID_TESTING
		std::nth_element(definitions.begin(), definitions.begin(), definitions.end());
		while (definitions[0].priorityCurrent < 50000)
		{
			definitions[0].performSimulation();
			std::nth_element(definitions.begin(), definitions.begin(), definitions.end());
		}
#endif
	}

	void gameStop()
	{
#ifdef GRID_TESTING
		for (auto &d : definitions)
		{
			if (d.iteration > 0)
				CAGE_LOG(severityEnum::Info, "statistics", string() + "spawn '" + d.name + "', iteration: " + d.iteration + ", spawned: " + d.spawned + ", priority: " + d.priorityCurrent + ", change: " + d.priorityChange);
		}
#endif // GRID_TESTING

		definitions.clear();
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
		eventListener<void()> gameStopListener;
	public:
		callbacksClass()
		{
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent());
			gameStartListener.bind<&gameStart>();
			gameStopListener.attach(gameStopEvent());
			gameStopListener.bind<&gameStop>();
		}
	} callbacksInstance;
}

void spawnGeneral(monsterTypeFlags type, const vec3 &spawnPosition, const vec3 &color)
{
	switch (type)
	{
	case monsterTypeFlags::Snake: return spawnSnake(spawnPosition, color);
	case monsterTypeFlags::Shielder: return spawnShielder(spawnPosition, color);
	case monsterTypeFlags::Shocker: return spawnShocker(spawnPosition, color);
	case monsterTypeFlags::Wormhole: return spawnWormhole(spawnPosition, color);
	case monsterTypeFlags::BossEgg: return spawnBossEgg(spawnPosition, color);
	default: return spawnSimple(type, spawnPosition, color);
	}
}

void monstersSpawnInitial()
{
	{
		spawnDefinitionStruct d("initial 1");
		d.spawnTypes = (monsterTypeFlags)(monsterTypeFlags::Circle);
		d.spawnCountMin = monstersLimit();
		d.spawnCountMax = d.spawnCountMin + 10;
		d.spawn();
	}
	{
		spawnDefinitionStruct d("initial 2");
		d.spawnTypes = (monsterTypeFlags)(monsterTypeFlags::Circle);
		d.placingPolicy = placingPolicyEnum::Grouped;
		d.distanceMin = 80;
		d.distanceMax = 100;
		d.spawnCountMin = 1;
		d.spawnCountMax = 3;
		d.spawn();
	}
}
