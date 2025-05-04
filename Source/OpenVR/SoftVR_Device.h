#ifndef OPENVR_DEVICE_H__
#define VOPENR_DEVICE_H__

#include "VR_DeviceInterface.h"
#include "VRDebugCamera.h"

namespace tim {
	class SoftVR_Device : public VR_DeviceInterface
	{
	public:
		SoftVR_Device(VRDebugCamera* pDebugCamera);
		virtual ~SoftVR_Device() override {}

		uivec2 hmdResolution() const { return uivec2(1852,2056); };
		bool isInit() const override  { return true;  }
        void update(float time) override;
		void submit(renderer::Texture* left, renderer::Texture* right) override;
        void sync() override;

        const core::mat4& controllerPose(int id) const override { return _controllerPos[id]; }
        const core::vec3& controllerVel(int id) const override { return nullVel; }
        bool isControllerConnected(int id) const override { return true; }
        bool isHmdConnected() const override { return true; }

	private:
		VRDebugCamera* _pDebugCamera;
		mat4 _controllerPos[2];
		vec3 nullVel = vec3(0, 0, 0.1);
	};
}

#endif // OPENVR_DEVICE_H
