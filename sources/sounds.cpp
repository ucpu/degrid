#include "game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/assets.h>
#include <cage-core/utility/spatial.h>
#include <cage-core/utility/hashString.h>

#include <cage-client/sound.h>

extern configFloat confVolumeMusic;
extern configFloat confVolumeEffects;
extern configFloat confVolumeSpeech;

globalSoundsStruct sounds;

namespace
{
	struct soundDataStruct
	{
		holder<busClass> suspenseBus;
		holder<busClass> actionBus;
		holder<busClass> endBus;

		holder<volumeFilterClass> suspenseVolume;
		holder<volumeFilterClass> actionVolume;
		holder<volumeFilterClass> endVolume;

		bool suspenseLoaded;
		bool actionLoaded;
		bool endLoaded;

		holder<volumeFilterClass> musicVolume;
		holder<volumeFilterClass> effectsVolume;

		holder<busClass> speechBus;
		holder<volumeFilterClass> speechVolume;
		holder<filterClass> speechFilter;
		uint64 speechStart;
		uint32 speechName;

		soundDataStruct() : suspenseLoaded(false), actionLoaded(false), endLoaded(false), speechStart(0), speechName(0) {}

		void speechExecute(const filterApiStruct &api)
		{
			soundDataBufferStruct s = api.output;
			s.time -= (sint64)speechStart;
			api.input(s);
		}
	};
	soundDataStruct *data;

	void alterVolume(real &current, real target)
	{
		const real change = 0.7f / (1000000 / soundThread().timePerTick);
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

	void alterVolume(holder<volumeFilterClass> &current, real target)
	{
		alterVolume(current->volume, target);
	}

	void soundsInit()
	{
		data = detail::systemArena().createObject<soundDataStruct>();
#define GCHL_GENERATE(NAME) \
		data->CAGE_JOIN(NAME, Bus) = newBus(sound()); \
		data->CAGE_JOIN(NAME, Bus)->addOutput(musicMixer()); \
		data->CAGE_JOIN(NAME, Volume) = newFilterVolume(sound()); \
		data->CAGE_JOIN(NAME, Volume)->filter->setBus(data->CAGE_JOIN(NAME, Bus).get()); \
		data->CAGE_JOIN(NAME, Volume)->volume = 0;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, suspense, action, end));
#undef GCHL_GENERATE

		data->musicVolume = newFilterVolume(sound());
		data->musicVolume->filter->setBus(musicMixer());
		data->effectsVolume = newFilterVolume(sound());
		data->effectsVolume->filter->setBus(effectsMixer());

		data->speechBus = newBus(sound());
		data->speechVolume = newFilterVolume(sound());
		data->speechFilter = newFilter(sound());
		data->speechFilter->execute.bind<soundDataStruct, &soundDataStruct::speechExecute>(data);
	}

	void soundsDone()
	{
		detail::systemArena().destroy<soundDataStruct>(data);
	}

	void destroyOutdatedSouns()
	{
		statistics.soundEffectsCurrent = 0;
		uint64 time = currentControlTime() - 2000000;
		for (entityClass *e : voiceComponent::component->getComponentEntities()->entities())
		{
			ENGINE_GET_COMPONENT(voice, s, e);
			if (s.name == hashString("grid/player/shield.ogg"))
				continue;
			if (!assets()->ready(s.name))
				continue;
			sourceClass *src = assets()->get<assetSchemeIndexSound, sourceClass>(s.name);
			if (src && s.startTime + src->getDuration() + 100000 < time)
				e->addGroup(entitiesToDestroy);
			statistics.soundEffectsCurrent++;
		}
		statistics.soundEffectsMax = max(statistics.soundEffectsMax, statistics.soundEffectsCurrent);
	}

	void soundUpdate()
	{
		data->musicVolume->volume = (float)confVolumeMusic;
		data->effectsVolume->volume = (float)confVolumeEffects;
		data->speechVolume->volume = (float)confVolumeSpeech;

		static const uint32 suspenseName = hashString("grid/music/fear-and-horror.ogg");
		static const uint32 actionName = hashString("grid/music/chaotic-filth.ogg");
		static const uint32 endName = hashString("grid/music/sad-song.ogg");
#define GCHL_GENERATE(NAME) if (!data->CAGE_JOIN(NAME, Loaded) && assets()->state(CAGE_JOIN(NAME, Name)) == assetStateEnum::Ready) { assets()->get<assetSchemeIndexSound, sourceClass>(CAGE_JOIN(NAME, Name))->addOutput(data->CAGE_JOIN(NAME, Bus).get()); data->CAGE_JOIN(NAME, Volume)->volume = 0; data->CAGE_JOIN(NAME, Loaded) = true; }
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, suspense, action, end));
#undef GCHL_GENERATE

		if (data->speechName)
		{
			sourceClass *s = assets()->get<assetSchemeIndexSound, sourceClass>(data->speechName);
			if (s)
			{
				uint64 d = s->getDuration();
				if (data->speechStart + d + 100000 < currentControlTime())
				{
					data->speechBus->clear();
					data->speechName = 0;
				}
				else
				{
					data->speechBus->addOutput(masterMixer());
					data->speechVolume->filter->setBus(data->speechBus.get());
					data->speechFilter->setBus(data->speechBus.get());
					s->addOutput(data->speechBus.get());
				}
			}
		}

		if (player.gameOver)
		{
			alterVolume(data->suspenseVolume, 0);
			alterVolume(data->actionVolume, 0);
			alterVolume(data->endVolume, 1);
			return;
		}

		real f = sounds.suspense; // < 0.5 ? 0 : 1;
		alterVolume(data->suspenseVolume, f);
		alterVolume(data->actionVolume, 1 - f);
		alterVolume(data->endVolume, 0);
	}

	void musicUpdate()
	{
		if (player.cinematic)
		{
			sounds.suspense = 0;
			return;
		}
		if (player.paused)
		{
			sounds.suspense = 1;
			return;
		}

		static const real distMin = 25;
		static const real distMax = 35;
		ENGINE_GET_COMPONENT(transform, playerTransform, player.playerEntity);
		real closestMonsterToPlayer = real::PositiveInfinity;
		spatialQuery->intersection(sphere(playerTransform.position, distMax));
		for (uint32 otherName : spatialQuery->result())
		{
			if (!entities()->hasEntity(otherName))
				continue;
			entityClass *e = entities()->getEntity(otherName);
			if (e->hasComponent(monsterComponent::component))
			{
				ENGINE_GET_COMPONENT(transform, p, e);
				real d = p.position.distance(playerTransform.position);
				closestMonsterToPlayer = min(closestMonsterToPlayer, d);
			}
		}
		closestMonsterToPlayer = clamp(closestMonsterToPlayer, distMin, distMax);
		sounds.suspense = (closestMonsterToPlayer - distMin) / (distMax - distMin);
	}

	void gameStart()
	{
		if (!player.cinematic)
		{
			uint32 sounds[] = {
				hashString("grid/speech/starts/enemies-are-approaching.wav"),
				hashString("grid/speech/starts/enemy-is-approaching.wav"),
				hashString("grid/speech/starts/its-a-trap.wav"),
				hashString("grid/speech/starts/lets-do-this.wav"),
				hashString("grid/speech/starts/let-us-kill-some-.wav"),
				hashString("grid/speech/starts/ready-set-go.wav"),
				0
			};
			soundSpeech(sounds);
		}
	}

	void gameStop()
	{
		uint32 sounds[] = {
			hashString("grid/speech/over/game-over.wav"),
			hashString("grid/speech/over/lets-try-again.wav"),
			hashString("grid/speech/over/oh-no.wav"),
			hashString("grid/speech/over/pitty.wav"),
			hashString("grid/speech/over/thats-it.wav"),
			0 };
		soundSpeech(sounds);
	}
}

void soundEffect(uint32 sound, const vec3 &position)
{
	entityClass *e = entities()->newUniqueEntity();
	ENGINE_GET_COMPONENT(transform, t, e);
	t.position = position;
	ENGINE_GET_COMPONENT(voice, s, e);
	s.name = sound;
	s.startTime = currentControlTime();
}

void soundSpeech(uint32 sound)
{
	if (data->speechName)
		return;
	data->speechName = sound;
	data->speechStart = currentControlTime() + 10000;
}

void soundSpeech(uint32 sounds[])
{
	uint32 *p = sounds;
	uint32 count = 0;
	while (*p++) count++;
	soundSpeech(sounds[random(0u, count)]);
}

