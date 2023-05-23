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
	const auto engineInitListener = controlThread().initialize.listen([]() {
		engineEntities()->defineComponent(GravityComponent());
		engineEntities()->defineComponent(VelocityComponent());
		engineEntities()->defineComponent(RotationComponent());
		engineEntities()->defineComponent(TimeoutComponent());
		engineEntities()->defineComponent(GridComponent());
		engineEntities()->defineComponent(ShotComponent());
		engineEntities()->defineComponent(PowerupComponent());
		engineEntities()->defineComponent(MonsterComponent());
		engineEntities()->defineComponent(BossComponent());
	}, -50);
}
