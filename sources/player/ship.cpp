#include "../game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/utility/spatial.h>
#include <cage-core/utility/hashString.h>

extern configUint32 confControlMovement;

namespace
{
	void shipMovement()
	{
		if (player.cinematic)
			return;

		ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
		GRID_GET_COMPONENT(velocity, vl, player.playerEntity);

		if (player.moveDirection != vec3())
		{
			real maxSpeed = player.powerups[(uint32)powerupTypeEnum::MaxSpeed] * 0.3 + 0.8;
			vec3 change = player.moveDirection.normalize() * (player.powerups[(uint32)powerupTypeEnum::Acceleration] + 1) * 0.1;
			if (confControlMovement == 1 && ((tr.orientation * vec3(0, 0, -1)).dot(normalize(vl.velocity + change)) < 1e-5))
				vl.velocity = vec3();
			else
				vl.velocity += change;
			if (vl.velocity.squaredLength() > maxSpeed * maxSpeed)
				vl.velocity = vl.velocity.normalize() * maxSpeed;
			if (change.squaredLength() > 0.01)
			{
				entityClass *spark = entities()->newAnonymousEntity();
				ENGINE_GET_COMPONENT(transform, transform, spark);
				transform.scale = cage::random() * 0.2 + 0.3;
				transform.position = tr.position + tr.orientation * vec3((sint32)(statistics.updateIterationIgnorePause % 2) * 1.2 - 0.6, 0, 1) * tr.scale;
				transform.orientation = randomDirectionQuat();
				ENGINE_GET_COMPONENT(render, render, spark);
				render.object = hashString("grid/environment/spark.object");
				GRID_GET_COMPONENT(velocity, vel, spark);
				vel.velocity = (change + randomDirection3() * 0.05) * cage::random() * -5;
				GRID_GET_COMPONENT(timeout, ttl, spark);
				ttl.ttl = random(10, 15);
				ENGINE_GET_COMPONENT(animatedTexture, at, spark);
				at.startTime = currentControlTime();
				at.speed = 30.f / ttl.ttl;
			}
		}
		else
			vl.velocity *= 0.97;

		// pull to center
		if (tr.position.length() > mapNoPullRadius)
		{
			vec3 pullToCenter = -tr.position.normalize() * pow((tr.position.length() - mapNoPullRadius) / mapNoPullRadius, 2);
			vl.velocity += pullToCenter;
		}

		vl.velocity[1] = 0;
		tr.position[1] = 0.5;
		player.monstersTarget = tr.position + vl.velocity * 3;
		if (vl.velocity.squaredLength() > 1e-5)
			tr.orientation = quat(degs(), aTan2(-vl.velocity[2], -vl.velocity[0]), degs());
	}

	void shipShield()
	{
		if (!player.playerEntity || !player.shieldEntity)
			return;
		ENGINE_GET_COMPONENT(transform, tr, player.playerEntity);
		ENGINE_GET_COMPONENT(transform, trs, player.shieldEntity);
		trs.position = tr.position;
		trs.scale = tr.scale;
		if (player.powerups[(uint32)powerupTypeEnum::Shield] > 0)
		{
			ENGINE_GET_COMPONENT(render, render, player.shieldEntity);
			render.object = hashString("grid/player/shield.object");
			ENGINE_GET_COMPONENT(voice, sound, player.shieldEntity);
			sound.name = hashString("grid/player/shield.ogg");
			sound.startTime = -1;
		}
		else
		{
			player.shieldEntity->removeComponent(renderComponent::component);
			player.shieldEntity->removeComponent(voiceComponent::component);
		}
	}

	void scoreUpdate()
	{
		uint32 lg = player.scorePrevious >= 20000 ? 10 : player.scorePrevious >= 2000 ? 2 : 1;
		uint32 sg = lg * 500;
		uint32 ld = (player.score - player.scorePrevious) / sg;
		if (ld)
		{
			player.scorePrevious += ld * sg;

			uint32 sounds[] = {
				hashString("grid/speech/gain/doing-fine.wav"),
				hashString("grid/speech/gain/doing-well.wav"),
				hashString("grid/speech/gain/fantastic.wav"),
				hashString("grid/speech/gain/go-on.wav"),
				hashString("grid/speech/gain/keep-going.wav"),
				hashString("grid/speech/gain/lets-roll.wav"),
				0 };
			soundSpeech(sounds);
		}
	}

	bool engineUpdate()
	{
		if (!player.paused)
		{
			shipMovement();
			scoreUpdate();
			if (!player.cinematic && player.life <= 1e-7)
			{
				gameStopEvent().dispatch();
				return true;
			}
		}
		shipShield();
		return false;
	}

	void gameStart()
	{
		{ // player ship entity
			player.playerEntity = entities()->newUniqueEntity();
			ENGINE_GET_COMPONENT(transform, transform, player.playerEntity);
			transform.scale = playerScale;
			ENGINE_GET_COMPONENT(render, render, player.playerEntity);
			render.object = hashString("grid/player/player.object");
			player.monstersTarget = vec3();
		}

		{ // player shield entity
			player.shieldEntity = entities()->newUniqueEntity();
			ENGINE_GET_COMPONENT(transform, transform, player.shieldEntity);
			(void)transform;
			ENGINE_GET_COMPONENT(animatedTexture, aniTex, player.shieldEntity);
			aniTex.speed = 0.05;
		}
	}

	void gameStop()
	{
		ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, player.playerEntity);
		environmentExplosion(playerTransform.position, playerVelocity.velocity, playerDeathColor, 20, playerScale);
		for (uint32 i = 0; i < 10; i++)
			environmentExplosion(playerTransform.position, randomDirection3() * vec3(1, 0.1, 1), playerDeathColor, 5, playerScale);
		player.playerEntity->addGroup(entitiesToDestroy);
		player.playerEntity = nullptr;
	}

	class callbacksClass
	{
		eventListener<bool()> engineUpdateListener;
		eventListener<void()> gameStartListener;
		eventListener<void()> gameStopListener;
	public:
		callbacksClass()
		{
			engineUpdateListener.attach(controlThread().update, -20);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), -20);
			gameStartListener.bind<&gameStart>();
			gameStopListener.attach(gameStopEvent(), -20);
			gameStopListener.bind<&gameStop>();
		}
	} callbacksInstance;
}
