#ifndef VR_DEVICE_H__
#define VR_DEVICE_H__

#include "core/Matrix.h"
#include "renderer/FrameBuffer.h"

#include "core/MemoryLoggerOn.h"
namespace tim {
	class VR_DeviceInterface
	{
	public:
		enum { LEFT = 0, RIGHT = 1 };

		class HmdCamera
		{
		public:
			const mat4& hmdView() const { return _hmdMatrix;  }

			const mat4& eyeProjection(int eye) const { return _projection[eye]; }
			const mat4& eyeView(int eye) const { return _eyeView[eye]; }

		protected:
			friend class VR_DeviceInterface;
			friend class OpenVR_Device;
			friend class SoftVR_Device;
			HmdCamera() = default;

            mat4 _hmdMatrix = mat4::IDENTITY();
			vec3 _vel, _angVel;

            mat4 _projection[2] = {mat4::IDENTITY(), mat4::IDENTITY()};
            mat4 _hmdToEye[2] = {mat4::IDENTITY(), mat4::IDENTITY()};
            mat4 _eyeView[2] = {mat4::IDENTITY(), mat4::IDENTITY()};
		};

		virtual ~VR_DeviceInterface() {}

		virtual uivec2 hmdResolution() const = 0;
		virtual bool isInit() const = 0;
        virtual void update(float time) = 0;
		virtual void submit(renderer::Texture* left, renderer::Texture* right) = 0;
        virtual void sync() = 0;

		HmdCamera& camera() { return _hmdCamera;  }
        const HmdCamera& camera() const { return _hmdCamera;  }

		virtual const core::mat4& controllerPose(int id) const = 0;
		virtual const core::vec3& controllerVel(int id) const = 0;
		virtual bool isControllerConnected(int id) const = 0;
		virtual bool isHmdConnected() const = 0;

	protected:
		HmdCamera _hmdCamera;
	};
}
#include "core/MemoryLoggerOff.h"

#endif // VR_DEVICE_H
