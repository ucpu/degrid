#include <algorithm>
#include <vector>

#include "monsters.h"

#include <cage-core/color.h>

namespace
{
	vec3 playerPosition;

	const vec3 aroundPosition(real index, real radius, vec3 center)
	{
		rads angle = index * rads::Full;
		vec3 dir = vec3(cos(angle), 0, sin(angle));
		return center + dir * radius;
	}

	const vec3 randomPosition()
	{
		return aroundPosition(cage::random(), cage::random(200, 400), playerPosition);
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
		const bool operator < (const spawnDefinitionStruct &other) const { return priorityCurrent < other.priorityCurrent; }
		void perform();
		void performSimulation();
		void spawn();
	};

	spawnDefinitionStruct::spawnDefinitionStruct(const string &name) :
		spawnTypes((monsterTypeFlags)0),
		spawnCountMin(1), spawnCountMax(1),
		placingPolicy(placingPolicyEnum::Random),
		priorityCurrent(0), priorityChange(0), priorityAdditive(0), priorityMultiplier(1),
		iteration(0), spawned(0), name(name)
	{}

	const uint32 monstersLimit()
	{
#ifdef CAGE_DEBUG
		return 50;
#else
		return player.cinematic ? 100 : (100 + min(player.score / 30, 50u) + min(player.score / 10000, 50u));
#endif // CAGE_DEBUG
	}

	std::vector<spawnDefinitionStruct> definitions;

	void spawnDefinitionStruct::perform()
	{
		spawn();
		performSimulation();
		statistics.monstersCurrentSpawningPriority = priorityCurrent;
	}

	void spawnDefinitionStruct::performSimulation()
	{
		iteration++;
		priorityCurrent += max(priorityChange, 1e-5);
		priorityChange += priorityAdditive;
		priorityChange *= priorityMultiplier;
	}

	void spawnDefinitionStruct::spawn()
	{
		{ // update player position
			ENGINE_GET_COMPONENT(transform, p, player.playerEntity);
			playerPosition = p.position;
		}

		CAGE_ASSERT_RUNTIME(spawnCountMin <= spawnCountMax && spawnCountMin > 0, spawnCountMin, spawnCountMax);
		std::vector<monsterTypeFlags> allowed;
		allowed.reserve(16);
		{
			uint32 bit = 1;
			for (uint32 i = 0; i < 16; i++)
			{
				if ((spawnTypes & (monsterTypeFlags)bit) == (monsterTypeFlags)bit)
					allowed.push_back((monsterTypeFlags)bit);
				bit <<= 1;
			}
		}
		uint32 alSiz = numeric_cast<uint32>(allowed.size());
		CAGE_ASSERT_RUNTIME(alSiz > 0);
		uint32 spawnCount = random(spawnCountMin, spawnCountMax + 1);
		spawned += spawnCount;
		vec3 color = convertHsvToRgb(vec3(cage::random(), sqrt(cage::random()) * 0.5 + 0.5, sqrt(cage::random()) * 0.5 + 0.5));
		switch (placingPolicy)
		{
		case placingPolicyEnum::Random:
		{
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[cage::random(0u, alSiz)], randomPosition(), color);
		} break;
		case placingPolicyEnum::Around:
		{
			real angularOffset = cage::random();
			real radius = cage::random(120, 140);
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[cage::random(0u, alSiz)], aroundPosition(angularOffset + (cage::random() * 0.3 + i) / (real)spawnCount, radius, playerPosition), color);
		} break;
		case placingPolicyEnum::Grouped:
		{
			real radius = cage::random(spawnCount / 2, spawnCount);
			vec3 center = aroundPosition(cage::random(), cage::random(200, 250), playerPosition);
			for (uint32 i = 0; i < spawnCount; i++)
				spawnGeneral(allowed[random(0u, alSiz)], aroundPosition((real)i / (real)spawnCount, radius, center), color);
		} break;
		default: CAGE_THROW_CRITICAL(exception, "invalid placing policy");
		}
	}

	void engineUpdate()
	{
		if (player.paused)
			return;

		uint32 limit = monstersLimit();
		real probability = 1.f - (real)statistics.monstersCurrent / (real)limit;
		if (probability <= 0)
			return;

		probability = pow(100, probability - 1);
		if (cage::random() > probability)
			return;

		definitions[0].perform();
		std::nth_element(definitions.begin(), definitions.begin(), definitions.end());
	}

	void gameStart()
	{
		definitions.clear();
		definitions.reserve(30);

		if (player.cinematic)
		{ // cinematic spawns
			spawnDefinitionStruct d("cinematic");
			d.spawnTypes = (monsterTypeFlags)(monsterTypeFlags::Circle | monsterTypeFlags::SmallTriangle | monsterTypeFlags::SmallCube | monsterTypeFlags::LargeTriangle | monsterTypeFlags::LargeCube);
			definitions.push_back(d);
			return;
		}

		///////////////////////////////////////////////////////////////////////////
		// jokes
		///////////////////////////////////////////////////////////////////////////

		if (cage::random() < 0.0001)
		{
			CAGE_LOG(severityEnum::Info, "joke", "JOKE: snakes only");
			spawnDefinitionStruct d("snakes only");
			d.spawnTypes = (monsterTypeFlags)(monsterTypeFlags::Snake);
			d.priorityCurrent = 0;
			definitions.push_back(d);
			return;
		}

		if (cage::random() < 0.0001)
		{
			CAGE_LOG(severityEnum::Info, "joke", "JOKE: shielders only");
			spawnDefinitionStruct d("shielders only");
			d.spawnTypes = (monsterTypeFlags)(monsterTypeFlags::Shielder);
			d.priorityCurrent = 0;
			definitions.push_back(d);
			return;
		}

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

		{ // shielders individually
			spawnDefinitionStruct d("individual shielders");
			d.spawnTypes = (monsterTypeFlags::Shielder);
			d.priorityCurrent = 2000;
			d.priorityChange = 71;
			d.priorityAdditive = 0.5;
			d.priorityMultiplier = 1.02;
			definitions.push_back(d);
		}

		{ // snakes individually
			spawnDefinitionStruct d("individual snakes");
			d.spawnTypes = (monsterTypeFlags::Snake);
			d.priorityCurrent = 3000;
			d.priorityChange = 133;
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
			d.priorityChange = cage::random(200, 300);
			d.priorityAdditive = cage::random(1, 10);
			definitions.push_back(d);
		}

		{ // large triangle groups
			spawnDefinitionStruct d("large triangle groups");
			d.spawnCountMin = 5;
			d.spawnCountMax = 15;
			d.spawnTypes = (monsterTypeFlags::LargeTriangle);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 1500;
			d.priorityChange = cage::random(200, 300);
			d.priorityAdditive = cage::random(1, 10);
			definitions.push_back(d);
		}

		{ // large cube groups
			spawnDefinitionStruct d("large cube groups");
			d.spawnCountMin = 5;
			d.spawnCountMax = 15;
			d.spawnTypes = (monsterTypeFlags::LargeCube);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 1500;
			d.priorityChange = cage::random(200, 300);
			d.priorityAdditive = cage::random(1, 10);
			definitions.push_back(d);
		}

		{ // pin wheel groups
			spawnDefinitionStruct d("pinwheel groups");
			d.spawnCountMin = 2;
			d.spawnCountMax = 3;
			d.spawnTypes = (monsterTypeFlags::PinWheel);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 2500;
			d.priorityChange = cage::random(200, 300);
			d.priorityAdditive = cage::random(5, 15);
			definitions.push_back(d);
		}

		{ // diamond groups
			spawnDefinitionStruct d("diamond groups");
			d.spawnCountMin = 3;
			d.spawnCountMax = 7;
			d.spawnTypes = (monsterTypeFlags::Diamond);
			d.placingPolicy = placingPolicyEnum::Grouped;
			d.priorityCurrent = 3000;
			d.priorityChange = cage::random(200, 300);
			d.priorityAdditive = cage::random(5, 15);
			definitions.push_back(d);
		}

		///////////////////////////////////////////////////////////////////////////
		// final stage
		///////////////////////////////////////////////////////////////////////////

		{ // finals 1
			spawnDefinitionStruct d("finals 1");
			d.spawnCountMin = 20;
			d.spawnCountMax = 25;
			d.spawnTypes = (monsterTypeFlags::LargeTriangle | monsterTypeFlags::LargeCube | monsterTypeFlags::Diamond);
			d.placingPolicy = placingPolicyEnum::Around;
			d.priorityCurrent = 5000;
			d.priorityChange = cage::random(200, 300);
			d.priorityAdditive = cage::random(5, 15);
			d.priorityMultiplier = 0.98;
			definitions.push_back(d);
		}

		{ // finals 2
			spawnDefinitionStruct d("finals 2");
			d.spawnCountMin = 10;
			d.spawnCountMax = 20;
			d.spawnTypes = (monsterTypeFlags::Circle | monsterTypeFlags::LargeTriangle | monsterTypeFlags::LargeCube | monsterTypeFlags::PinWheel | monsterTypeFlags::Shielder);
			d.placingPolicy = placingPolicyEnum::Around;
			d.priorityCurrent = 5000;
			d.priorityChange = cage::random(300, 400);
			d.priorityAdditive = cage::random(10, 20);
			d.priorityMultiplier = 0.98;
			definitions.push_back(d);
		}

		{ // wormholes
			spawnDefinitionStruct d("wormholes");
			d.spawnCountMin = 1;
			d.spawnCountMax = 2;
			d.spawnTypes = (monsterTypeFlags::Wormhole);
			d.placingPolicy = placingPolicyEnum::Around;
			d.priorityCurrent = 7000;
			d.priorityChange = cage::random(6000, 8000);
			d.priorityAdditive = cage::random(300, 500);
			d.priorityMultiplier = 0.98;
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
	case monsterTypeFlags::Shielder: return spawnShielder(spawnPosition, color);
	case monsterTypeFlags::Snake: return spawnSnake(spawnPosition, color);
	case monsterTypeFlags::Wormhole: return spawnWormhole(spawnPosition, color);
	default: return spawnSimple(type, spawnPosition, color);
	}
}

void monstersSpawnInitial()
{
	spawnDefinitionStruct d("initial");
	d.spawnTypes = (monsterTypeFlags)(monsterTypeFlags::Circle);
	d.spawnCountMin = monstersLimit() - 5;
	d.spawnCountMax = d.spawnCountMin + 25;
	d.spawn();
}

