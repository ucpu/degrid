#include <cage-core/entities.h>

#include "game.h"

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
		engineEntities()->defineComponent(GravityComponent());
		engineEntities()->defineComponent(VelocityComponent());
		engineEntities()->defineComponent(RotationComponent());
		engineEntities()->defineComponent(TimeoutComponent());
		engineEntities()->defineComponent(GridComponent());
		engineEntities()->defineComponent(ShotComponent());
		engineEntities()->defineComponent(PowerupComponent());
		engineEntities()->defineComponent(MonsterComponent());
		engineEntities()->defineComponent(BossComponent());
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
