#ifndef OPENVR_DEVICE_H__
#define VOPENR_DEVICE_H__

#include "VR_DeviceInterface.h"
#include <openvr.h>

#include "core/MemoryLoggerOn.h"
namespace tim {
	class OpenVR_Device : public VR_DeviceInterface
	{
	public:
		OpenVR_Device(bool useWaitGetPoseOnUpdate, vec2 zdist = { 0.1f, 1000.f });
		virtual ~OpenVR_Device() override;

		uivec2 hmdResolution() const override;
		bool isInit() const override  { return _hmd != nullptr;  }
        void update(float time) override;
		void submit(renderer::Texture* left, renderer::Texture* right) override;
        void sync() override;

        const core::mat4& controllerPose(int id) const override  { return _devicePose[_controller[id]]; }
        const core::vec3& controllerVel(int id) const override  { return _deviceVel[_controller[id]]; }
        bool isControllerConnected(int id) const override { return _isControllerConnected[id]; }
        bool isHmdConnected() const override { return _hmdConnected; }

	private:
		vr::IVRSystem* _hmd = nullptr;
		vr::IVRCompositor* _compositor = nullptr;

		core::mat4 _devicePose[vr::k_unMaxTrackedDeviceCount];
        core::vec3 _deviceVel[vr::k_unMaxTrackedDeviceCount];
		vr::TrackedDevicePose_t _vrTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];

        uint _controller[2];
        bool _isControllerConnected[2] = {false, false};
        bool _hmdConnected = false;
		bool _useWaitGetPoseOnUpdate;
	};
}
#include "core/MemoryLoggerOff.h"

#endif // OPENVR_DEVICE_H
