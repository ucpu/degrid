#include <cage-core/entities.h>

#include "game.h"

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
		GravityComponent::component = engineEntities()->defineComponent(GravityComponent());
		VelocityComponent::component = engineEntities()->defineComponent(VelocityComponent());
		RotationComponent::component = engineEntities()->defineComponent(RotationComponent());
		TimeoutComponent::component = engineEntities()->defineComponent(TimeoutComponent());
		GridComponent::component = engineEntities()->defineComponent(GridComponent());
		ShotComponent::component = engineEntities()->defineComponent(ShotComponent());
		PowerupComponent::component = engineEntities()->defineComponent(PowerupComponent());
		MonsterComponent::component = engineEntities()->defineComponent(MonsterComponent());
		BossComponent::component = engineEntities()->defineComponent(BossComponent());
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
