#ifndef MESHBUFFERPOOL_H
#define MESHBUFFERPOOL_H

#include "core/RangeAllocator.h"
#include "IndexBuffer.h"
#include "GenericVertexBuffer.h"

#include "MemoryLoggerOn.h"
namespace tim
{
    using namespace core;
namespace renderer
{
    template<class BufferType, uint SIZE_BLOCK>
    class BufferPool
    {
    public:

        using InternBufferType = typename BufferType::Type;
        using AllocatorType = FixedSizeBlocksAllocator<SIZE_BLOCK>;

        class Instance : public NonCopyable
        {
            friend class BufferPool;

        public:
            using Type = InternBufferType;

            void bind() const
            {
                _pool._buffer.bind();
            }

            void flush(const InternBufferType* data, size_t begin, size_t size) const
            {
                size = std::min(size, _capacity);
                _pool._buffer.flush(data, begin+_begin, size);
            }

            ~Instance()
            {
                _pool._bufferAllocator.dealloc(_begin);
            }

            void setSize(size_t s) { _size = s; }

            size_t offset() const { return _begin; }
            size_t size() const { return _size; }
            size_t capacity() const { return _capacity; }
            size_t elementSize() const { return _elementSize; }

        private:
            Instance(size_t begin, size_t capacity, size_t elementSize, BufferPool& mbp)
                : _begin(begin), _size(capacity), _capacity(capacity), _elementSize(elementSize), _pool(mbp) {}

            size_t _begin, _size, _capacity, _elementSize;
            BufferPool& _pool;
        };

        template<class... Args>
        BufferPool(size_t, Args... args);
        ~BufferPool() = default;

        Instance* alloc(size_t);

        const AllocatorType& allocator() const { return _bufferAllocator; }

        const BufferType& buffer() const { return _buffer; }

    private:
        AllocatorType _bufferAllocator;
        BufferType _buffer;
    };

    template<class BufferType, uint SIZE_BLOCK>
    template<class... Args>
    BufferPool<BufferType, SIZE_BLOCK>::BufferPool(size_t size, Args... args) : _bufferAllocator(size), _buffer()
    {
        _buffer.create(size, nullptr, args...);
    }

    template<class BufferType, uint SIZE_BLOCK>
    typename BufferPool<BufferType, SIZE_BLOCK>::Instance* BufferPool<BufferType, SIZE_BLOCK>::alloc(size_t size)
    {
        typename AllocatorType::addr addr = _bufferAllocator.alloc(size);
        return new typename BufferPool<BufferType, SIZE_BLOCK>::Instance(addr, size, _buffer.elementSize(), *this);
    }

}
}
#include "MemoryLoggerOff.h"

#endif // MESHBUFFERPOOL_H
