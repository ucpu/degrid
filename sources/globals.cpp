#include "game.h"

#include <cage-core/entities.h>

componentClass *velocityComponent::component;
componentClass *timeoutComponent::component;
componentClass *gridComponent::component;
componentClass *shotComponent::component;
componentClass *powerupComponent::component;
componentClass *turretComponent::component;
componentClass *decoyComponent::component;
componentClass *monsterComponent::component;
componentClass *simpleMonsterComponent::component;
componentClass *snakeHeadComponent::component;
componentClass *snakeTailComponent::component;
componentClass *shielderComponent::component;
componentClass *shieldComponent::component;
componentClass *wormholeComponent::component;

cage::eventDispatcher<bool()> &gameStartEvent()
{
	static cage::eventDispatcher<bool()> inst;
	return inst;
}

cage::eventDispatcher<bool()> &gameStopEvent()
{
	static cage::eventDispatcher<bool()> inst;
	return inst;
}

namespace
{
	void engineInit()
	{
		velocityComponent::component = entities()->defineComponent(velocityComponent(), true);
		timeoutComponent::component = entities()->defineComponent(timeoutComponent(), true);
		gridComponent::component = entities()->defineComponent(gridComponent(), true);
		shotComponent::component = entities()->defineComponent(shotComponent(), true);
		powerupComponent::component = entities()->defineComponent(powerupComponent(), true);
		turretComponent::component = entities()->defineComponent(turretComponent(), true);
		decoyComponent::component = entities()->defineComponent(decoyComponent(), true);
		monsterComponent::component = entities()->defineComponent(monsterComponent(), true);
		simpleMonsterComponent::component = entities()->defineComponent(simpleMonsterComponent(), true);
		snakeTailComponent::component = entities()->defineComponent(snakeTailComponent(), true);
		snakeHeadComponent::component = entities()->defineComponent(snakeHeadComponent(), true);
		shielderComponent::component = entities()->defineComponent(shielderComponent(), true);
		shieldComponent::component = entities()->defineComponent(shieldComponent(), true);
		wormholeComponent::component = entities()->defineComponent(wormholeComponent(), true);
		entitiesToDestroy = entities()->defineGroup();
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize, -1000);
			engineInitListener.bind<&engineInit>();
		}
	} callbacksInstance;
}
