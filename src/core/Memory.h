// Memory.h (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"

#define NUM_BUFFER_BITS 16
#define MAX_NUM_BUFFERS ((size_t)1 << NUM_BUFFER_BITS)
#define NUM_ADDRESS_BITS ((sizeof(size_t)<<3) - NUM_BUFFER_BITS)
#define MAX_BUFFER_SIZE ((size_t)1 << NUM_ADDRESS_BITS)

#define EXTRACT_BUFFER(address) \
  (address >> NUM_ADDRESS_BITS)
#define EXTRACT_OFFSET(address) \
  (address & (((size_t)-1) >> NUM_BUFFER_BITS))

namespace oclgrind
{
  class Context;

  typedef struct
  {
    size_t address;
    cl_image_format format;
    cl_image_desc desc;
  } Image;

  class Memory
  {
  public:
    Memory(unsigned int addrSpace, const Context *context);
    virtual ~Memory();

    size_t allocateBuffer(size_t size);
    uint32_t atomicAdd(size_t address, uint32_t value);
    uint32_t atomicAnd(size_t address, uint32_t value);
    uint32_t atomicCmpxchg(size_t address, uint32_t cmp, uint32_t value);
    uint32_t atomicDec(size_t address);
    uint32_t atomicInc(size_t address);
    uint32_t atomicMax(size_t address, uint32_t value);
    uint32_t atomicMin(size_t address, uint32_t value);
    uint32_t atomicOr(size_t address, uint32_t value);
    uint32_t atomicSub(size_t address, uint32_t value);
    uint32_t atomicXchg(size_t address, uint32_t value);
    uint32_t atomicXor(size_t address, uint32_t value);
    void clear();
    Memory *clone() const;
    size_t createHostBuffer(size_t size, void *ptr);
    bool copy(size_t dest, size_t src, size_t size);
    void deallocateBuffer(size_t address);
    void dump() const;
    unsigned int getAddressSpace() const;
    void* getPointer(size_t address) const;
    size_t getTotalAllocated() const;
    bool isAddressValid(size_t address, size_t size=1) const;
    bool load(unsigned char *dest, size_t address, size_t size=1) const;
    unsigned char* mapBuffer(size_t address, size_t offset, size_t size);
    bool store(const unsigned char *source, size_t address, size_t size=1);

    static size_t getMaxAllocSize();

  private:
    typedef struct
    {
      bool hostPtr;
      size_t size;
      unsigned char *data;
    } Buffer;

    const Context *m_context;
    std::queue<int> m_freeBuffers;
    std::vector<Buffer> m_memory;
    unsigned int m_addressSpace;
    size_t m_totalAllocated;

    uint32_t* atomic(size_t address);
    int getNextBuffer();
  };
}