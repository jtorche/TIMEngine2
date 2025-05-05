#ifndef MESHINSTANCE_H
#define MESHINSTANCE_H

#include "Matrix.h"
#include "Mesh.h"
#include "SimpleScene.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace interface
{
    class MeshInstance : public scene::Transformable
    {
        friend class scene::BasicScene<scene::Transformable>;

    public:
        void setMesh(const Mesh&);
        const Mesh& mesh() const { return _mesh; }

        const mat4& matrix() const { return _model; }
        void setMatrix(const mat4&);

        void attachUBO(uint id, uint index);
        const vector<uint>& attachedUBO() const;
        void clearAttachedUBO();

        void setUseShadowLOD(bool use) { _useShadowLOD = use; }
        void setUseVisualLOD(bool use) { _useVisualLOD = use; }
        bool useShadowLOD() const { return _useShadowLOD; }
        bool useVisualLOD() const { return _useVisualLOD; }
          
    protected:
        mat4 _model;
        Mesh _mesh;
        vector<uint> _extraUbo;
        bool _useShadowLOD = false, _useVisualLOD = false;

        MeshInstance() = default;
        ~MeshInstance() = default;

        MeshInstance(const mat4& m) : Transformable() { setMatrix(m); }
        MeshInstance(const Mesh& mesh, const mat4& m) : Transformable(), _mesh(mesh) { setMatrix(m); }
    };
}
}
#include "MemoryLoggerOff.h"

#endif // MESHINSTANCE_H
