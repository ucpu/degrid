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
		GravityComponent::component = entities()->defineComponent(GravityComponent(), true);
		VelocityComponent::component = entities()->defineComponent(VelocityComponent(), true);
		RotationComponent::component = entities()->defineComponent(RotationComponent(), true);
		TimeoutComponent::component = entities()->defineComponent(TimeoutComponent(), true);
		GridComponent::component = entities()->defineComponent(GridComponent(), true);
		ShotComponent::component = entities()->defineComponent(ShotComponent(), true);
		PowerupComponent::component = entities()->defineComponent(PowerupComponent(), true);
		MonsterComponent::component = entities()->defineComponent(MonsterComponent(), true);
		BossComponent::component = entities()->defineComponent(BossComponent(), true);
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
