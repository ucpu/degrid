#include "includes.h"
#include "game.h"

namespace grid
{
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
			const real change = 0.7f / (1000000 / soundThread::tickTime);
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
				if (data->speechStart + d + 100000 < getApplicationTime())
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

	void soundEffect(uint32 sound, const vec3 &position)
	{
		entityClass *e = entities()->newEntity(entities()->generateUniqueName());
		ENGINE_GET_COMPONENT(transform, t, e);
		t.position = position;
		ENGINE_GET_COMPONENT(voice, s, e);
		s.sound = sound;
		s.soundStart = getApplicationTime();
	}

	void soundSpeech(uint32 sound)
	{
		if (data->speechName)
			return;
		data->speechName = sound;
		data->speechStart = getApplicationTime() + 10000;
	}

	void soundSpeech(uint32 *sounds)
	{
		uint32 *p = sounds;
		uint32 count = 0;
		while (*p++) count++;
		soundSpeech(sounds[random(0, count)]);
	}
}