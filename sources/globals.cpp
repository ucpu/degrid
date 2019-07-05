#include "game.h"

#include <cage-core/entities.h>

entityComponent *gravityComponent::component;
entityComponent *velocityComponent::component;
entityComponent *rotationComponent::component;
entityComponent *timeoutComponent::component;
entityComponent *gridComponent::component;
entityComponent *shotComponent::component;
entityComponent *powerupComponent::component;
entityComponent *monsterComponent::component;
entityComponent *bossComponent::component;

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
		bossComponent::component = entities()->defineComponent(bossComponent(), true);
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
