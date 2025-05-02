#ifndef OPENVR_DEVICE_H__
#define VOPENR_DEVICE_H__

#include "VR_DeviceInterface.h"

namespace tim {
	class SoftVR_Device : public VR_DeviceInterface
	{
	public:
		SoftVR_Device(vec2 zdist = { 0.1f, 1000.f }) {}
		virtual ~SoftVR_Device() override {}

		uivec2 hmdResolution() const { return uivec2(800,600); };
		bool isInit() const override  { return false;  }
        void update(float time) override;
		void submit(renderer::Texture* left, renderer::Texture* right) override;
        void sync() override;

        const core::mat4& controllerPose(int id) const override { return mat4::IDENTITY(); }
        const core::vec3& controllerVel(int id) const override { return vec3(0,0,0); }
        bool isControllerConnected(int id) const override { return true; }
        bool isHmdConnected() const override { return true; }

	private:

	};
}

#endif // OPENVR_DEVICE_H
