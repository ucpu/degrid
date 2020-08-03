#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>

#include "../game.h"

namespace
{
	Entity *primaryCameraEntity;
	Entity *skyboxPrimaryCameraEntity;
	Entity *secondaryCameraEntity;
	Entity *skyboxSecondaryCameraEntity;

	constexpr float AmbientLight = 0.3f;
	constexpr float DirectionalLight = 0;
	constexpr float CameraDistance = 220;

	VariableSmoothingBuffer<vec3, 16> playerPosSmoother;
	VariableSmoothingBuffer<vec3, 16> cameraPosSmoother;
	real cameraFovShakeAmplitude;

	void engineUpdate()
	{
		OPTICK_EVENT("camera");

		if (game.gameOver)
			return;

		// fov (breathing effect)
		cameraFovShakeAmplitude = interpolate(cameraFovShakeAmplitude, (real)BossComponent::component->group()->count(), 0.003);
		rads fov = degs(40 + 0.5 * cage::sin(degs(engineControlTime() * 2.5e-4)) * clamp(cameraFovShakeAmplitude, 0, 1));
		{
			CAGE_COMPONENT_ENGINE(Camera, pc, primaryCameraEntity);
			pc.camera.perspectiveFov = fov;
			CAGE_COMPONENT_ENGINE(Camera, sc, skyboxPrimaryCameraEntity);
			sc.camera.perspectiveFov = fov;
		}

		{ // camera
			CAGE_COMPONENT_ENGINE(Transform, p, game.playerEntity);
			playerPosSmoother.add(p.position);
			CAGE_COMPONENT_ENGINE(Transform, c, primaryCameraEntity);
			quat rot = quat(degs(game.cinematic ? -40 : -90), {}, {});
			vec3 ct = playerPosSmoother.smooth() + rot * vec3(0, 0, CameraDistance);
			cameraPosSmoother.add(ct);
			c.position = cameraPosSmoother.smooth();
			c.orientation = quat(playerPosSmoother.smooth() - c.position, vec3(0, 0, -1));
			CAGE_COMPONENT_ENGINE(Transform, s, skyboxPrimaryCameraEntity);
			s.orientation = c.orientation;
		}

		{ // secondaryCamera
			static ConfigBool secondaryCamera("degrid/secondaryCamera/enabled", false);
			if (secondaryCamera)
			{
				CAGE_COMPONENT_ENGINE(Transform, tp, game.playerEntity);
				{
					CAGE_COMPONENT_ENGINE(Transform, ts, skyboxSecondaryCameraEntity);
					ts.orientation = tp.orientation;
					CAGE_COMPONENT_ENGINE(Camera, c, skyboxSecondaryCameraEntity);
					c.cameraOrder = 3;
					c.sceneMask = 2;
					c.near = 0.5;
					c.far = 3;
					c.camera.perspectiveFov = fov * 1.5;
					c.viewportOrigin = vec2(0.7, 0);
					c.viewportSize = vec2(0.3, 0.3);
				}
				{
					CAGE_COMPONENT_ENGINE(Transform, tc, secondaryCameraEntity);
					tc.position = tp.position;
					tc.orientation = tp.orientation;
					CAGE_COMPONENT_ENGINE(Camera, c, secondaryCameraEntity);
					c.cameraOrder = 4;
					c.sceneMask = 1;
					c.near = 3;
					c.far = 500;
					c.ambientColor = c.ambientDirectionalColor = vec3(1);
					c.ambientIntensity = AmbientLight;
					c.ambientDirectionalIntensity = DirectionalLight;
					c.clear = CameraClearFlags::None;
					c.camera.perspectiveFov = fov * 1.5;
					c.viewportOrigin = vec2(0.7, 0);
					c.viewportSize = vec2(0.3, 0.3);
					c.effects = CameraEffectsFlags::CombinedPass & ~CameraEffectsFlags::AmbientOcclusion;
				}
			}
			else
			{
				skyboxSecondaryCameraEntity->remove(CameraComponent::component);
				secondaryCameraEntity->remove(CameraComponent::component);
			}
		}
	}

	void gameStart()
	{
		{
			skyboxPrimaryCameraEntity = engineEntities()->createUnique();
			CAGE_COMPONENT_ENGINE(Camera, c, skyboxPrimaryCameraEntity);
			c.cameraOrder = 1;
			c.sceneMask = 2;
			c.near = 0.5;
			c.far = 3;
			c.camera.perspectiveFov = degs(40);
		}

		{
			primaryCameraEntity = engineEntities()->createUnique();
			CAGE_COMPONENT_ENGINE(Camera, c, primaryCameraEntity);
			c.cameraOrder = 2;
			c.sceneMask = 1;
			c.near = 150;
			c.far = 1000;
			c.camera.perspectiveFov = degs(40);
			c.ambientColor = c.ambientDirectionalColor = vec3(1);
			c.ambientIntensity = AmbientLight;
			c.ambientDirectionalIntensity = DirectionalLight;
			c.clear = CameraClearFlags::None;
			c.effects = CameraEffectsFlags::CombinedPass & ~CameraEffectsFlags::AmbientOcclusion;
			CAGE_COMPONENT_ENGINE(Listener, ls, primaryCameraEntity);
			constexpr float HalfVolumeDistance = 30;
			ls.attenuation[1] = 2.0 / HalfVolumeDistance;
			ls.attenuation[0] = ls.attenuation[1] * CameraDistance * -1;
		}

		{
			skyboxSecondaryCameraEntity = engineEntities()->createUnique();
			secondaryCameraEntity = engineEntities()->createUnique();
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

