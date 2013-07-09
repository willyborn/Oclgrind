#include "common.h"
#include <istream>

#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS
#include "llvm/Support/InstIterator.h"

#include "Kernel.h"
#include "Memory.h"
#include "Device.h"
#include "WorkGroup.h"

using namespace spirsim;
using namespace std;

Device::Device()
{
  m_globalMemory = new Memory();
  m_outputMask = 0;

  // Check environment variables for output masks
  const char *env;

  env = getenv("OCLGRIND_OUTPUT_PRIVATE_MEM");
  if (env && strcmp(env, "1") == 0)
  {
    m_outputMask |= OUTPUT_PRIVATE_MEM;
  }
  env = getenv("OCLGRIND_OUTPUT_LOCAL_MEM");
  if (env && strcmp(env, "1") == 0)
  {
    m_outputMask |= OUTPUT_LOCAL_MEM;
  }
  env = getenv("OCLGRIND_OUTPUT_GLOBAL_MEM");
  if (env && strcmp(env, "1") == 0)
  {
    m_outputMask |= OUTPUT_GLOBAL_MEM;
  }
  env = getenv("OCLGRIND_OUTPUT_INSTRUCTIONS");
  if (env && strcmp(env, "1") == 0)
  {
    m_outputMask |= OUTPUT_INSTRUCTIONS;
  }
}

Device::~Device()
{
  delete m_globalMemory;
}

Memory* Device::getGlobalMemory() const
{
  return m_globalMemory;
}

void Device::run(Kernel& kernel, unsigned int workDim,
                 const size_t *globalOffset,
                 const size_t *globalSize,
                 const size_t *localSize)
{
  size_t offset[3] = {0,0,0};
  size_t ndrange[3] = {1,1,1};
  size_t wgsize[3] = {1,1,1};
  for (int i = 0; i < workDim; i++)
  {
    ndrange[i] = globalSize[i];
    if (globalOffset)
    {
      offset[i] = globalOffset[i];
    }
    if (localSize)
    {
      wgsize[i] = localSize[i];
    }
  }

  // Allocate and initialise constant memory
  kernel.allocateConstants(m_globalMemory);

  // Create work-groups
  size_t numGroups[3] = {ndrange[0]/wgsize[0],
                         ndrange[1]/wgsize[1],
                         ndrange[2]/wgsize[2]};
  size_t totalNumGroups = numGroups[0]*numGroups[1]*numGroups[2];
  for (int k = 0; k < numGroups[2]; k++)
  {
    for (int j = 0; j < numGroups[1]; j++)
    {
      for (int i = 0; i < numGroups[0]; i++)
      {
        if (m_outputMask &
            (OUTPUT_INSTRUCTIONS | OUTPUT_PRIVATE_MEM | OUTPUT_LOCAL_MEM))
        {
          cout << endl << BIG_SEPARATOR << endl;
          cout << "Work-group ("
               << i << ","
               << j << ","
               << k
               << ")" << endl;
          cout << BIG_SEPARATOR << endl;
        }

        WorkGroup *workGroup = new WorkGroup(kernel, *m_globalMemory,
                                             workDim, i, j, k,
                                             offset, ndrange, wgsize);
        workGroup->run(kernel, m_outputMask & OUTPUT_INSTRUCTIONS);

        // Dump contents of memories
        if (m_outputMask & OUTPUT_PRIVATE_MEM)
        {
          workGroup->dumpPrivateMemory();
        }
        if (m_outputMask & OUTPUT_LOCAL_MEM)
        {
          workGroup->dumpLocalMemory();
        }

        delete workGroup;
      }
    }
  }

  // Deallocate constant memory
  kernel.deallocateConstants(m_globalMemory);

  // Output global memory dump if required
  if (m_outputMask & OUTPUT_GLOBAL_MEM)
  {
    cout << endl << BIG_SEPARATOR << endl << "Global Memory:";
    m_globalMemory->dump();
    cout << BIG_SEPARATOR << endl;
  }
}

void Device::setOutputMask(unsigned char mask)
{
  m_outputMask = mask;
}
