#include "monsters.h"

namespace
{
	struct cannoneerBodyComponent
	{
		static componentClass *component;
	};

	struct cannoneerHandComponent
	{
		static componentClass *component;
	};

	componentClass *cannoneerBodyComponent::component;
	componentClass *cannoneerHandComponent::component;

	void engineInit()
	{
		cannoneerBodyComponent::component = entities()->defineComponent(cannoneerBodyComponent(), true);
		cannoneerHandComponent::component = entities()->defineComponent(cannoneerHandComponent(), true);
	}

	void engineUpdate()
	{
		if (game.paused)
			return;
		// todo
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

void spawnBossCannoneer(const vec3 &spawnPosition, const vec3 &color)
{
	entityClass *body = initializeMonster(spawnPosition, color, 7, hashString("grid/boss/cannoneerBody.object"), hashString("grid/monster/boss/cannoneer-bum.ogg"), real::PositiveInfinity, /*real::PositiveInfinity*/10);
	{
		GRID_GET_COMPONENT(boss, boss, body);
	}
	{
		GRID_GET_COMPONENT(cannoneerBody, cb, body);
	}
}
