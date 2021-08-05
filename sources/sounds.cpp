#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/assetManager.h>
#include <cage-core/spatialStructure.h>
#include <cage-core/hashString.h>
#include <cage-engine/sound.h>

#include "game.h"

extern ConfigFloat confVolumeMusic;
extern ConfigFloat confVolumeEffects;
extern ConfigFloat confVolumeSpeech;

namespace
{
	real suspense;
	real suspenseVolume;
	real actionVolume;
	real endVolume;
	Entity *suspenseEnt;
	Entity *actionEnt;
	Entity *endEnt;

	void determineSuspense()
	{
		if (game.cinematic)
		{
			suspense = 0;
			return;
		}
		if (game.paused)
		{
			suspense = 1;
			return;
		}

		constexpr const float distMin = 30;
		constexpr const float distMax = 60;
		CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
		real closestMonsterToPlayer = real::Infinity();
		spatialSearchQuery->intersection(Sphere(playerTransform.position, distMax));
		for (uint32 otherName : spatialSearchQuery->result())
		{
			Entity *e = engineEntities()->get(otherName);
			if (e->has<MonsterComponent>())
			{
				CAGE_COMPONENT_ENGINE(Transform, p, e);
				real d = distance(p.position, playerTransform.position);
				closestMonsterToPlayer = min(closestMonsterToPlayer, d);
			}
		}
		// hysteresis
		if (closestMonsterToPlayer < distMin)
			suspense = 0;
		else if (closestMonsterToPlayer > distMax)
			suspense = 1;
	}

	void alterVolume(real& current, real target)
	{
		real change = 0.7f / (1000000 / controlThread().updatePeriod());
		if (current > target + change)
		{
			current -= change;
			return;
		}
		if (current < target - change)
		{
			current += change;
			return;
		}
		current = target;
	}

	void determineVolumes()
	{
		if (game.gameOver)
		{
			alterVolume(suspenseVolume, 0);
			alterVolume(actionVolume, 0);
			alterVolume(endVolume, 1);
			return;
		}

		alterVolume(suspenseVolume, suspense);
		alterVolume(actionVolume, 1 - suspense);
		alterVolume(endVolume, 0);
	}

	void engineUpdate()
	{
		determineSuspense();
		determineVolumes();

		suspenseEnt->value<SoundComponent>().gain = suspenseVolume * (real)confVolumeMusic;
		actionEnt->value<SoundComponent>().gain = actionVolume * (real)confVolumeMusic;
		endEnt->value<SoundComponent>().gain = endVolume * (real)confVolumeMusic;
	}

	Entity *initEntity(const uint32 soundName)
	{
		Entity *e = engineEntities()->createUnique();
		CAGE_COMPONENT_ENGINE(Sound, s, e);
		s.gain = 0;
		s.name = soundName;
		return e;
	}

	void gameStart()
	{
		suspenseEnt = initEntity(HashString("degrid/music/fear-and-horror.ogg"));
		actionEnt = initEntity(HashString("degrid/music/chaotic-filth.ogg"));
		endEnt = initEntity(HashString("degrid/music/sad-song.ogg"));

		if (!game.cinematic)
		{
			constexpr const uint32 Sounds[] = {
				HashString("degrid/speech/starts/a-princess-needs-our-help.wav"),
				HashString("degrid/speech/starts/enemy-is-approaching.wav"),
				HashString("degrid/speech/starts/its-a-trap.wav"),
				HashString("degrid/speech/starts/let-the-journey-begins.wav"),
				HashString("degrid/speech/starts/our-destiny-awaits.wav"),
				HashString("degrid/speech/starts/enemies-are-approaching.wav"),
				HashString("degrid/speech/starts/it-is-our-duty-to-save-the-princess.wav"),
				HashString("degrid/speech/starts/lets-do-this.wav"),
				HashString("degrid/speech/starts/let-us-kill-some-triangles.wav"),
				HashString("degrid/speech/starts/ready-set-go.wav"),
				0
			};
			soundSpeech(Sounds);
		}
	}

	void gameStop()
	{
		constexpr const uint32 Sounds[] = {
			HashString("degrid/speech/gameover/game-over.wav"),
			HashString("degrid/speech/gameover/lets-try-again.wav"),
			HashString("degrid/speech/gameover/oh-no.wav"),
			HashString("degrid/speech/gameover/pitty.wav"),
			HashString("degrid/speech/gameover/thats-it.wav"),
			0
		};
		soundSpeech(Sounds);
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
		EventListener<void()> gameStartListener;
		EventListener<void()> gameStopListener;
	public:
		Callbacks() : engineUpdateListener("sounds"), gameStartListener("sounds"), gameStopListener("sounds")
		{
			engineUpdateListener.attach(controlThread().update, 55);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), 55);
			gameStartListener.bind<&gameStart>();
			gameStopListener.attach(gameStopEvent(), 55);
			gameStopListener.bind<&gameStop>();
		}
	} callbacksInstance;
}

void soundEffect(uint32 soundName, const vec3 &position)
{
	Holder<Sound> src = engineAssets()->get<AssetSchemeIndexSound, Sound>(soundName);
	if (!src)
		return;
	Entity *e = engineEntities()->createUnique();
	CAGE_COMPONENT_ENGINE(Transform, t, e);
	t.position = position;
	CAGE_COMPONENT_ENGINE(Sound, s, e);
	s.name = soundName;
	s.startTime = engineControlTime();
	s.gain = (real)confVolumeEffects;
	DEGRID_COMPONENT(Timeout, ttl, e);
	ttl.ttl = numeric_cast<uint32>((src->duration() + 100000) / controlThread().updatePeriod());
	e->add(entitiesPhysicsEvenWhenPaused);
}

void soundSpeech(uint32 soundName)
{
	Holder<Sound> src = engineAssets()->get<AssetSchemeIndexSound, Sound>(soundName);
	if (!src)
		return;
	Entity* e = engineEntities()->createUnique();
	CAGE_COMPONENT_ENGINE(Sound, s, e);
	s.name = soundName;
	s.startTime = engineControlTime();
	s.gain = (real)confVolumeSpeech;
	DEGRID_COMPONENT(Timeout, ttl, e);
	ttl.ttl = numeric_cast<uint32>((src->duration() + 100000) / controlThread().updatePeriod());
	e->add(entitiesPhysicsEvenWhenPaused);
}

void soundSpeech(const uint32 sounds[])
{
	const uint32 *p = sounds;
	uint32 count = 0;
	while (*p++)
		count++;
	soundSpeech(sounds[randomRange(0u, count)]);
}
