#include "game.h"

#include <cage-core/entities.h>

EntityComponent *gravityComponent::component;
EntityComponent *velocityComponent::component;
EntityComponent *rotationComponent::component;
EntityComponent *timeoutComponent::component;
EntityComponent *gridComponent::component;
EntityComponent *shotComponent::component;
EntityComponent *powerupComponent::component;
EntityComponent *monsterComponent::component;
EntityComponent *bossComponent::component;

EventDispatcher<bool()> &gameStartEvent()
{
	static EventDispatcher<bool()> inst;
	return inst;
}

EventDispatcher<bool()> &gameStopEvent()
{
	static EventDispatcher<bool()> inst;
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
		EventListener<void()> engineInitListener;
	public:
		callbacksClass() : engineInitListener("globals")
		{
			engineInitListener.attach(controlThread().initialize, -50);
			engineInitListener.bind<&engineInit>();
		}
	} callbacksInstance;
}
