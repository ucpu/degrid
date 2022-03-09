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
	Real suspense;
	Real suspenseVolume;
	Real actionVolume;
	Real endVolume;
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
		TransformComponent &playerTransform = game.playerEntity->value<TransformComponent>();
		Real closestMonsterToPlayer = Real::Infinity();
		spatialSearchQuery->intersection(Sphere(playerTransform.position, distMax));
		for (uint32 otherName : spatialSearchQuery->result())
		{
			Entity *e = engineEntities()->get(otherName);
			if (e->has<MonsterComponent>())
			{
				TransformComponent &p = e->value<TransformComponent>();
				Real d = distance(p.position, playerTransform.position);
				closestMonsterToPlayer = min(closestMonsterToPlayer, d);
			}
		}
		// hysteresis
		if (closestMonsterToPlayer < distMin)
			suspense = 0;
		else if (closestMonsterToPlayer > distMax)
			suspense = 1;
	}

	void alterVolume(Real& current, Real target)
	{
		Real change = 0.7f / (1000000 / controlThread().updatePeriod());
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

		suspenseEnt->value<SoundComponent>().gain = suspenseVolume * (Real)confVolumeMusic;
		actionEnt->value<SoundComponent>().gain = actionVolume * (Real)confVolumeMusic;
		endEnt->value<SoundComponent>().gain = endVolume * (Real)confVolumeMusic;
	}

	Entity *initEntity(const uint32 soundName)
	{
		Entity *e = engineEntities()->createUnique();
		SoundComponent &s = e->value<SoundComponent>();
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
		Callbacks()
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

void soundEffect(uint32 soundName, const Vec3 &position)
{
	Holder<Sound> src = engineAssets()->get<AssetSchemeIndexSound, Sound>(soundName);
	if (!src)
		return;
	Entity *e = engineEntities()->createUnique();
	TransformComponent &t = e->value<TransformComponent>();
	t.position = position;
	SoundComponent &s = e->value<SoundComponent>();
	s.name = soundName;
	s.startTime = engineControlTime();
	s.gain = (Real)confVolumeEffects;
	TimeoutComponent &ttl = e->value<TimeoutComponent>();
	ttl.ttl = numeric_cast<uint32>((src->duration() + 100000) / controlThread().updatePeriod());
	e->add(entitiesPhysicsEvenWhenPaused);
}

void soundSpeech(uint32 soundName)
{
	Holder<Sound> src = engineAssets()->get<AssetSchemeIndexSound, Sound>(soundName);
	if (!src)
		return;
	Entity* e = engineEntities()->createUnique();
	SoundComponent &s = e->value<SoundComponent>();
	s.name = soundName;
	s.startTime = engineControlTime();
	s.gain = (Real)confVolumeSpeech;
	TimeoutComponent &ttl = e->value<TimeoutComponent>();
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
