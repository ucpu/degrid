#include "../game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/spatial.h>
#include <cage-core/hashString.h>

extern configUint32 confControlMovement;

namespace
{
	uint64 scorePreviousSound;
	uint64 scorePreviousAchievements;

	void shipMovement()
	{
		if (game.cinematic)
			return;

		ENGINE_GET_COMPONENT(transform, tr, game.playerEntity);
		GRID_GET_COMPONENT(velocity, vl, game.playerEntity);

		if (game.moveDirection != vec3())
		{
			real maxSpeed = game.powerups[(uint32)powerupTypeEnum::MaxSpeed] * 0.3 + 0.8;
			vec3 change = game.moveDirection.normalize() * (game.powerups[(uint32)powerupTypeEnum::Acceleration] + 1) * 0.1;
			if (confControlMovement == 1 && ((tr.orientation * vec3(0, 0, -1)).dot(normalize(vl.velocity + change)) < 1e-5))
				vl.velocity = vec3();
			else
				vl.velocity += change;
			if (vl.velocity.squaredLength() > maxSpeed * maxSpeed)
				vl.velocity = vl.velocity.normalize() * maxSpeed;
			if (change.squaredLength() > 0.01)
			{
				entityClass *spark = entities()->createAnonymous();
				ENGINE_GET_COMPONENT(transform, transform, spark);
				transform.scale = randomChance() * 0.2 + 0.3;
				transform.position = tr.position + tr.orientation * vec3((sint32)(statistics.updateIterationIgnorePause % 2) * 1.2 - 0.6, 0, 1) * tr.scale;
				transform.orientation = randomDirectionQuat();
				ENGINE_GET_COMPONENT(render, render, spark);
				render.object = hashString("grid/environment/spark.object");
				GRID_GET_COMPONENT(velocity, vel, spark);
				vel.velocity = (change + randomDirection3() * 0.05) * randomChance() * -5;
				GRID_GET_COMPONENT(timeout, ttl, spark);
				ttl.ttl = randomRange(10, 15);
				ENGINE_GET_COMPONENT(animatedTexture, at, spark);
				at.startTime = currentControlTime();
				at.speed = 30.f / ttl.ttl;
				spark->add(entitiesPhysicsEvenWhenPaused);
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
		game.monstersTarget = tr.position + vl.velocity * 3;
		if (vl.velocity.squaredLength() > 1e-5)
			tr.orientation = quat(degs(), aTan2(-vl.velocity[2], -vl.velocity[0]), degs());
	}

	void shipShield()
	{
		if (!game.playerEntity || !game.shieldEntity)
			return;
		ENGINE_GET_COMPONENT(transform, tr, game.playerEntity);
		ENGINE_GET_COMPONENT(transform, trs, game.shieldEntity);
		trs.position = tr.position;
		trs.scale = tr.scale;
		if (game.powerups[(uint32)powerupTypeEnum::Shield] > 0)
		{
			ENGINE_GET_COMPONENT(render, render, game.shieldEntity);
			render.object = hashString("grid/player/shield.object");
			ENGINE_GET_COMPONENT(voice, sound, game.shieldEntity);
			sound.name = hashString("grid/player/shield.ogg");
			sound.startTime = -1;
		}
		else
		{
			game.shieldEntity->remove(renderComponent::component);
			game.shieldEntity->remove(voiceComponent::component);
		}
	}

	void checkScore(uint32 limit, const string &name)
	{
		if (game.score >= limit && scorePreviousAchievements < limit)
			achievementFullfilled(name);
	}

	void scoreUpdate()
	{
		if (game.jokeMap && game.score >= 10000 && scorePreviousAchievements < 10000)
			achievementFullfilled("joke-map");
		checkScore(  10000, "starting-kit");
		checkScore(  50000, "bronze");
		checkScore( 250000, "silver");
		checkScore(1250000, "gold");
		scorePreviousAchievements = game.score;

		uint64 lg = scorePreviousSound >= 20000 ? 10 : scorePreviousSound >= 2000 ? 2 : 1;
		uint64 sg = lg * 500;
		uint64 ld = (game.score - scorePreviousSound) / sg;
		if (ld)
		{
			scorePreviousSound += ld * sg;

			uint32 sounds[] = {
				hashString("grid/speech/progress/doing-fine.wav"),
				hashString("grid/speech/progress/doing-well.wav"),
				hashString("grid/speech/progress/fantastic.wav"),
				hashString("grid/speech/progress/go-on.wav"),
				hashString("grid/speech/progress/i-hope-the-princess-is-worth-all-the-trouble.wav"),
				hashString("grid/speech/progress/keep-going.wav"),
				hashString("grid/speech/progress/lets-roll.wav"),
				hashString("grid/speech/progress/they-say-the-princess-is-very-beautiful.wav"),
				0
			};
			soundSpeech(sounds);
		}
	}

	bool engineUpdate()
	{
		if (!game.paused)
		{
			shipMovement();
			scoreUpdate();
			if (!game.cinematic && game.life <= 1e-7)
			{
				game.life = 0;
				gameStopEvent().dispatch();
				return true;
			}
		}
		return false;
	}

	void gameStart()
	{
		scorePreviousSound = 0;

		{ // player ship entity
			game.playerEntity = entities()->createUnique();
			ENGINE_GET_COMPONENT(transform, transform, game.playerEntity);
			transform.scale = playerScale;
			ENGINE_GET_COMPONENT(render, render, game.playerEntity);
			render.object = hashString("grid/player/player.object");
			game.monstersTarget = vec3();
		}

		{ // player shield entity
			game.shieldEntity = entities()->createUnique();
			ENGINE_GET_COMPONENT(transform, transform, game.shieldEntity);
			(void)transform;
			ENGINE_GET_COMPONENT(animatedTexture, aniTex, game.shieldEntity);
			aniTex.speed = 0.05;
		}
	}

	void gameStop()
	{
		ENGINE_GET_COMPONENT(transform, playerTransform, game.playerEntity);
		GRID_GET_COMPONENT(velocity, playerVelocity, game.playerEntity);
		environmentExplosion(playerTransform.position, playerVelocity.velocity, playerDeathColor, 20, playerScale);
		for (uint32 i = 0; i < 10; i++)
			environmentExplosion(playerTransform.position, randomDirection3() * vec3(1, 0.1, 1), playerDeathColor, 5, playerScale);
		game.playerEntity->add(entitiesToDestroy);
		game.playerEntity = nullptr;
		game.shieldEntity->add(entitiesToDestroy);
		game.shieldEntity = nullptr;
	}

	class callbacksClass
	{
		eventListener<bool()> engineUpdateListener1;
		eventListener<void()> engineUpdateListener2;
		eventListener<void()> gameStartListener;
		eventListener<void()> gameStopListener;
	public:
		callbacksClass()
		{
			engineUpdateListener1.attach(controlThread().update, -20);
			engineUpdateListener1.bind<&engineUpdate>();
			engineUpdateListener2.attach(controlThread().update, 40); // after physics
			engineUpdateListener2.bind<&shipShield>();
			gameStartListener.attach(gameStartEvent(), -20);
			gameStartListener.bind<&gameStart>();
			gameStopListener.attach(gameStopEvent(), -20);
			gameStopListener.bind<&gameStop>();
		}
	} callbacksInstance;
}
