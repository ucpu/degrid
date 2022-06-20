#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>

#include "../game.h"

namespace
{
	Entity *primaryCameraEntity;

	constexpr float AmbientLight = 0.3f;
	constexpr float DirectionalLight = 0;
	constexpr float CameraDistance = 220;

	VariableSmoothingBuffer<Vec3, 16> playerPosSmoother;
	VariableSmoothingBuffer<Vec3, 16> cameraPosSmoother;
	Real cameraFovShakeAmplitude;

	void engineUpdate()
	{
		if (game.gameOver)
			return;

		// fov (breathing effect)
		cameraFovShakeAmplitude = interpolate(cameraFovShakeAmplitude, (Real)engineEntities()->component<BossComponent>()->count(), 0.003);
		const Rads fov = Degs(40 + 0.5 * cage::sin(Degs(engineControlTime() * 2.5e-4)) * saturate(cameraFovShakeAmplitude));
		primaryCameraEntity->value<CameraComponent>().camera.perspectiveFov = fov;

		{ // camera
			playerPosSmoother.add(game.playerEntity->value<TransformComponent>().position);
			TransformComponent &c = primaryCameraEntity->value<TransformComponent>();
			const Quat rot = Quat(Degs(game.cinematic ? -40 : -90), {}, {});
			const Vec3 ct = playerPosSmoother.smooth() + rot * Vec3(0, 0, CameraDistance);
			cameraPosSmoother.add(ct);
			c.position = cameraPosSmoother.smooth();
			c.orientation = Quat(playerPosSmoother.smooth() - c.position, Vec3(0, 0, -1));
		}
	}

	void gameStart()
	{
		primaryCameraEntity = engineEntities()->createUnique();
		CameraComponent &c = primaryCameraEntity->value<CameraComponent>();
		c.near = 150;
		c.far = 1000;
		c.camera.perspectiveFov = Degs(40);
		c.ambientColor = c.ambientDirectionalColor = Vec3(1);
		c.ambientIntensity = AmbientLight;
		c.ambientDirectionalIntensity = DirectionalLight;
		c.effects = CameraEffectsFlags::Default & ~CameraEffectsFlags::AmbientOcclusion;
		primaryCameraEntity->value<ListenerComponent>().rolloffFactor = 0.5 / CameraDistance;
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
		EventListener<void()> gameStartListener;
	public:
		Callbacks()
		{
			engineUpdateListener.attach(controlThread().update, 50);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), 50);
			gameStartListener.bind<&gameStart>();
		}
	} callbacksInstance;
}

Entity *getPrimaryCameraEntity()
{
	return primaryCameraEntity;
}

