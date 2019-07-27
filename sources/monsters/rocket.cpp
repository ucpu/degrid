#include "monsters.h"

namespace
{
	struct rocketMonsterComponent
	{
		static entityComponent *component;
	};

	entityComponent *rocketMonsterComponent::component;

	void engineInit()
	{
		rocketMonsterComponent::component = entities()->defineComponent(rocketMonsterComponent(), true);
	}

	void engineUpdate()
	{
		OPTICK_EVENT("rocket");

		if (game.paused)
			return;

		real disapearDistance2 = mapNoPullRadius * 2;
		disapearDistance2 *= disapearDistance2;
		for (entity *e : rocketMonsterComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, tr, e);
			if (tr.position.squaredLength() > disapearDistance2)
				e->add(entitiesToDestroy);
			else
				tr.orientation = tr.orientation * quat(degs(), degs(), degs(3));
		}
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

void spawnRocket(const vec3 &spawnPosition, const vec3 &color)
{
	uint32 special = 0;
	entity *e = initializeMonster(spawnPosition, color, 2.5, hashString("degrid/monster/rocket.object"), hashString("degrid/monster/bum-rocket.ogg"), 6, 2 + monsterMutation(special));
	DEGRID_COMPONENT(rocketMonster, r, e);
	DEGRID_COMPONENT(velocity, v, e);
	v.velocity = game.monstersTarget - spawnPosition;
	v.velocity[1] = 0;
	v.velocity = normalize(v.velocity) * (1.5 + 0.3 * monsterMutation(special));
	CAGE_COMPONENT_ENGINE(transform, tr, e);
	tr.orientation = quat(v.velocity, vec3(0, 1, 0), true);
	monsterReflectMutation(e, special);
}

