#include "SoftVR_Device.h"
#include "core/core.h"

namespace tim
{
	using namespace core;

	SoftVR_Device::SoftVR_Device(VRDebugCamera* pDebugCamera) : _pDebugCamera(pDebugCamera)
	{
		_hmdCamera._projection[0] = mat4::Projection(110, 1.0, 0.02, 300);
		_hmdCamera._projection[1] = mat4::Projection(110, 1.0, 0.02, 300);
	}
	void SoftVR_Device::update (float time)
	{
		_pDebugCamera->update(time);
		_hmdCamera._eyeView[0] = _pDebugCamera->eyeView(0);
		_hmdCamera._eyeView[1] = _pDebugCamera->eyeView(1);
		_hmdCamera._hmdMatrix = _pDebugCamera->viewMat();

		vec3 debugControllerPos = _pDebugCamera->pos() + _pDebugCamera->dir() * 0.5;
		mat4 l = mat4::RotationY(45);
		l.setTranslation(debugControllerPos);
		_controllerPos[0] = l;

		mat4 r = l;
		r.translate({ 0,0,0.3 });
		_controllerPos[1] = r;
	}

	void SoftVR_Device::submit(renderer::Texture* left, renderer::Texture* right)
	{

	}

	void SoftVR_Device::sync()
	{

	}

#if 0

#endif
}

