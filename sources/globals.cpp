#include "game.h"

#include <cage-core/entities.h>

EntityComponent *GravityComponent::component;
EntityComponent *VelocityComponent::component;
EntityComponent *RotationComponent::component;
EntityComponent *TimeoutComponent::component;
EntityComponent *GridComponent::component;
EntityComponent *ShotComponent::component;
EntityComponent *PowerupComponent::component;
EntityComponent *MonsterComponent::component;
EntityComponent *BossComponent::component;

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
		GravityComponent::component = engineEntities()->defineComponent(GravityComponent(), true);
		VelocityComponent::component = engineEntities()->defineComponent(VelocityComponent(), true);
		RotationComponent::component = engineEntities()->defineComponent(RotationComponent(), true);
		TimeoutComponent::component = engineEntities()->defineComponent(TimeoutComponent(), true);
		GridComponent::component = engineEntities()->defineComponent(GridComponent(), true);
		ShotComponent::component = engineEntities()->defineComponent(ShotComponent(), true);
		PowerupComponent::component = engineEntities()->defineComponent(PowerupComponent(), true);
		MonsterComponent::component = engineEntities()->defineComponent(MonsterComponent(), true);
		BossComponent::component = engineEntities()->defineComponent(BossComponent(), true);
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
	public:
		Callbacks() : engineInitListener("globals")
		{
			engineInitListener.attach(controlThread().initialize, -50);
			engineInitListener.bind<&engineInit>();
		}
	} callbacksInstance;
}
