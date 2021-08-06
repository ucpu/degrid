#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>

#include "../game.h"

namespace
{
	Entity *primaryCameraEntity;
	Entity *skyboxPrimaryCameraEntity;

	constexpr float AmbientLight = 0.3f;
	constexpr float DirectionalLight = 0;
	constexpr float CameraDistance = 220;

	VariableSmoothingBuffer<vec3, 16> playerPosSmoother;
	VariableSmoothingBuffer<vec3, 16> cameraPosSmoother;
	real cameraFovShakeAmplitude;

	void engineUpdate()
	{
		if (game.gameOver)
			return;

		// fov (breathing effect)
		cameraFovShakeAmplitude = interpolate(cameraFovShakeAmplitude, (real)engineEntities()->component<BossComponent>()->count(), 0.003);
		rads fov = degs(40 + 0.5 * cage::sin(degs(engineControlTime() * 2.5e-4)) * saturate(cameraFovShakeAmplitude));
		{
			CameraComponent &pc = primaryCameraEntity->value<CameraComponent>();
			pc.camera.perspectiveFov = fov;
			CameraComponent &sc = skyboxPrimaryCameraEntity->value<CameraComponent>();
			sc.camera.perspectiveFov = fov;
		}

		{ // camera
			TransformComponent &p = game.playerEntity->value<TransformComponent>();
			playerPosSmoother.add(p.position);
			TransformComponent &c = primaryCameraEntity->value<TransformComponent>();
			quat rot = quat(degs(game.cinematic ? -40 : -90), {}, {});
			vec3 ct = playerPosSmoother.smooth() + rot * vec3(0, 0, CameraDistance);
			cameraPosSmoother.add(ct);
			c.position = cameraPosSmoother.smooth();
			c.orientation = quat(playerPosSmoother.smooth() - c.position, vec3(0, 0, -1));
			TransformComponent &s = skyboxPrimaryCameraEntity->value<TransformComponent>();
			s.orientation = c.orientation;
		}
	}

	void gameStart()
	{
		{
			skyboxPrimaryCameraEntity = engineEntities()->createUnique();
			CameraComponent &c = skyboxPrimaryCameraEntity->value<CameraComponent>();
			c.cameraOrder = 1;
			c.sceneMask = 2;
			c.near = 0.5;
			c.far = 3;
			c.camera.perspectiveFov = degs(40);
		}

		{
			primaryCameraEntity = engineEntities()->createUnique();
			CameraComponent &c = primaryCameraEntity->value<CameraComponent>();
			c.cameraOrder = 2;
			c.sceneMask = 1;
			c.near = 150;
			c.far = 1000;
			c.camera.perspectiveFov = degs(40);
			c.ambientColor = c.ambientDirectionalColor = vec3(1);
			c.ambientIntensity = AmbientLight;
			c.ambientDirectionalIntensity = DirectionalLight;
			c.clear = CameraClearFlags::None;
			c.effects = CameraEffectsFlags::Default & ~CameraEffectsFlags::AmbientOcclusion;
			ListenerComponent &ls = primaryCameraEntity->value<ListenerComponent>();
			ls.rolloffFactor = 0.5 / CameraDistance;
		}
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
		EventListener<void()> gameStartListener;
	public:
		Callbacks() : engineUpdateListener("camera"), gameStartListener("camera")
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

