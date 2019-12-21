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

		CAGE_COMPONENT_ENGINE(transform, tr, game.playerEntity);
		DEGRID_COMPONENT(velocity, vl, game.playerEntity);

		if (game.moveDirection != vec3())
		{
			real maxSpeed = game.powerups[(uint32)powerupTypeEnum::MaxSpeed] * 0.3 + 0.8;
			vec3 change = normalize(game.moveDirection) * (game.powerups[(uint32)powerupTypeEnum::Acceleration] + 1) * 0.1;
			if (confControlMovement == 1 && dot(tr.orientation * vec3(0, 0, -1), normalize(vl.velocity + change)) < 1e-5)
				vl.velocity = vec3();
			else
				vl.velocity += change;
			if (lengthSquared(vl.velocity) > maxSpeed * maxSpeed)
				vl.velocity = normalize(vl.velocity) * maxSpeed;
			if (lengthSquared(change) > 0.01)
			{
				entity *spark = entities()->createAnonymous();
				CAGE_COMPONENT_ENGINE(transform, transform, spark);
				transform.scale = randomChance() * 0.2 + 0.3;
				transform.position = tr.position + tr.orientation * vec3((sint32)(statistics.updateIterationIgnorePause % 2) * 1.2 - 0.6, 0, 1) * tr.scale;
				transform.orientation = randomDirectionQuat();
				CAGE_COMPONENT_ENGINE(render, render, spark);
				render.object = hashString("degrid/environment/spark.object");
				DEGRID_COMPONENT(velocity, vel, spark);
				vel.velocity = (change + randomDirection3() * 0.05) * randomChance() * -5;
				DEGRID_COMPONENT(timeout, ttl, spark);
				ttl.ttl = randomRange(10, 15);
				CAGE_COMPONENT_ENGINE(textureAnimation, at, spark);
				at.startTime = currentControlTime();
				at.speed = 30.f / ttl.ttl;
				spark->add(entitiesPhysicsEvenWhenPaused);
			}
		}
		else
			vl.velocity *= 0.97;

		// pull to center
		if (length(tr.position) > mapNoPullRadius)
		{
			vec3 pullToCenter = -normalize(tr.position) * pow((length(tr.position) - mapNoPullRadius) / mapNoPullRadius, 2);
			vl.velocity += pullToCenter;
		}

		vl.velocity[1] = 0;
		tr.position[1] = 0.5;
		game.monstersTarget = tr.position + vl.velocity * 3;
		if (lengthSquared(vl.velocity) > 1e-5)
			tr.orientation = quat(degs(), atan2(-vl.velocity[2], -vl.velocity[0]), degs());
	}

	void shipShield()
	{
		if (!game.playerEntity || !game.shieldEntity)
			return;
		CAGE_COMPONENT_ENGINE(transform, tr, game.playerEntity);
		CAGE_COMPONENT_ENGINE(transform, trs, game.shieldEntity);
		trs.position = tr.position;
		trs.scale = tr.scale;
		if (game.powerups[(uint32)powerupTypeEnum::Shield] > 0)
		{
			CAGE_COMPONENT_ENGINE(render, render, game.shieldEntity);
			render.object = hashString("degrid/player/shield.object");
			CAGE_COMPONENT_ENGINE(voice, sound, game.shieldEntity);
			sound.name = hashString("degrid/player/shield.ogg");
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
		if (game.jokeMap && game.score >= 5000 && scorePreviousAchievements < 5000)
			achievementFullfilled("joke-map");
		checkScore(   5000, "starting-kit");
		checkScore(  20000, "bronze");
		checkScore( 100000, "silver");
		checkScore(1000000, "gold");
		scorePreviousAchievements = game.score;

		uint64 lg = scorePreviousSound >= 20000 ? 10 : scorePreviousSound >= 2000 ? 2 : 1;
		uint64 sg = lg * 500;
		uint64 ld = (game.score - scorePreviousSound) / sg;
		if (ld)
		{
			scorePreviousSound += ld * sg;

			uint32 sounds[] = {
				hashString("degrid/speech/progress/doing-fine.wav"),
				hashString("degrid/speech/progress/doing-well.wav"),
				hashString("degrid/speech/progress/fantastic.wav"),
				hashString("degrid/speech/progress/go-on.wav"),
				hashString("degrid/speech/progress/i-hope-the-princess-is-worth-all-the-trouble.wav"),
				hashString("degrid/speech/progress/keep-going.wav"),
				hashString("degrid/speech/progress/lets-roll.wav"),
				hashString("degrid/speech/progress/they-say-the-princess-is-very-beautiful.wav"),
				0
			};
			soundSpeech(sounds);
		}
	}

	void engineUpdate()
	{
		OPTICK_EVENT("ship");

		if (!game.paused)
		{
			shipMovement();
			scoreUpdate();
			if (!game.cinematic && game.life <= 1e-7)
			{
				game.life = 0;
				gameStopEvent().dispatch();
			}
		}
	}

	void gameStart()
	{
		scorePreviousSound = 0;

		{ // player ship entity
			game.playerEntity = entities()->createUnique();
			CAGE_COMPONENT_ENGINE(transform, transform, game.playerEntity);
			transform.scale = playerScale;
			CAGE_COMPONENT_ENGINE(render, render, game.playerEntity);
			render.object = hashString("degrid/player/player.object");
			game.monstersTarget = vec3();
		}

		{ // player shield entity
			game.shieldEntity = entities()->createUnique();
			CAGE_COMPONENT_ENGINE(transform, transform, game.shieldEntity);
			(void)transform;
			CAGE_COMPONENT_ENGINE(textureAnimation, aniTex, game.shieldEntity);
			aniTex.speed = 0.05;
		}
	}

	void gameStop()
	{
		CAGE_COMPONENT_ENGINE(transform, playerTransform, game.playerEntity);
		DEGRID_COMPONENT(velocity, playerVelocity, game.playerEntity);
		environmentExplosion(playerTransform.position, playerVelocity.velocity, playerDeathColor, playerScale);
		game.playerEntity->add(entitiesToDestroy);
		game.playerEntity = nullptr;
		game.shieldEntity->add(entitiesToDestroy);
		game.shieldEntity = nullptr;
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener1;
		eventListener<void()> engineUpdateListener2;
		eventListener<void()> gameStartListener;
		eventListener<void()> gameStopListener;
	public:
		callbacksClass() : engineUpdateListener1("ship1"), engineUpdateListener2("ship2"), gameStartListener("ship"), gameStopListener("ship")
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
