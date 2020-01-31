#include "game.h"

#include <cage-core/geometry.h>
#include <cage-core/entities.h>
#include <cage-core/config.h>
#include <cage-core/assetManager.h>
#include <cage-core/spatial.h>
#include <cage-core/hashString.h>
#include <cage-core/macros.h>

#include <cage-engine/sound.h>

#include <atomic>

extern ConfigFloat confVolumeMusic;
extern ConfigFloat confVolumeEffects;
extern ConfigFloat confVolumeSpeech;

namespace
{
	real suspense;

	struct SoundData
	{
		Holder<MixingBus> suspenseBus;
		Holder<MixingBus> actionBus;
		Holder<MixingBus> endBus;

		Holder<VolumeFilter> suspenseVolume;
		Holder<VolumeFilter> actionVolume;
		Holder<VolumeFilter> endVolume;

		bool suspenseLoaded;
		bool actionLoaded;
		bool endLoaded;

		Holder<VolumeFilter> musicVolume;
		Holder<VolumeFilter> effectsVolume;

		// this is a bit of a hack. the Engine will have to be improved to handle situations like these
		// Engine master bus <- bus 1 <- volume filter <- filter (speechCallback) <- bus 2 <- bus 3 <- source
		// problem is, that the filter cannot modify its inputs (bus 2) of its own bus (bus 1)
		// but it may modify bus 3 :D
		Holder<MixingBus> speechBus1;
		Holder<MixingBus> speechBus2;
		Holder<MixingBus> speechBus3;
		Holder<VolumeFilter> speechVolume;
		Holder<MixingFilter> speechFilter;
		uint64 speechStart;
		std::atomic<uint32> speechName;

		SoundData() : suspenseLoaded(false), actionLoaded(false), endLoaded(false), speechStart(0), speechName(0)
		{}
	};
	SoundData *data;

	void alterVolume(real &current, real target)
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

	void alterVolume(Holder<VolumeFilter> &current, real target)
	{
		alterVolume(current->volume, target);
	}

	void musicUpdate()
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

		static const real distMin = 30;
		static const real distMax = 60;
		CAGE_COMPONENT_ENGINE(Transform, playerTransform, game.playerEntity);
		real closestMonsterToPlayer = real::Infinity();
		SpatialSearchQuery->intersection(sphere(playerTransform.position, distMax));
		for (uint32 otherName : SpatialSearchQuery->result())
		{
			Entity *e = engineEntities()->get(otherName);
			if (e->has(MonsterComponent::component))
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

	void speechCallback(const MixingFilterApi &api)
	{
		// this function is called in another thread!
		data->speechBus3->clear();
		if (!data->speechName)
			return;
		Holder<SoundSource> src = engineAssets()->get<AssetSchemeIndexSoundSource, SoundSource>(data->speechName);
		if (!src)
			return;
		if (data->speechStart + src->getDuration() + 100000 < engineControlTime())
		{
			data->speechName = 0;
			return;
		}
		src->addOutput(data->speechBus3.get());
		data->speechBus3->addOutput(data->speechBus2.get());
		SoundDataBuffer s = api.output;
		s.time -= (sint64)data->speechStart;
		api.input(s);
	}

	void soundUpdate()
	{
		data->musicVolume->volume = (float)confVolumeMusic;
		data->effectsVolume->volume = (float)confVolumeEffects;
		data->speechVolume->volume = (float)confVolumeSpeech;

		static const uint32 suspenseName = HashString("degrid/music/fear-and-horror.ogg");
		static const uint32 actionName = HashString("degrid/music/chaotic-filth.ogg");
		static const uint32 endName = HashString("degrid/music/sad-song.ogg");
#define GCHL_GENERATE(NAME) if (!data->CAGE_JOIN(NAME, Loaded)) { Holder<SoundSource> src = engineAssets()->get<AssetSchemeIndexSoundSource, SoundSource>(CAGE_JOIN(NAME, Name)); if (!src) return; src->addOutput(data->CAGE_JOIN(NAME, Bus).get()); data->CAGE_JOIN(NAME, Volume)->volume = 0; data->CAGE_JOIN(NAME, Loaded) = true; }
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, suspense, action, end));
#undef GCHL_GENERATE

		if (game.gameOver)
		{
			alterVolume(data->suspenseVolume, 0);
			alterVolume(data->actionVolume, 0);
			alterVolume(data->endVolume, 1);
			return;
		}

		alterVolume(data->suspenseVolume, suspense);
		alterVolume(data->actionVolume, 1 - suspense);
		alterVolume(data->endVolume, 0);
	}

	void engineInit()
	{
		data = detail::systemArena().createObject<SoundData>();
#define GCHL_GENERATE(NAME) \
		data->CAGE_JOIN(NAME, Bus) = newMixingBus(engineSound()); \
		data->CAGE_JOIN(NAME, Bus)->addOutput(engineMusicMixer()); \
		data->CAGE_JOIN(NAME, Volume) = newVolumeFilter(engineSound()); \
		data->CAGE_JOIN(NAME, Volume)->filter->setBus(data->CAGE_JOIN(NAME, Bus).get()); \
		data->CAGE_JOIN(NAME, Volume)->volume = 0;
		CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(GCHL_GENERATE, suspense, action, end));
#undef GCHL_GENERATE

		data->musicVolume = newVolumeFilter(engineSound());
		data->musicVolume->filter->setBus(engineMusicMixer());
		data->effectsVolume = newVolumeFilter(engineSound());
		data->effectsVolume->filter->setBus(engineEffectsMixer());

		data->speechBus1 = newMixingBus(engineSound());
		data->speechBus1->addOutput(engineMasterMixer());
		data->speechVolume = newVolumeFilter(engineSound());
		data->speechVolume->filter->setBus(data->speechBus1.get());
		data->speechVolume->volume = 0;
		data->speechFilter = newMixingFilter(engineSound());
		data->speechFilter->setBus(data->speechBus1.get());
		data->speechFilter->execute.bind<speechCallback>();
		data->speechBus2 = newMixingBus(engineSound());
		data->speechBus2->addOutput(data->speechBus1.get());
		data->speechBus3 = newMixingBus(engineSound());
	}

	void engineUpdate()
	{
		OPTICK_EVENT("sound");
		musicUpdate();
		soundUpdate();
	}

	void engineFin()
	{
		detail::systemArena().destroy<SoundData>(data);
	}

	void gameStart()
	{
		if (!game.cinematic)
		{
			uint32 sounds[] = {
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
			soundSpeech(sounds);
		}
	}

	void gameStop()
	{
		uint32 sounds[] = {
			HashString("degrid/speech/gameover/game-over.wav"),
			HashString("degrid/speech/gameover/lets-try-again.wav"),
			HashString("degrid/speech/gameover/oh-no.wav"),
			HashString("degrid/speech/gameover/pitty.wav"),
			HashString("degrid/speech/gameover/thats-it.wav"),
			0
		};
		soundSpeech(sounds);
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
		EventListener<void()> engineFinListener;
		EventListener<void()> gameStartListener;
		EventListener<void()> gameStopListener;
	public:
		Callbacks() : engineInitListener("sounds"), engineUpdateListener("sounds"), engineFinListener("sounds"), gameStartListener("sounds"), gameStopListener("sounds")
		{
			engineInitListener.attach(controlThread().initialize, -55);
			engineInitListener.bind<&engineInit>();
			engineUpdateListener.attach(controlThread().update, -55);
			engineUpdateListener.bind<&engineUpdate>();
			engineFinListener.attach(controlThread().finalize, -55);
			engineFinListener.bind<&engineFin>();
			gameStartListener.attach(gameStartEvent(), -55);
			gameStartListener.bind<&gameStart>();
			gameStopListener.attach(gameStopEvent(), -55);
			gameStopListener.bind<&gameStop>();
		}
	} callbacksInstance;
}

void soundEffect(uint32 sound, const vec3 &position)
{
	Holder<SoundSource> src = engineAssets()->get<AssetSchemeIndexSoundSource, SoundSource>(sound);
	if (!src)
		return;
	Entity *e = engineEntities()->createUnique();
	CAGE_COMPONENT_ENGINE(Transform, t, e);
	t.position = position;
	CAGE_COMPONENT_ENGINE(Sound, s, e);
	s.name = sound;
	s.startTime = engineControlTime();
	DEGRID_COMPONENT(Timeout, ttl, e);
	ttl.ttl = numeric_cast<uint32>((src->getDuration() + 100000) / controlThread().updatePeriod());
	e->add(entitiesPhysicsEvenWhenPaused);
}

void soundSpeech(uint32 sound)
{
	if (data->speechName)
		return;
	data->speechName = sound;
	data->speechStart = engineControlTime() + 10000;
}

void soundSpeech(uint32 sounds[])
{
	uint32 *p = sounds;
	uint32 count = 0;
	while (*p++)
		count++;
	soundSpeech(sounds[randomRange(0u, count)]);
}
