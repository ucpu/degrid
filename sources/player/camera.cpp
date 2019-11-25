#include "../game.h"

#include <cage-core/config.h>
#include <cage-core/entities.h>
#include <cage-core/hashString.h>
#include <cage-core/variableSmoothingBuffer.h>

namespace
{
	entity *primaryCameraEntity;
	entity *skyboxPrimaryCameraEntity;
	entity *secondaryCameraEntity;
	entity *skyboxSecondaryCameraEntity;

	const vec3 ambientLight = vec3(0.1);
	const vec3 directionalLight = vec3(1);
	const real cameraDistance = 220;

	variableSmoothingBuffer<vec3, 8> cameraSmoother;

	void engineUpdate()
	{
		OPTICK_EVENT("camera");

		if (game.gameOver)
			return;

		{ // camera
			CAGE_COMPONENT_ENGINE(transform, tr, primaryCameraEntity);
			CAGE_COMPONENT_ENGINE(transform, p, game.playerEntity);
			cameraSmoother.add(p.position + vec3(0, cameraDistance, 0));
			tr.position = cameraSmoother.oldest();
			tr.orientation = quat(p.position - tr.position, vec3(0, 0, -1));
		}

		{ // secondaryCamera
			static configBool secondaryCamera("degrid.secondary-camera.enabled", false);
			if (secondaryCamera)
			{
				CAGE_COMPONENT_ENGINE(transform, tp, game.playerEntity);
				{
					CAGE_COMPONENT_ENGINE(transform, ts, skyboxSecondaryCameraEntity);
					ts.orientation = tp.orientation;
					CAGE_COMPONENT_ENGINE(camera, c, skyboxSecondaryCameraEntity);
					c.cameraOrder = 3;
					c.sceneMask = 2;
					c.near = 0.5;
					c.far = 3;
					c.camera.perspectiveFov = degs(60);
					c.viewportOrigin = vec2(0.7, 0);
					c.viewportSize = vec2(0.3, 0.3);
				}
				{
					CAGE_COMPONENT_ENGINE(transform, tc, secondaryCameraEntity);
					tc.position = tp.position;
					tc.orientation = tp.orientation;
					CAGE_COMPONENT_ENGINE(camera, c, secondaryCameraEntity);
					c.cameraOrder = 4;
					c.sceneMask = 1;
					c.near = 3;
					c.far = 500;
					c.ambientLight = ambientLight;
					c.ambientDirectionalLight = directionalLight;
					c.clear = (cameraClearFlags)0;
					c.camera.perspectiveFov = degs(60);
					c.viewportOrigin = vec2(0.7, 0);
					c.viewportSize = vec2(0.3, 0.3);
					c.effects = cameraEffectsFlags::CombinedPass & ~cameraEffectsFlags::AmbientOcclusion;
				}
			}
			else
			{
				skyboxSecondaryCameraEntity->remove(cameraComponent::component);
				secondaryCameraEntity->remove(cameraComponent::component);
			}
		}
	}

	void gameStart()
	{
		{
			skyboxPrimaryCameraEntity = entities()->createUnique();
			CAGE_COMPONENT_ENGINE(transform, transform, skyboxPrimaryCameraEntity);
			transform.orientation = quat(degs(-90), degs(), degs());
			CAGE_COMPONENT_ENGINE(camera, c, skyboxPrimaryCameraEntity);
			c.cameraOrder = 1;
			c.sceneMask = 2;
			c.near = 0.5;
			c.far = 3;
			c.camera.perspectiveFov = degs(40);
		}

		{
			primaryCameraEntity = entities()->createUnique();
			CAGE_COMPONENT_ENGINE(transform, transform, primaryCameraEntity);
			transform.orientation = quat(degs(-90), degs(), degs());
			transform.position = vec3(0, cameraDistance, 0);
			cameraSmoother.seed(transform.position);
			CAGE_COMPONENT_ENGINE(camera, c, primaryCameraEntity);
			c.cameraOrder = 2;
			c.sceneMask = 1;
			c.near = 150;
			c.far = 1000;
			c.camera.perspectiveFov = degs(40);
			c.ambientLight = ambientLight;
			c.ambientDirectionalLight = directionalLight;
			c.clear = (cameraClearFlags)0;
			c.effects = cameraEffectsFlags::CombinedPass & ~cameraEffectsFlags::AmbientOcclusion;
			CAGE_COMPONENT_ENGINE(listener, ls, primaryCameraEntity);
			static const float halfVolumeDistance = 30;
			ls.attenuation[1] = 2.0 / halfVolumeDistance;
			ls.attenuation[0] = ls.attenuation[1] * transform.position[1] * -1;
		}

		{
			skyboxSecondaryCameraEntity = entities()->createUnique();
			secondaryCameraEntity = entities()->createUnique();
		}
	}

	class callbacksClass
	{
		eventListener<void()> engineUpdateListener;
		eventListener<void()> gameStartListener;
	public:
		callbacksClass() : engineUpdateListener("camera"), gameStartListener("camera")
		{
			engineUpdateListener.attach(controlThread().update, 50);
			engineUpdateListener.bind<&engineUpdate>();
			gameStartListener.attach(gameStartEvent(), 50);
			gameStartListener.bind<&gameStart>();
		}
	} callbacksInstance;
}

entity *getPrimaryCameraEntity()
{
	return primaryCameraEntity;
}

