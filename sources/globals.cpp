#include "game.h"

#include <cage-core/entities.h>

componentClass *gravityComponent::component;
componentClass *velocityComponent::component;
componentClass *rotationComponent::component;
componentClass *timeoutComponent::component;
componentClass *gridComponent::component;
componentClass *shotComponent::component;
componentClass *powerupComponent::component;
componentClass *monsterComponent::component;

eventDispatcher<bool()> &gameStartEvent()
{
	static eventDispatcher<bool()> inst;
	return inst;
}

eventDispatcher<bool()> &gameStopEvent()
{
	static eventDispatcher<bool()> inst;
	return inst;
}

namespace
{
	void engineInit()
	{
		gravityComponent::component = entities()->defineComponent(gravityComponent(), true);
		velocityComponent::component = entities()->defineComponent(velocityComponent(), true);
		rotationComponent::component = entities()->defineComponent(rotationComponent(), true);
		timeoutComponent::component = entities()->defineComponent(timeoutComponent(), true);
		gridComponent::component = entities()->defineComponent(gridComponent(), true);
		shotComponent::component = entities()->defineComponent(shotComponent(), true);
		powerupComponent::component = entities()->defineComponent(powerupComponent(), true);
		monsterComponent::component = entities()->defineComponent(monsterComponent(), true);
	}

	class callbacksClass
	{
		eventListener<void()> engineInitListener;
	public:
		callbacksClass()
		{
			engineInitListener.attach(controlThread().initialize, -50);
			engineInitListener.bind<&engineInit>();
		}
	} callbacksInstance;
}
