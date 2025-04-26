#ifndef RANGEALLOCATOR
#define RANGEALLOCATOR

#include <type.h>
#include "Timath.h"
#include <mutex>

#include "Exception.h"

#include "MemoryLoggerOn.h"
namespace tim
{
namespace core
{
    template <uint BLOCK_BIT_SIZE = 10, class MutexType = std::mutex>
    class BuddyBlocksAllocator
    {
    public:
        using addr = unsigned long int;

        BuddyBlocksAllocator(size_t maxSize) : _maxSize(le_power2(maxSize))
        {
            _remainingMemory = _maxSize;
            _root = new Node; // use object pool
            _root->startAddr = 0;
            _root->parent = nullptr;
            _root->magnitude = magnitudeFromSize(_maxSize);
        }

        ~BuddyBlocksAllocator() = default;

        addr alloc(size_t size)
        {
            std::lock_guard<MutexType> guard(_mutex);

            size_t upperSize = std::max(ge_power2(size), 1u<<BLOCK_BIT_SIZE);
            uint m = magnitudeFromSize(upperSize);

            while(1)
            {
                Node* curSmallest = nullptr;
                Node* n = searchFreeNode(_root, m, &curSmallest);
                TIM_ASSERT(curSmallest);
                if(n)
                {
                    n->state = ALLOCATED;
                    n->allocatedSize = size;
                    _remainingMemory -= upperSize;
                    _allocatedMemory += size;
                    _allocatedAddr[n->startAddr] = n;

                    return n->startAddr;
                }
                else
                {
                    TIM_ASSERT(curSmallest->magnitude != 0 && curSmallest->state == FREE);
                    for(int i=0 ; i<2 ; ++i)
                    {
                        curSmallest->child[i] = new Node; // _pool.construct();
                        curSmallest->child[i]->state = FREE;
                        curSmallest->child[i]->parent = curSmallest;
                        curSmallest->child[i]->magnitude = curSmallest->magnitude - 1;
                        curSmallest->child[i]->startAddr = curSmallest->startAddr + i*(1 << (curSmallest->magnitude+BLOCK_BIT_SIZE-1));
                    }
                    curSmallest->state = SPLIT;
                }
            }
        }

        void dealloc(addr ptr)
        {
            std::lock_guard<MutexType> guard(_mutex);

            auto it = _allocatedAddr.find(ptr);
            if(it == _allocatedAddr.end())
                return;

            it->second->state = FREE;
            _remainingMemory += 1 << (it->second->magnitude+BLOCK_BIT_SIZE);
            _allocatedMemory -= it->second->allocatedSize;

            Node* n = it->second->parent;
            while(n)
            {
                if(n->child[0]->state == FREE && n->child[1]->state == FREE)
                {
                    n->state = FREE;
                    for(int i=0 ; i<2 ; i++)
                    {
                        delete n->child[i];
                        // _pool.destroy(n->child[i]);
                        n->child[i] = nullptr;
                    }
                }
                n = n->parent;
            }

            _allocatedAddr.erase(it);
        }

        uint maxSize() const { return _maxSize; }
        uint remainingMemory() const { return _remainingMemory; }
        uint allocatedMemory() const { return _allocatedMemory; }

    private:
        enum NodeState : int { FREE, ALLOCATED, SPLIT };
        struct Node
        {
            addr startAddr;
            uint magnitude;
            Node* parent;
            uint allocatedSize = 0;
            Node* child[2] = {nullptr};
            NodeState state = FREE;
        };

        uint _maxSize;
        Node* _root;
        MutexType _mutex;

        std::map<addr, Node*> _allocatedAddr;

        // debug info
        size_t _remainingMemory, _allocatedMemory=0;

        uint magnitudeFromSize(size_t s) const { return log2_ui<uint>(s >> BLOCK_BIT_SIZE); }

        Node* searchFreeNode(Node* node, uint magnitude, Node ** smallest)
        {
            if(node == nullptr) return nullptr;

            switch(node->state)
            {
            case FREE:
                if(node->magnitude < magnitude) return nullptr;
                else if(node->magnitude == magnitude) return node;
                else {
                    if((*smallest) == nullptr || (*smallest)->magnitude > node->magnitude)
                        *smallest = node;
                    return nullptr;
                }

            case ALLOCATED:
                return nullptr;

            case SPLIT:
                for(int i=0 ; i<2 ; ++i)
                {
                    Node* n = searchFreeNode(node->child[i], magnitude, smallest);
                    if(n) return n;
                }
                return nullptr;
            }
        }
    };

    template <uint BLOCK_SIZE = 1024, class MutexType = std::mutex>
    class FixedSizeBlocksAllocator
    {
    public:
        using addr = unsigned long int;

        FixedSizeBlocksAllocator(uint maxSize) : _maxSize(maxSize - maxSize%BLOCK_SIZE)
        {
            _remainingMemory = _maxSize;
            _freeChunks.insert({0, _maxSize / BLOCK_SIZE});
        }

        ~FixedSizeBlocksAllocator() = default;

        addr alloc(uint size)
        {
            std::lock_guard<MutexType> guard(_mutex);
            uint nbBlocks = 1 + (size-1) / BLOCK_SIZE;

            for(auto it=_freeChunks.begin() ; it != _freeChunks.end() ; ++it)
            {
                EmptyChunck chunk = *it;
                if(chunk.nbBlocks >= nbBlocks)
                {
                    _freeChunks.erase(it);
                    if(chunk.nbBlocks > nbBlocks)
                        _freeChunks.insert({chunk.firstBlock+nbBlocks, chunk.nbBlocks-nbBlocks});

                    _remainingMemory -= nbBlocks*BLOCK_SIZE;
                    _allocatedMemory += size;

                    uint minBlocksChunkAddr = chunk.firstBlock*BLOCK_SIZE;

                    _allocatedAddr[minBlocksChunkAddr] = {nbBlocks, size};
                    return minBlocksChunkAddr;
                }
            }
            TIM_ASSERT(false);
            return 0;
        }

        void dealloc(addr ptr)
        {
            std::lock_guard<MutexType> guard(_mutex);

            auto it_allocated = _allocatedAddr.find(ptr);
            if(it_allocated == _allocatedAddr.end())
                return;

            uint firstBlock = ptr / BLOCK_SIZE;
            uint nbBlocks = it_allocated->second.nbBlocks;

            EmptyChunck chunk = {firstBlock, nbBlocks};

            for(auto it=_freeChunks.begin() ; it != _freeChunks.end() ;)
            {
                auto cur_it = it++;
                if(cur_it->firstBlock+cur_it->nbBlocks == firstBlock)
                {
                    chunk.firstBlock = cur_it->firstBlock;
                    chunk.nbBlocks = chunk.nbBlocks + cur_it->nbBlocks;
                    _freeChunks.erase(cur_it);
                }
                else if(firstBlock+nbBlocks == cur_it->firstBlock)
                {
                    chunk.nbBlocks = chunk.nbBlocks + cur_it->nbBlocks;
                    _freeChunks.erase(cur_it);
                }
            }

            _freeChunks.insert(chunk);

            _remainingMemory += nbBlocks*BLOCK_SIZE;
            _allocatedMemory -= it_allocated->second.nbMem;
            _allocatedAddr.erase(it_allocated);
        }

        uint maxAllocation() const { std::lock_guard<MutexType> guard(_mutex); return _freeChunks.rbegin()->nbBlocks * BLOCK_SIZE; }

        uint maxSize() const { return _maxSize; }
        uint remainingMemory() const { return _remainingMemory; }
        uint allocatedMemory() const { return _allocatedMemory; }

    private:
        uint _maxSize;
        mutable MutexType _mutex;

        struct EmptyChunck { uint firstBlock; uint nbBlocks; };
        struct CompareEmptyChunk
        { bool operator()(const EmptyChunck& c1, const EmptyChunck& c2) const  { return c1.nbBlocks==c2.nbBlocks ?
                          c1.firstBlock < c2.firstBlock : c1.nbBlocks < c2.nbBlocks; } };

        std::set<EmptyChunck, CompareEmptyChunk> _freeChunks;

        struct Allocation { uint nbBlocks; uint nbMem; };
        std::map<addr, Allocation> _allocatedAddr;

        // debug info
        uint _remainingMemory, _allocatedMemory=0;
    };
}
}
#include "MemoryLoggerOff.h"

#endif // RANGEALLOCATOR

