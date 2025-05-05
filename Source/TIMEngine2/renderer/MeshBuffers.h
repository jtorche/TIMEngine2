#ifndef MESHBUFFERS_H_INCLUDED
#define MESHBUFFERS_H_INCLUDED

#include "renderer.h"
#include "VAO.h"
#include "Sphere.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace renderer
{
    struct MeshData
    {
        using DataType = renderer::VNCT_Vertex;
        static const int DATA_ID = renderer::VNCT;
        std::string name;

        uint nbIndex = 0;
        uint nbSecondaryIndex = 0;
        uint* indexData = nullptr;
        uint* secondaryIndexData = nullptr;

        renderer::VertexFormat format = renderer::VertexFormat::VNCT;
        uint nbVertex = 0;
        DataType* vData = nullptr;

        void clear()
        {
            nbIndex=0;
            nbSecondaryIndex=0;
            nbVertex=0;
            delete[] indexData;
            delete[] secondaryIndexData;
            delete[] vData;
            indexData=nullptr;
            secondaryIndexData = nullptr;
            vData=nullptr;
        }
    };

    class MeshBuffers : NonCopyable
    {
    public:
        MeshBuffers(VBuffer* vb, IBuffer* ib, IBuffer* ib2 = nullptr, const Sphere& s = Sphere(), MeshData* cpuData=nullptr)
            : _vb(vb), _ib(ib), _ib2(ib2), _volume(s), _cpuData(cpuData) {}

        ~MeshBuffers()
        {
            freeCpuData();
            delete _vb;
            delete _ib;
            delete _ib2;
        }

        void draw(size_t s, VertexMode primitive, size_t nbInstance, const VAO* vao = nullptr) const
        {
            if(!_vb || !_ib) return;

            if(vao)
                vao->bind();

            _ib->bind(); // bind Element
            s = std::min(s, _ib->size());

            if(nbInstance>0)
                glDrawElementsInstancedBaseVertex(IndexBuffer::GLPrimitive[primitive], s, GL_UNSIGNED_INT, BUFFER_OFFSET(_ib->offset()*sizeof(IBuffer::Type)), nbInstance, _vb->offset());
            else
                glDrawElementsBaseVertex(IndexBuffer::GLPrimitive[primitive], s, GL_UNSIGNED_INT, BUFFER_OFFSET(_ib->offset()*sizeof(IBuffer::Type)), _vb->offset());
        }

        VBuffer* vb() const { return _vb; }
        IBuffer* ib(bool useSecondary = false) const { return !_ib2 ? _ib : (useSecondary ? _ib2 : _ib); }

        bool hasSecondaryIndexBuffer() const { return _ib2 != nullptr; }
        bool isNull() const { return (!_vb || !_ib); }

        const Sphere& volume() const { return _volume; }
        void setVolume(const Sphere& s) { _volume = s; }

        void swap(MeshBuffers& buf)
        {
            std::swap(_vb, buf._vb);
            std::swap(_ib, buf._ib);
            std::swap(_ib2, buf._ib2);
            std::swap(_volume, buf._volume);
            std::swap(_cpuData, buf._cpuData);
        }

        MeshData* cpuData() const { return _cpuData; }
        void freeCpuData()
        {
            if(_cpuData)
            {
                _cpuData->clear();
                delete _cpuData;
                _cpuData = nullptr;
            }
        }

    private:
        VBuffer* _vb;
        IBuffer* _ib;
        IBuffer* _ib2;

        Sphere _volume;
        MeshData* _cpuData = nullptr; // optional, use to keep track of the initial data, can be freed at any moment
    };
}
}
#include "MemoryLoggerOff.h"

#endif // MESHBUFFERS_H_INCLUDED
