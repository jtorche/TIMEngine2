#ifndef HMD_SCENEVIEW_H
#define HMD_SCENEVIEW_H

#include "interface/pipeline/pipeline.h"
#include "VR_DeviceInterface.h"
#include "VRDebugCamera.h"

namespace tim {
    struct HmdSceneView
    {
    public:
        HmdSceneView(float fov, float ratio, float far=500);

        void setScaleAndShiftRoom(float scale, float zShift) { _scaleRoom = scale; _zShift = zShift; }

        interface::View& cullingView() { return _cullingView; }
        const interface::View& cullingView() const { return _cullingView; }

        interface::View& eyeView(int eye) { return _eyeView[eye]; }
        const interface::View& eyeView(int eye) const { return _eyeView[eye]; }

        void addOffset(const mat4& o);
        const mat4& offset() const { return _offset; }
        mat4 applyTransformOnHmdMatrix(const mat4&) const;
        mat4 applyTransformOnControllerMatrix(const mat4&) const;

        void update(const VR_DeviceInterface& hmdDevice);
        void update(const VRDebugCamera& cam, float ratio);

        void update(const Camera&)
        {
            TIM_ASSERT(false);
        }

        const mat4& transform() const { return _transform; }

    private:
        interface::View _cullingView;
        interface::View _eyeView[2];
        float _renderFov = 110;
        float _scaleRoom = 1;
        float _zShift = 0;

        mat4 _offset = mat4::IDENTITY();
        mat4 _offsetScale = mat4::IDENTITY();
        mat4 _transform = mat4::IDENTITY();
    };
}

#endif // HMD_SCENEVIEW_H
