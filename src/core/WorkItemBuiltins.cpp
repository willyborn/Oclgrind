// WorkItemBuiltins.cpp (Oclgrind)
// Copyright (c) 2013-2014, James Price and Simon McIntosh-Smith,
// University of Bristol. All rights reserved.
//
// This program is provided under a three-clause BSD license. For full
// license terms please see the LICENSE file distributed with this
// source code.

#include "common.h"
#include <algorithm>
#include <fenv.h>

#pragma STDC FENV_ACCESS ON

#include "llvm/Instructions.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/Metadata.h"

#include "CL/cl.h"
#include "Context.h"
#include "Device.h"
#include "half.h"
#include "Memory.h"
#include "WorkGroup.h"
#include "WorkItem.h"

using namespace oclgrind;
using namespace std;

#define CLK_NORMALIZED_COORDS_TRUE 0x0001

#define CLK_ADDRESS_NONE 0x0000
#define CLK_ADDRESS_CLAMP_TO_EDGE 0x0002
#define CLK_ADDRESS_CLAMP 0x0004
#define CLK_ADDRESS_REPEAT 0x0006
#define CLK_ADDRESS_MIRRORED_REPEAT 0x0008
#define CLK_ADDRESS_MASK 0x000E

#define CLK_FILTER_NEAREST 0x0010
#define CLK_FILTER_LINEAR 0x0020

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

namespace oclgrind
{
  class WorkItemBuiltins
  {
    // Utility macros for creating builtins
#define DEFINE_BUILTIN(name)                                           \
  static void name(WorkItem *workItem, const llvm::CallInst *callInst, \
                   const string& fnName, const string& overload,       \
                   TypedValue& result, void *)
#define ARG(i) (callInst->getArgOperand(i))
#define UARGV(i,v) workItem->getOperand(ARG(i)).getUInt(v)
#define SARGV(i,v) workItem->getOperand(ARG(i)).getSInt(v)
#define FARGV(i,v) workItem->getOperand(ARG(i)).getFloat(v)
#define PARGV(i,v) workItem->getOperand(ARG(i)).getPointer(v)
#define UARG(i) UARGV(i, 0)
#define SARG(i) SARGV(i, 0)
#define FARG(i) FARGV(i, 0)
#define PARG(i) PARGV(i, 0)

    // Functions that apply generic builtins to each component of a vector
    static void f1arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, double (*func)(double))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setFloat(func(FARGV(0, i)), i);
      }
    }
    static void f2arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, double (*func)(double, double))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setFloat(func(FARGV(0, i), FARGV(1, i)), i);
      }
    }
    static void f3arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, double (*func)(double, double, double))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setFloat(func(FARGV(0, i), FARGV(1, i), FARGV(2, i)), i);
      }
    }
    static void u1arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, uint64_t (*func)(uint64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setUInt(func(UARGV(0, i)), i);
      }
    }
    static void u2arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, uint64_t (*func)(uint64_t, uint64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setUInt(func(UARGV(0, i), UARGV(1, i)), i);
      }
    }
    static void u3arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, uint64_t (*func)(uint64_t, uint64_t, uint64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setUInt(func(UARGV(0, i), UARGV(1, i), UARGV(2, i)), i);
      }
    }
    static void s1arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, int64_t (*func)(int64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setSInt(func(SARGV(0, i)), i);
      }
    }
    static void s2arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, int64_t (*func)(int64_t, int64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setSInt(func(SARGV(0, i), SARGV(1, i)), i);
      }
    }
    static void s3arg(WorkItem *workItem, const llvm::CallInst *callInst,
                      const string& name, const string& overload,
                      TypedValue& result, int64_t (*func)(int64_t, int64_t, int64_t))
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setSInt(func(SARGV(0, i), SARGV(1, i), SARGV(2, i)), i);
      }
    }
    static void rel1arg(WorkItem *workItem, const llvm::CallInst *callInst,
                        const string& name, const string& overload,
                        TypedValue& result, int64_t (*func)(double))
    {
      int64_t t = result.num > 1 ? -1 : 1;
      for (int i = 0; i < result.num; i++)
      {
        result.setSInt(func(FARGV(0, i))*t, i);
      }
    }
    static void rel2arg(WorkItem *workItem, const llvm::CallInst *callInst,
                        const string& name, const string& overload,
                        TypedValue& result, int64_t (*func)(double, double))
    {
      int64_t t = result.num > 1 ? -1 : 1;
      for (int i = 0; i < result.num; i++)
      {
        result.setSInt(func(FARGV(0, i), FARGV(1, i))*t, i);
      }
    }

    // Extract the (first) argument type from an overload string
    static char getOverloadArgType(const string& overload)
    {
      char type = overload[0];
      if (type == 'D')
      {
        char *typestr;
        strtol(overload.c_str() + 2, &typestr, 10);
        type = typestr[1];
      }
      return type;
    }


    ///////////////////////////////////////
    // Async Copy and Prefetch Functions //
    ///////////////////////////////////////

    DEFINE_BUILTIN(async_work_group_copy)
    {
      int arg = 0;

      // Get src/dest addresses
      const llvm::Value *destOp = ARG(arg++);
      const llvm::Value *srcOp = ARG(arg++);
      size_t dest = workItem->getOperand(destOp).getPointer();
      size_t src = workItem->getOperand(srcOp).getPointer();

      // Get size of copy
      size_t elemSize = getTypeSize(destOp->getType()->getPointerElementType());
      uint64_t num = UARG(arg++);

      // Get stride
      uint64_t stride = 1;
      size_t srcStride = 1;
      size_t destStride = 1;
      if (fnName == "async_work_group_strided_copy")
      {
        stride = UARG(arg++);
      }

      size_t event = UARG(arg++);

      // Get type of copy
      WorkGroup::AsyncCopyType type;
      if (destOp->getType()->getPointerAddressSpace() == AddrSpaceLocal)
      {
        type = WorkGroup::GLOBAL_TO_LOCAL;
        srcStride = stride;
      }
      else
      {
        type = WorkGroup::LOCAL_TO_GLOBAL;
        destStride = stride;
      }

      // Register copy
      event = workItem->m_workGroup->async_copy(
        workItem,
        callInst,
        type,
        dest,
        src,
        elemSize,
        num,
        srcStride,
        destStride,
        event);
      result.setUInt(event);
    }

    DEFINE_BUILTIN(wait_group_events)
    {
      uint64_t num = UARG(0);
      size_t address = PARG(1);
      list<size_t> events;
      for (int i = 0; i < num; i++)
      {
        size_t event;
        if (!workItem->m_privateMemory->load((unsigned char*)&event,
            address, sizeof(size_t)))
        {
          return;
        }
        events.push_back(event);
        address += sizeof(size_t);
      }
      workItem->m_state = WorkItem::BARRIER;
      workItem->m_workGroup->notifyBarrier(workItem, callInst,
                                           CLK_LOCAL_MEM_FENCE, events);
    }

    DEFINE_BUILTIN(prefetch)
    {
      // Do nothing.
    }


    //////////////////////
    // Atomic Functions //
    //////////////////////

    DEFINE_BUILTIN(atomic_add)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_add");
      }
      uint32_t old = memory->atomicAdd(address, UARG(1));
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_and)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_and");
      }
      uint32_t old = memory->atomicAnd(address, UARG(1));
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_cmpxchg)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_cmpxchg");
      }
      uint32_t old = memory->atomicCmpxchg(address, UARG(1), UARG(2));
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_dec)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_dec");
      }
      uint32_t old = memory->atomicDec(address);
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_inc)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_dec");
      }
      uint32_t old = memory->atomicInc(address);
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_max)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_max");
      }
      uint32_t old = memory->atomicMax(address, UARG(1));
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_min)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_min");
      }
      uint32_t old = memory->atomicMin(address, UARG(1));
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_or)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_or");
      }
      uint32_t old = memory->atomicOr(address, UARG(1));
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_sub)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_sub");
      }
      uint32_t old = memory->atomicSub(address, UARG(1));
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_xchg)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_xchg");
      }
      uint32_t old = memory->atomicXchg(address, UARG(1));
      result.setUInt(old);
    }

    DEFINE_BUILTIN(atomic_xor)
    {
      Memory *memory =
        workItem->getMemory(ARG(0)->getType()->getPointerAddressSpace());

      size_t address = PARG(0);
      // Verify the address is 4-byte aligned
      if ((address & 0x3) != 0) {
        workItem->m_context->logError("Unaligned address on atomic_xor");
      }
      uint32_t old = memory->atomicXor(address, UARG(1));
      result.setUInt(old);
    }


    //////////////////////
    // Common Functions //
    //////////////////////

    template<typename T> T static _max_(T a, T b){return a > b ? a : b;}
    template<typename T> T static _min_(T a, T b){return a < b ? a : b;}
    template<typename T> T static _clamp_(T x, T min, T max)
    {
      return _min_(_max_(x, min), max);
    }

    static double _degrees_(double x)
    {
      return x * (180 / M_PI);
    }

    static double _radians_(double x)
    {
      return x * (M_PI / 180);
    }

    static double _sign_(double x)
    {
      if (::isnan(x))  return  0.0;
      if (x  >  0.0) return  1.0;
      if (x == -0.0) return -0.0;
      if (x ==  0.0) return  0.0;
      if (x  <  0.0) return -1.0;
      return 0.0;
    }

    DEFINE_BUILTIN(clamp)
    {
      switch (getOverloadArgType(overload))
      {
        case 'f':
        case 'd':
          if (ARG(1)->getType()->isVectorTy())
          {
            f3arg(workItem, callInst, fnName, overload, result, _clamp_);
          }
          else
          {
            for (int i = 0; i < result.num; i++)
            {
              double x = FARGV(0, i);
              double minval = FARG(1);
              double maxval = FARG(2);
              result.setFloat(_clamp_(x, minval, maxval), i);
            }
          }
          break;
        case 'h':
        case 't':
        case 'j':
        case 'm':
          u3arg(workItem, callInst, fnName, overload, result, _clamp_);
          break;
        case 'c':
        case 's':
        case 'i':
        case 'l':
          s3arg(workItem, callInst, fnName, overload, result, _clamp_);
          break;
        default:
          FATAL_ERROR("Unsupported argument type: %c",
                      getOverloadArgType(overload));
      }
    }

    DEFINE_BUILTIN(max)
    {
      switch (getOverloadArgType(overload))
      {
        case 'f':
        case 'd':
          if (ARG(1)->getType()->isVectorTy())
          {
            f2arg(workItem, callInst, fnName, overload, result, fmax);
          }
          else
          {
            for (int i = 0; i < result.num; i++)
            {
              double x = FARGV(0, i);
              double y = FARG(1);
              result.setFloat(_max_(x, y), i);
            }
          }
          break;
        case 'h':
        case 't':
        case 'j':
        case 'm':
          u2arg(workItem, callInst, fnName, overload, result, _max_);
          break;
        case 'c':
        case 's':
        case 'i':
        case 'l':
          s2arg(workItem, callInst, fnName, overload, result, _max_);
          break;
        default:
          FATAL_ERROR("Unsupported argument type: %c",
                      getOverloadArgType(overload));
      }
    }

    DEFINE_BUILTIN(min)
    {
      switch (getOverloadArgType(overload))
      {
        case 'f':
        case 'd':
          if (ARG(1)->getType()->isVectorTy())
          {
            f2arg(workItem, callInst, fnName, overload, result, fmin);
          }
          else
          {
            for (int i = 0; i < result.num; i++)
            {
              double x = FARGV(0, i);
              double y = FARG(1);
              result.setFloat(_min_(x, y), i);
            }
          }
          break;
        case 'h':
        case 't':
        case 'j':
        case 'm':
          u2arg(workItem, callInst, fnName, overload, result, _min_);
          break;
        case 'c':
        case 's':
        case 'i':
        case 'l':
          s2arg(workItem, callInst, fnName, overload, result, _min_);
          break;
        default:
          FATAL_ERROR("Unsupported argument type: %c",
                      getOverloadArgType(overload));
      }
    }

    DEFINE_BUILTIN(mix)
    {
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        double y = FARGV(1, i);
        double a = ARG(2)->getType()->isVectorTy() ? FARGV(2, i) : FARG(2);
        double r = x + (y - x) * a;
        result.setFloat(r, i);
      }
    }

    DEFINE_BUILTIN(smoothstep)
    {
      for (int i = 0; i < result.num; i++)
      {
        double edge0 = ARG(0)->getType()->isVectorTy() ? FARGV(0, i) : FARG(0);
        double edge1 = ARG(1)->getType()->isVectorTy() ? FARGV(1, i) : FARG(1);
        double x = FARGV(2, i);
        double t = _clamp_<double>((x - edge0) / (edge1 - edge0), 0, 1);
        double r = t * t * (3 - 2*t);
        result.setFloat(r, i);
      }
    }

    DEFINE_BUILTIN(step)
    {
      for (int i = 0; i < result.num; i++)
      {
        double edge = ARG(0)->getType()->isVectorTy() ? FARGV(0, i) : FARG(0);
        double x = FARGV(1, i);
        double r = (x < edge) ? 0.0 : 1.0;
        result.setFloat(r, i);
      }
    }


    /////////////////////////
    // Geometric Functions //
    /////////////////////////

    DEFINE_BUILTIN(cross)
    {
      double u1 = FARGV(0, 0);
      double u2 = FARGV(0, 1);
      double u3 = FARGV(0, 2);
      double v1 = FARGV(1, 0);
      double v2 = FARGV(1, 1);
      double v3 = FARGV(1, 2);
      result.setFloat(u2*v3 - u3*v2, 0);
      result.setFloat(u3*v1 - u1*v3, 1);
      result.setFloat(u1*v2 - u2*v1, 2);
      result.setFloat(0, 3);
    }

    DEFINE_BUILTIN(dot)
    {
      int num = 1;
      if (ARG(0)->getType()->isVectorTy())
      {
        num = ARG(0)->getType()->getVectorNumElements();
      }

      double r = 0.f;
      for (int i = 0; i < num; i++)
      {
        double a = FARGV(0, i);
        double b = FARGV(1, i);
        r += a * b;
      }
      result.setFloat(r);
    }

    DEFINE_BUILTIN(distance)
    {
      int num = 1;
      if (ARG(0)->getType()->isVectorTy())
      {
        num = ARG(0)->getType()->getVectorNumElements();
      }

      double distSq = 0.0;
      for (int i = 0; i < num; i++)
      {
        double diff = FARGV(0,i) - FARGV(1,i);
        distSq += diff*diff;
      }
      result.setFloat(sqrt(distSq));
    }

    DEFINE_BUILTIN(length)
    {
      int num = 1;
      if (ARG(0)->getType()->isVectorTy())
      {
        num = ARG(0)->getType()->getVectorNumElements();
      }

      double lengthSq = 0.0;
      for (int i = 0; i < num; i++)
      {
        lengthSq += FARGV(0, i) * FARGV(0, i);
      }
      result.setFloat(sqrt(lengthSq));
    }

    DEFINE_BUILTIN(normalize)
    {
      double lengthSq = 0.0;
      for (int i = 0; i < result.num; i++)
      {
        lengthSq += FARGV(0, i) * FARGV(0, i);
      }
      double length = sqrt(lengthSq);

      for (int i = 0; i < result.num; i++)
      {
        result.setFloat(FARGV(0, i)/length, i);
      }
    }


    /////////////////////
    // Image Functions //
    /////////////////////

    static size_t getChannelSize(const cl_image_format& format)
    {
      switch (format.image_channel_data_type)
      {
      case CL_SNORM_INT8:
      case CL_UNORM_INT8:
      case CL_SIGNED_INT8:
      case CL_UNSIGNED_INT8:
        return 1;
      case CL_SNORM_INT16:
      case CL_UNORM_INT16:
      case CL_SIGNED_INT16:
      case CL_UNSIGNED_INT16:
      case CL_HALF_FLOAT:
        return 2;
      case CL_SIGNED_INT32:
      case CL_UNSIGNED_INT32:
      case CL_FLOAT:
        return 4;
      default:
        return 0;
      }
    }

    static size_t getNumChannels(const cl_image_format& format)
    {
      switch (format.image_channel_order)
      {
      case CL_R:
      case CL_Rx:
      case CL_A:
      case CL_INTENSITY:
      case CL_LUMINANCE:
        return 1;
      case CL_RG:
      case CL_RGx:
      case CL_RA:
        return 2;
      case CL_RGB:
      case CL_RGBx:
        return 3;
      case CL_RGBA:
      case CL_ARGB:
      case CL_BGRA:
        return 4;
      default:
        return 0;
      }
    }

    static bool hasZeroAlphaBorder(const cl_image_format& format)
    {
      switch (format.image_channel_order)
      {
      case CL_A:
      case CL_INTENSITY:
      case CL_Rx:
      case CL_RA:
      case CL_RGx:
      case CL_RGBx:
      case CL_ARGB:
      case CL_BGRA:
      case CL_RGBA:
        return true;
      default:
        return false;
      }
    }

    DEFINE_BUILTIN(get_image_array_size)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);
      result.setSInt(image->desc.image_array_size);
    }

    DEFINE_BUILTIN(get_image_channel_data_type)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);
      result.setSInt(image->format.image_channel_data_type);
    }

    DEFINE_BUILTIN(get_image_channel_order)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);
      result.setSInt(image->format.image_channel_order);
    }

    DEFINE_BUILTIN(get_image_dim)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);

      result.setSInt(image->desc.image_width, 0);
      result.setSInt(image->desc.image_height, 1);
      if (result.num > 2)
      {
        result.setSInt(image->desc.image_depth, 2);
        result.setSInt(0, 3);
      }
    }

    DEFINE_BUILTIN(get_image_depth)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);
      result.setSInt(image->desc.image_depth);
    }

    DEFINE_BUILTIN(get_image_height)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);
      result.setSInt(image->desc.image_height);
    }

    DEFINE_BUILTIN(get_image_width)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);
      result.setSInt(image->desc.image_width);
    }

    static inline float getCoordinate(const llvm::Value *value, int index,
                                      char type, WorkItem *workItem)
    {
      switch (type)
      {
        case 'i':
          return workItem->getOperand(value).getSInt(index);
        case 'f':
          return workItem->getOperand(value).getFloat(index);
        default:
          FATAL_ERROR("Unsupported coordinate type: '%c'", type);
      }
    }

    static inline int getNearestCoordinate(uint32_t sampler,
                                           float n, // Normalized
                                           float u, // Unormalized
                                           size_t size)
    {
      switch (sampler & CLK_ADDRESS_MASK)
      {
        case CLK_ADDRESS_NONE:
          return floor(u);
        case CLK_ADDRESS_CLAMP_TO_EDGE:
          return _clamp_<int>(floor(u), 0, size - 1);
        case CLK_ADDRESS_CLAMP:
          return _clamp_<int>(floor(u), -1, size);
        case CLK_ADDRESS_REPEAT:
          return (int)floorf((n - floorf(n))*size) % size;
        case CLK_ADDRESS_MIRRORED_REPEAT:
          return _min_<int>((int)floorf(fabsf(n - 2.f * rintf(0.5f*n)) * size),
                            size - 1);
        default:
          FATAL_ERROR("Unsupported sampler addressing mode: %X",
                      sampler & CLK_ADDRESS_MASK);
      }
    }

    static inline float getAdjacentCoordinates(uint32_t sampler,
                                               float n, // Normalized
                                               float u, // Unnormalized
                                               size_t size,
                                               int *c0, int *c1)
    {
      switch (sampler & CLK_ADDRESS_MASK)
      {
        case CLK_ADDRESS_NONE:
          *c0 = floor(u);
          *c1 = floor(u) + 1;
          return u;
        case CLK_ADDRESS_CLAMP_TO_EDGE:
          *c0 = _clamp_<int>(floorf(u - 0.5f), 0, size - 1);
          *c1 = _clamp_<int>(floorf(u - 0.5f) + 1, 0, size - 1);
          return u;
        case CLK_ADDRESS_CLAMP:
          *c0 = _clamp_<int>((floorf(u - 0.5f)), -1, size);
          *c1 = _clamp_<int>((floorf(u - 0.5f)) + 1, -1, size);
          return u;
        case CLK_ADDRESS_REPEAT:
        {
          u = (n - floorf(n)) * size;
          *c0 = (int)floorf(u - 0.5f);
          *c1 = *c0 + 1;
          if (*c0 < 0) *c0 += size;
          if (*c1 >= size) *c1 -= size;
          return u;
        }
        case CLK_ADDRESS_MIRRORED_REPEAT:
        {
          u = fabsf(n - 2.0f * rintf(0.5f * n)) * size;
          *c0 = (int)floorf(u - 0.5f);
          *c1 = *c0 + 1;
          *c0 = _max_(*c0, 0);
          *c1 = _min_(*c1, (int)size-1);
          return u;
        }
        default:
          FATAL_ERROR("Unsupported sampler addressing mode: %X",
                      sampler & CLK_ADDRESS_MASK);
      }
    }

    static inline int getInputChannel(const cl_image_format& format,
                                      int output, float *ret)
    {
      int input = output;
      switch (format.image_channel_order)
      {
        case CL_R:
        case CL_Rx:
          if (output == 1)
          {
            *ret = 0.f;
            return -1;
          }
        case CL_RG:
        case CL_RGx:
          if (output == 2)
          {
            *ret = 0.f;
            return -1;
          }
        case CL_RGB:
        case CL_RGBx:
          if (output == 3)
          {
            *ret = 1.f;
            return -1;
          }
          break;
        case CL_RGBA:
          break;
        case CL_BGRA:
          if (output == 0) input = 2;
          if (output == 2) input = 0;
          break;
        case CL_ARGB:
          if (output == 0) input = 1;
          if (output == 1) input = 2;
          if (output == 2) input = 3;
          if (output == 3) input = 0;
          break;
        case CL_A:
          if (output == 3) input = 0;
          else
          {
            *ret = 0.f;
            return -1;
          }
          break;
        case CL_RA:
          if (output == 3) input = 1;
          else if (output != 0)
          {
            *ret = 0.f;
            return -1;
          }
          break;
        case CL_INTENSITY:
          input = 0;
          break;
        case CL_LUMINANCE:
          if (output == 3)
          {
            *ret = 1.f;
            return -1;
          }
          input = 0;
          break;
        default:
          FATAL_ERROR("Unsupported image channel order: %X",
                      format.image_channel_order);
      }
      return input;
    }

    static inline float readNormalizedColor(const Image *image,
                                            WorkItem *workItem,
                                            int i, int j, int k,
                                            int layer, int c)
    {
      // Check for out-of-range coordinages
      if (i < 0 || i >= image->desc.image_width ||
          j < 0 || j >= image->desc.image_height ||
          k < 0 || k >= image->desc.image_depth)
      {
        // Return border color
        if (c == 3 && !hasZeroAlphaBorder(image->format))
        {
          return 1.f;
        }
        return 0.f;
      }

      // Remap channels
      float ret;
      int channel = getInputChannel(image->format, c, &ret);
      if (channel < 0)
      {
        return ret;
      }

      // Calculate pixel address
      size_t channelSize = getChannelSize(image->format);
      size_t numChannels = getNumChannels(image->format);
      size_t pixelSize = channelSize*numChannels;
      size_t address = image->address
                        + (i + (j + (k + layer*image->desc.image_depth)
                        * image->desc.image_height)
                        * image->desc.image_width) * pixelSize
                        + channel*channelSize;

      // Load channel data
      unsigned char *data = workItem->m_pool.alloc(channelSize);
      if (!workItem->getMemory(AddrSpaceGlobal)->load(data, address,
                                                       channelSize))
      {
        return 0.f;
      }

      // Compute normalized color value
      float color;
      switch (image->format.image_channel_data_type)
      {
        case CL_SNORM_INT8:
          color = _clamp_(*(int8_t*)data / 127.f, -1.f, 1.f);
          break;
        case CL_UNORM_INT8:
          color = _clamp_(*(uint8_t*)data / 255.f, 0.f, 1.f);
          break;
        case CL_SNORM_INT16:
          color = _clamp_(*(int16_t*)data / 32767.f, -1.f, 1.f);
          break;
        case CL_UNORM_INT16:
          color = _clamp_(*(uint16_t*)data / 65535.f, 0.f, 1.f);
          break;
        case CL_FLOAT:
          color = *(float*)data;
          break;
        case CL_HALF_FLOAT:
          color = halfToFloat(*(uint16_t*)data);
          break;
        default:
          FATAL_ERROR("Unsupported image channel data type: %X",
                      image->format.image_channel_data_type);
      }

      return color;
    }

    static inline int32_t readSignedColor(const Image *image,
                                          WorkItem *workItem,
                                          int i, int j, int k,
                                          int layer, int c)
    {
      // Check for out-of-range coordinages
      if (i < 0 || i >= image->desc.image_width ||
          j < 0 || j >= image->desc.image_height ||
          k < 0 || k >= image->desc.image_depth)
      {
        // Return border color
        if (c == 3 && !hasZeroAlphaBorder(image->format))
        {
          return 1.f;
        }
        return 0.f;
      }

      // Remap channels
      float ret;
      int channel = getInputChannel(image->format, c, &ret);
      if (channel < 0)
      {
        return ret;
      }

      // Calculate pixel address
      size_t channelSize = getChannelSize(image->format);
      size_t numChannels = getNumChannels(image->format);
      size_t pixelSize = channelSize*numChannels;
      size_t address = image->address
                        + (i + (j + (k + layer*image->desc.image_depth)
                        * image->desc.image_height)
                        * image->desc.image_width) * pixelSize
                        + channel*channelSize;

      // Load channel data
      unsigned char *data = workItem->m_pool.alloc(channelSize);
      if (!workItem->getMemory(AddrSpaceGlobal)->load(data, address,
                                                       channelSize))
      {
        return 0;
      }

      // Compute unnormalized color value
      int32_t color;
      switch (image->format.image_channel_data_type)
      {
        case CL_SIGNED_INT8:
          color = *(int8_t*)data;
          break;
        case CL_SIGNED_INT16:
          color = *(int16_t*)data;
          break;
        case CL_SIGNED_INT32:
          color = *(int32_t*)data;
          break;
        default:
          FATAL_ERROR("Unsupported image channel data type: %X",
                      image->format.image_channel_data_type);
      }

      return color;
    }

    static inline uint32_t readUnsignedColor(const Image *image,
                                             WorkItem *workItem,
                                             int i, int j, int k,
                                             int layer, int c)
    {
      // Check for out-of-range coordinages
      if (i < 0 || i >= image->desc.image_width ||
          j < 0 || j >= image->desc.image_height ||
          k < 0 || k >= image->desc.image_depth)
      {
        // Return border color
        if (c == 3 && !hasZeroAlphaBorder(image->format))
        {
          return 1.f;
        }
        return 0.f;
      }

      // Remap channels
      float ret;
      int channel = getInputChannel(image->format, c, &ret);
      if (channel < 0)
      {
        return ret;
      }

      // Calculate pixel address
      size_t channelSize = getChannelSize(image->format);
      size_t numChannels = getNumChannels(image->format);
      size_t pixelSize = channelSize*numChannels;
      size_t address = image->address
                        + (i + (j + (k + layer*image->desc.image_depth)
                        * image->desc.image_height)
                        * image->desc.image_width) * pixelSize
                        + channel*channelSize;

      // Load channel data
      unsigned char *data = workItem->m_pool.alloc(channelSize);
      if (!workItem->getMemory(AddrSpaceGlobal)->load(data, address,
                                                       channelSize))
      {
        return 0;
      }

      // Load color value
      uint32_t color;
      switch (image->format.image_channel_data_type)
      {
        case CL_UNSIGNED_INT8:
          color = *(uint8_t*)data;
          break;
        case CL_UNSIGNED_INT16:
          color = *(uint16_t*)data;
          break;
        case CL_UNSIGNED_INT32:
          color = *(uint32_t*)data;
          break;
        default:
          FATAL_ERROR("Unsupported image channel data type: %X",
                      image->format.image_channel_data_type);
      }

      return color;
    }

    static inline float frac(float x)
    {
      return x - floorf(x);
    }

    static inline float interpolate(float v000, float v010,
                                    float v100, float v110,
                                    float v001, float v011,
                                    float v101, float v111,
                                    float a, float b, float c)
    {
      return  (1-a) * (1-b) * (1-c) * v000
            +   a   * (1-b) * (1-c) * v100
            + (1-a) *    b  * (1-c) * v010
            +    a  *    b  * (1-c) * v110
            + (1-a) * (1-b) *    c  * v001
            +    a  * (1-b) *    c  * v101
            + (1-a) *    b  *    c  * v011
            +    a  *    b  *    c  * v111;
    }

    DEFINE_BUILTIN(read_imagef)
    {
      const Image *image = *(Image**)(workItem->getValue(ARG(0)).data);

      uint32_t sampler = CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
      int coordIndex = 1;

      // Check for sampler version
      if (callInst->getNumArgOperands() > 2)
      {
        sampler = UARG(1);
        coordIndex = 2;
      }

      // Get coordinates
      float s = 0.f, t = 0.f, r = 0.f;
      char coordType = *overload.rbegin();
      s = getCoordinate(ARG(coordIndex), 0, coordType, workItem);
      if (ARG(coordIndex)->getType()->isVectorTy())
      {
        t = getCoordinate(ARG(coordIndex), 1, coordType, workItem);
        if (ARG(coordIndex)->getType()->getVectorNumElements() > 2)
        {
          r = getCoordinate(ARG(coordIndex), 2, coordType, workItem);
        }
      }

      // Get unnormalized coordinates
      float u = 0.f, v = 0.f, w = 0.f;
      bool noormCoords = sampler & CLK_NORMALIZED_COORDS_TRUE;
      if (noormCoords)
      {
        u = s * image->desc.image_width;
        v = t * image->desc.image_height;
        w = r * image->desc.image_depth;
      }
      else
      {
        u = s;
        v = t;
        w = r;
      }

      // Get array layer index
      int layer = 0;
      if (image->desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY)
      {
        layer = _clamp_<int>(rintf(t), 0, image->desc.image_array_size - 1);
        v = t = 0.f;
      }
      else if (image->desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)
      {
        layer = _clamp_<int>(rintf(r), 0, image->desc.image_array_size - 1);
        w = r = 0.f;
      }

      float values[4];
      if (sampler & CLK_FILTER_LINEAR)
      {
        // Get coordinates of adjacent pixels
        int i0 = 0, i1 = 0, j0 = 0, j1 = 0, k0 = 0, k1 = 0;
        u = getAdjacentCoordinates(sampler, s, u, image->desc.image_width,
                                   &i0, &i1);
        v = getAdjacentCoordinates(sampler, t, v, image->desc.image_height,
                                   &j0, &j1);
        w = getAdjacentCoordinates(sampler, r, w, image->desc.image_depth,
                                   &k0, &k1);

        // Make sure y and z coordinates are equal for 1 and 2D images
        if (image->desc.image_type == CL_MEM_OBJECT_IMAGE1D ||
            image->desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY)
        {
          j0 = j1;
          k0 = k1;
        }
        else if (image->desc.image_type == CL_MEM_OBJECT_IMAGE2D ||
                 image->desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)
        {
          k0 = k1;
        }

        // Perform linear interpolation
        float a = frac(u - 0.5f);
        float b = frac(v - 0.5f);
        float c = frac(w - 0.5f);
        for (int i = 0; i < 4; i++)
        {
          values[i] = interpolate(
            readNormalizedColor(image, workItem, i0, j0, k0, layer, i),
            readNormalizedColor(image, workItem, i0, j1, k0, layer, i),
            readNormalizedColor(image, workItem, i1, j0, k0, layer, i),
            readNormalizedColor(image, workItem, i1, j1, k0, layer, i),
            readNormalizedColor(image, workItem, i0, j0, k1, layer, i),
            readNormalizedColor(image, workItem, i0, j1, k1, layer, i),
            readNormalizedColor(image, workItem, i1, j0, k1, layer, i),
            readNormalizedColor(image, workItem, i1, j1, k1, layer, i),
            a, b, c);
        }
      }
      else
      {
        // Read values from nearest pixel
        int i = getNearestCoordinate(sampler, s, u, image->desc.image_width);
        int j = getNearestCoordinate(sampler, t, v, image->desc.image_height);
        int k = getNearestCoordinate(sampler, r, w, image->desc.image_depth);
        values[0] = readNormalizedColor(image, workItem, i, j, k, layer, 0);
        values[1] = readNormalizedColor(image, workItem, i, j, k, layer, 1);
        values[2] = readNormalizedColor(image, workItem, i, j, k, layer, 2);
        values[3] = readNormalizedColor(image, workItem, i, j, k, layer, 3);
      }

      // Store values in result
      for (int i = 0; i < 4; i++)
      {
        result.setFloat(values[i], i);
      }
    }

    DEFINE_BUILTIN(read_imagei)
    {
      const Image *image = *(Image**)(workItem->getValue(ARG(0)).data);

      uint32_t sampler = CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
      int coordIndex = 1;

      // Check for sampler version
      if (callInst->getNumArgOperands() > 2)
      {
        sampler = UARG(1);
        coordIndex = 2;
      }

      // Get coordinates
      float s = 0.f, t = 0.f, r = 0.f;
      char coordType = *overload.rbegin();
      s = getCoordinate(ARG(coordIndex), 0, coordType, workItem);
      if (ARG(coordIndex)->getType()->isVectorTy())
      {
        t = getCoordinate(ARG(coordIndex), 1, coordType, workItem);
        if (ARG(coordIndex)->getType()->getVectorNumElements() > 2)
        {
          r = getCoordinate(ARG(coordIndex), 2, coordType, workItem);
        }
      }

      // Get unnormalized coordinates
      float u = 0.f, v = 0.f, w = 0.f;
      bool noormCoords = sampler & CLK_NORMALIZED_COORDS_TRUE;
      if (noormCoords)
      {
        u = s * image->desc.image_width;
        v = t * image->desc.image_height;
        w = r * image->desc.image_depth;
      }
      else
      {
        u = s;
        v = t;
        w = r;
      }

      // Get array layer index
      int layer = 0;
      if (image->desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY)
      {
        layer = _clamp_<int>(rintf(t), 0, image->desc.image_array_size - 1);
        v = t = 0.f;
      }
      else if (image->desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)
      {
        layer = _clamp_<int>(rintf(r), 0, image->desc.image_array_size - 1);
        w = r = 0.f;
      }

      // Read values from nearest pixel
      int32_t values[4];
      int i = getNearestCoordinate(sampler, s, u, image->desc.image_width);
      int j = getNearestCoordinate(sampler, t, v, image->desc.image_height);
      int k = getNearestCoordinate(sampler, r, w, image->desc.image_depth);
      values[0] = readSignedColor(image, workItem, i, j, k, layer, 0);
      values[1] = readSignedColor(image, workItem, i, j, k, layer, 1);
      values[2] = readSignedColor(image, workItem, i, j, k, layer, 2);
      values[3] = readSignedColor(image, workItem, i, j, k, layer, 3);

      // Store values in result
      for (int i = 0; i < 4; i++)
      {
        result.setSInt(values[i], i);
      }
    }

    DEFINE_BUILTIN(read_imageui)
    {
      const Image *image = *(Image**)(workItem->getValue(ARG(0)).data);

      uint32_t sampler = CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
      int coordIndex = 1;

      // Check for sampler version
      if (callInst->getNumArgOperands() > 2)
      {
        sampler = UARG(1);
        coordIndex = 2;
      }

      // Get coordinates
      float s = 0.f, t = 0.f, r = 0.f;
      char coordType = *overload.rbegin();
      s = getCoordinate(ARG(coordIndex), 0, coordType, workItem);
      if (ARG(coordIndex)->getType()->isVectorTy())
      {
        t = getCoordinate(ARG(coordIndex), 1, coordType, workItem);
        if (ARG(coordIndex)->getType()->getVectorNumElements() > 2)
        {
          r = getCoordinate(ARG(coordIndex), 2, coordType, workItem);
        }
      }

      // Get unnormalized coordinates
      float u = 0.f, v = 0.f, w = 0.f;
      bool noormCoords = sampler & CLK_NORMALIZED_COORDS_TRUE;
      if (noormCoords)
      {
        u = s * image->desc.image_width;
        v = t * image->desc.image_height;
        w = r * image->desc.image_depth;
      }
      else
      {
        u = s;
        v = t;
        w = r;
      }

      // Get array layer index
      int layer = 0;
      if (image->desc.image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY)
      {
        layer = _clamp_<int>(rintf(t), 0, image->desc.image_array_size - 1);
        v = t = 0.f;
      }
      else if (image->desc.image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)
      {
        layer = _clamp_<int>(rintf(r), 0, image->desc.image_array_size - 1);
        w = r = 0.f;
      }

      // Read values from nearest pixel
      uint32_t values[4];
      int i = getNearestCoordinate(sampler, s, u, image->desc.image_width);
      int j = getNearestCoordinate(sampler, t, v, image->desc.image_height);
      int k = getNearestCoordinate(sampler, r, w, image->desc.image_depth);
      values[0] = readUnsignedColor(image, workItem, i, j, k, layer, 0);
      values[1] = readUnsignedColor(image, workItem, i, j, k, layer, 1);
      values[2] = readUnsignedColor(image, workItem, i, j, k, layer, 2);
      values[3] = readUnsignedColor(image, workItem, i, j, k, layer, 3);

      // Store values in result
      for (int i = 0; i < 4; i++)
      {
        result.setUInt(values[i], i);
      }
    }

    DEFINE_BUILTIN(write_imagef)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);

      // Get pixel coordinates
      int x, y = 0, z = 0 ;
      x = SARGV(1, 0);
      if (ARG(1)->getType()->isVectorTy())
      {
        y = SARGV(1, 1);
        if (ARG(1)->getType()->getVectorNumElements() > 2)
        {
          z = SARGV(1, 2);
        }
      }

      // Get color data
      float values[4] =
      {
        (float)FARGV(2, 0),
        (float)FARGV(2, 1),
        (float)FARGV(2, 2),
        (float)FARGV(2, 3),
      };

      // Re-order color values
      switch (image->format.image_channel_order)
      {
      case CL_R:
      case CL_Rx:
      case CL_RG:
      case CL_RGx:
      case CL_RGB:
      case CL_RGBx:
      case CL_RGBA:
      case CL_INTENSITY:
      case CL_LUMINANCE:
        break;
      case CL_A:
        values[0] = values[3];
        break;
      case CL_RA:
        values[1] = values[3];
        break;
      case CL_ARGB:
        swap(values[2], values[3]);
        swap(values[1], values[2]);
        swap(values[0], values[1]);
        break;
      case CL_BGRA:
        swap(values[0], values[2]);
        break;
      default:
        FATAL_ERROR("Unsupported image channel order: %X",
                    image->format.image_channel_order);
      }

      size_t channelSize = getChannelSize(image->format);
      size_t numChannels = getNumChannels(image->format);
      size_t pixelSize = channelSize*numChannels;
      size_t pixelAddress = image->address
                            + (x + (y + z*image->desc.image_height)
                            * image->desc.image_width) * pixelSize;

      // Generate channel values
      Memory *memory = workItem->getMemory(AddrSpaceGlobal);
      unsigned char *data = workItem->m_pool.alloc(channelSize*numChannels);
      for (int i = 0; i < numChannels; i++)
      {
        switch (image->format.image_channel_data_type)
        {
          case CL_SNORM_INT8:
            ((int8_t*)data)[i] = rint(_clamp_(values[i] * 127.f,
                                              -128.f, 127.f));
            break;
          case CL_UNORM_INT8:
            data[i] = rint(_clamp_(values[i] * 255.f, 0.f, 255.f));
            break;
          case CL_SNORM_INT16:
            ((int16_t*)data)[i] = rint(_clamp_(values[i] * 32767.f,
                                               -32768.f, 32767.f));
            break;
          case CL_UNORM_INT16:
            ((uint16_t*)data)[i] = rint(_clamp_(values[i] * 65535.f,
                                                0.f, 65535.f));
            break;
          case CL_FLOAT:
            ((float*)data)[i] = values[i];
            break;
          case CL_HALF_FLOAT:
            ((uint16_t*)data)[i] = floatToHalf(values[i]);
            break;
          default:
            FATAL_ERROR("Unsupported image channel data type: %X",
                        image->format.image_channel_data_type);
        }
      }

      // Write pixel data
      memory->store(data, pixelAddress, channelSize*numChannels);
    }

    DEFINE_BUILTIN(write_imagei)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);

      // Get pixel coordinates
      int x, y = 0, z = 0 ;
      x = SARGV(1, 0);
      if (ARG(1)->getType()->isVectorTy())
      {
        y = SARGV(1, 1);
        if (ARG(1)->getType()->getVectorNumElements() > 2)
        {
          z = SARGV(1, 2);
        }
      }

      // Get color data
      int32_t values[4] =
      {
        (int32_t)SARGV(2, 0),
        (int32_t)SARGV(2, 1),
        (int32_t)SARGV(2, 2),
        (int32_t)SARGV(2, 3),
      };

      // Re-order color values
      switch (image->format.image_channel_order)
      {
      case CL_R:
      case CL_Rx:
      case CL_RG:
      case CL_RGx:
      case CL_RGB:
      case CL_RGBx:
      case CL_RGBA:
      case CL_INTENSITY:
      case CL_LUMINANCE:
        break;
      case CL_A:
        values[0] = values[3];
        break;
      case CL_RA:
        values[1] = values[3];
        break;
      case CL_ARGB:
        swap(values[2], values[3]);
        swap(values[1], values[2]);
        swap(values[0], values[1]);
        break;
      case CL_BGRA:
        swap(values[0], values[2]);
        break;
      default:
        FATAL_ERROR("Unsupported image channel order: %X",
                    image->format.image_channel_order);
      }

      size_t channelSize = getChannelSize(image->format);
      size_t numChannels = getNumChannels(image->format);
      size_t pixelSize = channelSize*numChannels;
      size_t pixelAddress = image->address
                            + (x + (y + z*image->desc.image_height)
                            * image->desc.image_width) * pixelSize;

      // Generate channel values
      Memory *memory = workItem->getMemory(AddrSpaceGlobal);
      unsigned char *data = workItem->m_pool.alloc(channelSize*numChannels);
      for (int i = 0; i < numChannels; i++)
      {
        switch (image->format.image_channel_data_type)
        {
          case CL_SIGNED_INT8:
            ((int8_t*)data)[i] = _clamp_(values[i], -128, 127);
            break;
          case CL_SIGNED_INT16:
            ((int16_t*)data)[i] = _clamp_(values[i], -32768, 32767);
            break;
          case CL_SIGNED_INT32:
            ((int32_t*)data)[i] = values[i];
            break;
          default:
            FATAL_ERROR("Unsupported image channel data type: %X",
                        image->format.image_channel_data_type);
        }
      }

      // Write pixel data
      memory->store(data, pixelAddress, channelSize*numChannels);
    }

    DEFINE_BUILTIN(write_imageui)
    {
      Image *image = *(Image**)(workItem->getValue(ARG(0)).data);

      // Get pixel coordinates
      int x, y = 0, z = 0 ;
      x = SARGV(1, 0);
      if (ARG(1)->getType()->isVectorTy())
      {
        y = SARGV(1, 1);
        if (ARG(1)->getType()->getVectorNumElements() > 2)
        {
          z = SARGV(1, 2);
        }
      }

      // Get color data
      uint32_t values[4] =
      {
        (uint32_t)SARGV(2, 0),
        (uint32_t)SARGV(2, 1),
        (uint32_t)SARGV(2, 2),
        (uint32_t)SARGV(2, 3),
      };

      // Re-order color values
      switch (image->format.image_channel_order)
      {
      case CL_R:
      case CL_Rx:
      case CL_RG:
      case CL_RGx:
      case CL_RGB:
      case CL_RGBx:
      case CL_RGBA:
      case CL_INTENSITY:
      case CL_LUMINANCE:
        break;
      case CL_A:
        values[0] = values[3];
        break;
      case CL_RA:
        values[1] = values[3];
        break;
      case CL_ARGB:
        swap(values[2], values[3]);
        swap(values[1], values[2]);
        swap(values[0], values[1]);
        break;
      case CL_BGRA:
        swap(values[0], values[2]);
        break;
      default:
        FATAL_ERROR("Unsupported image channel order: %X",
                    image->format.image_channel_order);
      }

      size_t channelSize = getChannelSize(image->format);
      size_t numChannels = getNumChannels(image->format);
      size_t pixelSize = channelSize*numChannels;
      size_t pixelAddress = image->address
                            + (x + (y + z*image->desc.image_height)
                            * image->desc.image_width) * pixelSize;

      // Generate channel values
      Memory *memory = workItem->getMemory(AddrSpaceGlobal);
      unsigned char *data = workItem->m_pool.alloc(channelSize*numChannels);
      for (int i = 0; i < numChannels; i++)
      {
        switch (image->format.image_channel_data_type)
        {
          case CL_UNSIGNED_INT8:
            ((uint8_t*)data)[i] = _min_<uint32_t>(values[i], UINT8_MAX);
            break;
          case CL_UNSIGNED_INT16:
            ((uint16_t*)data)[i] = _min_<uint32_t>(values[i], UINT16_MAX);
            break;
          case CL_UNSIGNED_INT32:
            ((uint32_t*)data)[i] = values[i];
            break;
          default:
            FATAL_ERROR("Unsupported image channel data type: %X",
                        image->format.image_channel_data_type);
        }
      }

      // Write pixel data
      memory->store(data, pixelAddress, channelSize*numChannels);
    }


    ///////////////////////
    // Integer Functions //
    ///////////////////////

    DEFINE_BUILTIN(abs_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
            result.setUInt(UARGV(0,i), i);
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
            result.setSInt(abs(SARGV(0,i)), i);
            break;
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    DEFINE_BUILTIN(abs_diff)
    {
      for (int i = 0; i < result.num; i++)
      {
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
          {
            uint64_t a = UARGV(0, i);
            uint64_t b = UARGV(1, i);
            result.setUInt(_max_(a,b) - _min_(a,b), i);
            break;
          }
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t a = SARGV(0, i);
            int64_t b = SARGV(1, i);
            result.setSInt(_max_(a,b) - _min_(a,b), i);
            break;
          }
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    DEFINE_BUILTIN(add_sat)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t uresult = UARGV(0,i) + UARGV(1,i);
        int64_t  sresult = SARGV(0,i) + SARGV(1,i);
        switch (getOverloadArgType(overload))
        {
          case 'h':
            uresult = _min_<uint64_t>(uresult, UINT8_MAX);
            result.setUInt(uresult, i);
            break;
          case 't':
            uresult = _min_<uint64_t>(uresult, UINT16_MAX);
            result.setUInt(uresult, i);
            break;
          case 'j':
            uresult = _min_<uint64_t>(uresult, UINT32_MAX);
            result.setUInt(uresult, i);
            break;
          case 'm':
            uresult = (UARGV(1, i) > uresult) ? UINT64_MAX : uresult;
            result.setUInt(uresult, i);
            break;
          case 'c':
            sresult = _clamp_<int64_t>(sresult, INT8_MIN, INT8_MAX);
            result.setSInt(sresult, i);
            break;
          case 's':
            sresult = _clamp_<int64_t>(sresult, INT16_MIN, INT16_MAX);
            result.setSInt(sresult, i);
            break;
          case 'i':
            sresult = _clamp_<int64_t>(sresult, INT32_MIN, INT32_MAX);
            result.setSInt(sresult, i);
            break;
          case 'l':
            if ((SARGV(0,i)>0) == (SARGV(1,i)>0) &&
                (SARGV(0,i)>0) != (sresult>0))
            {
              sresult = (SARGV(0,i)>0) ? INT64_MAX : INT64_MIN;
            }
            result.setSInt(sresult, i);
            break;
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    DEFINE_BUILTIN(clz)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t x = UARGV(0, i);
        int nz = 0;
        while (x)
        {
          x >>= 1;
          nz++;
        }

        uint64_t r = ((result.size<<3) - nz);
        result.setUInt(r, i);
      }
    }

    DEFINE_BUILTIN(hadd)
    {
      for (int i = 0; i < result.num; i++)
      {
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
          {
            uint64_t a = UARGV(0, i);
            uint64_t b = UARGV(1, i);
            uint64_t c = (a > UINT64_MAX-b) ? (1L<<63) : 0;
            result.setUInt(((a + b) >> 1) | c, i);
            break;
          }
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t a = SARGV(0, i);
            int64_t b = SARGV(1, i);
            int64_t c = (a & b) & 1;
            result.setSInt((a>>1) + (b>>1) + c, i);
            break;
          }
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    static uint64_t _mad_(uint64_t a, uint64_t b, uint64_t c)
    {
      return a*b + c;
    }

    static uint64_t _umul_hi_(uint64_t x, uint64_t y, uint64_t bits)
    {
      if (bits == 64)
      {
        uint64_t xl = x & UINT32_MAX;
        uint64_t xh = x >> 32;
        uint64_t yl = y & UINT32_MAX;
        uint64_t yh = y >> 32;

        uint64_t xlyl = xl*yl;
        uint64_t xlyh = xl*yh;
        uint64_t xhyl = xh*yl;
        uint64_t xhyh = xh*yh;

        uint64_t  a = xhyl + ((xlyl)>>32);
        uint64_t al = a & UINT32_MAX;
        uint64_t ah = a >> 32;
        uint64_t  b = ((al + xlyh)>>32) + ah;

        return xh*yh + b;
      }
      else
      {
        return (x*y) >> bits;
      }
    }

    static int64_t _smul_hi_(int64_t x, int64_t y, int64_t bits)
    {
      if (bits == 64)
      {
        // TODO: Sometimes 1 out
        int64_t xl = x & UINT32_MAX;
        int64_t xh = x >> 32;
        int64_t yl = y & UINT32_MAX;
        int64_t yh = y >> 32;

        int64_t xlyl = xl*yl;
        int64_t xlyh = xl*yh;
        int64_t xhyl = xh*yl;
        int64_t xhyh = xh*yh;

        int64_t  a = xhyl + ((xlyl)>>32);
        int64_t al = a & UINT32_MAX;
        int64_t ah = a >> 32;
        int64_t  b = ((al + xlyh)>>32) + ah;

        return xh*yh + b;
      }
      else
      {
        return (x*y) >> bits;
      }
    }

    DEFINE_BUILTIN(mad_hi)
    {
      for (int i = 0; i < result.num; i++)
      {
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
          {
            uint64_t r =
              _umul_hi_(UARGV(0, i), UARGV(1, i), result.size<<3) + UARGV(2, i);
            result.setUInt(r, i);
            break;
          }
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t r =
              _smul_hi_(SARGV(0, i), SARGV(1, i), result.size<<3) + SARGV(2, i);
            result.setSInt(r, i);
            break;
          }
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    DEFINE_BUILTIN(mad_sat)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t uresult = UARGV(0,i)*UARGV(1,i) + UARGV(2,i);
        int64_t  sresult = SARGV(0,i)*SARGV(1,i) + SARGV(2,i);
        switch (getOverloadArgType(overload))
        {
          case 'h':
            uresult = _min_<uint64_t>(uresult, UINT8_MAX);
            result.setUInt(uresult, i);
            break;
          case 't':
            uresult = _min_<uint64_t>(uresult, UINT16_MAX);
            result.setUInt(uresult, i);
            break;
          case 'j':
            uresult = _min_<uint64_t>(uresult, UINT16_MAX);
            result.setUInt(uresult, i);
            break;
          case 'm':
          {
            uint64_t hi = _umul_hi_(UARGV(0, i), UARGV(1, i), 64);
            if (hi || UARGV(2, i) > uresult)
            {
              uresult = UINT64_MAX;
            }
            result.setUInt(uresult, i);
            break;
          }
          case 'c':
            sresult = _clamp_<int64_t>(sresult, INT8_MIN, INT8_MAX);
            result.setSInt(sresult, i);
            break;
          case 's':
            sresult = _clamp_<int64_t>(sresult, INT16_MIN, INT16_MAX);
            result.setSInt(sresult, i);
            break;
          case 'i':
            sresult = _clamp_<int64_t>(sresult, INT32_MIN, INT32_MAX);
            result.setSInt(sresult, i);
            break;
          case 'l':
            // Check for overflow in multiplication
            if (_smul_hi_(SARGV(0, i), SARGV(1, i), 64))
            {
              sresult = (SARGV(0,i)>0) ^ (SARGV(1,i)>0) ? INT64_MIN : INT64_MAX;
            }
            else
            {
              // Check for overflow in addition
              int64_t m = SARGV(0, i) * SARGV(1, i);
              if ((m>0) == (SARGV(2,i)>0) && (m>0) != (sresult>0))
              {
                sresult = (m>0) ? INT64_MAX : INT64_MIN;
              }
            }
            result.setSInt(sresult, i);
            break;
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    static uint64_t _mul_(uint64_t a, uint64_t b)
    {
      return a*b;
    }

    DEFINE_BUILTIN(mul_hi)
    {
      for (int i = 0; i < result.num; i++)
      {
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
          {
            uint64_t r = _umul_hi_(UARGV(0, i), UARGV(1, i), result.size<<3);
            result.setUInt(r, i);
            break;
          }
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t r = _smul_hi_(SARGV(0, i), SARGV(1, i), result.size<<3);
            result.setSInt(r, i);
            break;
          }
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    static uint64_t _popcount_(uint64_t x)
    {
      int i = 0;
      while (x)
      {
        i += (x & 0x1);
        x >>= 1;
      }
      return i;
    }

    DEFINE_BUILTIN(rhadd)
    {
      for (int i = 0; i < result.num; i++)
      {
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
          {
            uint64_t a = UARGV(0, i);
            uint64_t b = UARGV(1, i);
            uint64_t c = (a > UINT64_MAX-(b+1)) ? (1L<<63) : 0;
            result.setUInt(((a + b + 1) >> 1) | c, i);
            break;
          }
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t a = SARGV(0, i);
            int64_t b = SARGV(1, i);
            int64_t c = (a | b) & 1;
            result.setSInt((a>>1) + (b>>1) + c, i);
            break;
          }
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    DEFINE_BUILTIN(rotate)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t width = (result.size << 3);
        uint64_t v  = UARGV(0, i);
        uint64_t ls = UARGV(1, i) % width;
        uint64_t rs = width - ls;
        result.setUInt((v << ls) | (v >> rs), i);
      }
    }

    DEFINE_BUILTIN(sub_sat)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t uresult = UARGV(0,i) - UARGV(1,i);
        int64_t  sresult = SARGV(0,i) - SARGV(1,i);
        switch (getOverloadArgType(overload))
        {
          case 'h':
            uresult = uresult > UINT8_MAX ? 0 : uresult;
            result.setUInt(uresult, i);
            break;
          case 't':
            uresult = uresult > UINT16_MAX ? 0 : uresult;
            result.setUInt(uresult, i);
            break;
          case 'j':
            uresult = uresult > UINT32_MAX ? 0 : uresult;
            result.setUInt(uresult, i);
            break;
          case 'm':
            uresult = (UARGV(1, i) > UARGV(0, i)) ? 0 : uresult;
            result.setUInt(uresult, i);
            break;
          case 'c':
            sresult = _clamp_<int64_t>(sresult, INT8_MIN, INT8_MAX);
            result.setSInt(sresult, i);
            break;
          case 's':
            sresult = _clamp_<int64_t>(sresult, INT16_MIN, INT16_MAX);
            result.setSInt(sresult, i);
            break;
          case 'i':
            sresult = _clamp_<int64_t>(sresult, INT32_MIN, INT32_MAX);
            result.setSInt(sresult, i);
            break;
          case 'l':
            if ((SARGV(0,i)>0) != (SARGV(1,i)>0) &&
                (SARGV(0,i)>0) != (sresult>0))
            {
              sresult = (SARGV(0,i)>0) ? INT64_MAX : INT64_MIN;
            }
            result.setSInt(sresult, i);
            break;
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    DEFINE_BUILTIN(upsample)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t r = (UARGV(0,i)<<(result.size<<2)) | UARGV(1, i);
        result.setUInt(r, i);
      }
    }


    ////////////////////
    // Math Functions //
    ////////////////////

    static double _acospi_(double x){ return (acos(x) / M_PI); }
    static double _asinpi_(double x){ return (asin(x) / M_PI); }
    static double _atanpi_(double x){ return (atan(x) / M_PI); }
    static double _atan2pi_(double x, double y){ return (atan2(x, y) / M_PI); }
    static double _cospi_(double x){ return (cos(x * M_PI)); }
    static double _exp10_(double x){ return pow(10, x); }
    static double _fdivide_(double x, double y){ return x/y; }
    static double _frecip_(double x){ return 1.0/x; }
    static double _rsqrt_(double x){ return 1.0 / sqrt(x); }
    static double _sinpi_(double x){ return (sin(x * M_PI)); }
    static double _tanpi_(double x){ return (tan(x * M_PI)); }

    static double _maxmag_(double x, double y)
    {
      double _x = fabs(x);
      double _y = fabs(y);
      if (_x > _y)
      {
        return x;
      }
      else if (_y > _x)
      {
        return y;
      }
      else
      {
        return fmax(x, y);
      }
    }

    static double _minmag_(double x, double y)
    {
      double _x = fabs(x);
      double _y = fabs(y);
      if (_x < _y)
      {
        return x;
      }
      else if (_y < _x)
      {
        return y;
      }
      else
      {
        return fmin(x, y);
      }
    }

    DEFINE_BUILTIN(fract)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t iptr = PARG(1);
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        double fl = floor(x);
#if defined(_WIN32) && !defined(__MINGW32__)
        double r = fmin(x - fl, nextafter(1, 0));
#else
        double r = fmin(x - fl, 0x1.fffffep-1f);
#endif

        size_t offset = i*result.size;
        result.setFloat(fl, i);
        memory->store(result.data + offset, iptr + offset, result.size);
        result.setFloat(r, i);
      }
    }

    DEFINE_BUILTIN(frexp_builtin)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t iptr = PARG(1);
      for (int i = 0; i < result.num; i++)
      {
        int32_t e;
        double r = frexp(FARGV(0, i), &e);
        memory->store((const unsigned char*)&e, iptr + i*4, 4);
        result.setFloat(r, i);
      }
    }

    DEFINE_BUILTIN(ilogb_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setSInt(ilogb(FARGV(0, i)), i);
      }
    }

    DEFINE_BUILTIN(ldexp_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setFloat(ldexp(FARGV(0, i), SARGV(1, i)), i);
      }
    }

    DEFINE_BUILTIN(lgamma_r)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t signp = PARG(1);
      for (int i = 0; i < result.num; i++)
      {
        double r = lgamma(FARGV(0, i));
        int32_t s = (tgamma(FARGV(0, i)) < 0 ? -1 : 1);
        memory->store((const unsigned char*)&s, signp + i*4, 4);
        result.setFloat(r, i);
      }
    }

    DEFINE_BUILTIN(modf_builtin)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t iptr = PARG(1);
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        double integral = trunc(x);
        double fractional = copysign(::isinf(x) ? 0.0 : x - integral, x);

        size_t offset = i*result.size;
        result.setFloat(integral, i);
        memory->store(result.data + offset, iptr + offset, result.size);
        result.setFloat(fractional, i);
      }
    }

    DEFINE_BUILTIN(nan_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t nancode = UARGV(0, i);
        result.setFloat(nan(""), i);
      }
    }

    DEFINE_BUILTIN(pown)
    {
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        int32_t y = SARGV(1, i);
        result.setFloat(pow(x, y), i);
      }
    }

    DEFINE_BUILTIN(remquo_builtin)
    {
      Memory *memory =
        workItem->getMemory(ARG(2)->getType()->getPointerAddressSpace());

      size_t quop = PARG(2);
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        double y = FARGV(1, i);

        int32_t quo;
        double rem = remquo(x, y, &quo);
        memory->store((const unsigned char*)&quo, quop + i*4, 4);
        result.setFloat(rem, i);
      }
    }

    DEFINE_BUILTIN(rootn)
    {
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        int y = SARGV(1, i);
        result.setFloat(pow(x, (double)(1.0/y)), i);
      }
    }

    DEFINE_BUILTIN(sincos)
    {
      Memory *memory =
        workItem->getMemory(ARG(1)->getType()->getPointerAddressSpace());

      size_t cv = PARG(1);
      for (int i = 0; i < result.num; i++)
      {
        double x = FARGV(0, i);
        size_t offset = i*result.size;
        result.setFloat(cos(x), i);
        memory->store(result.data + offset, cv + offset, result.size);
        result.setFloat(sin(x), i);
      }
    }


    ////////////////////////////
    // Misc. Vector Functions //
    ////////////////////////////

    DEFINE_BUILTIN(shuffle_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        result.setUInt(UARGV(0, UARGV(1, i)), i);
      }
    }

    DEFINE_BUILTIN(shuffle2_builtin)
    {
      for (int i = 0; i < result.num; i++)
      {
        uint64_t m = 1;
        if (ARG(0)->getType()->isVectorTy())
        {
          m = ARG(0)->getType()->getVectorNumElements();
        }

        uint64_t src = 0;
        uint64_t index = UARGV(2, i);
        if (index >= m)
        {
          index -= m;
          src = 1;
        }
        result.setUInt(UARGV(src, index), i);
      }
    }


    //////////////////////////
    // Relational Functions //
    //////////////////////////

    static int64_t _iseq_(double x, double y){ return x == y; }
    static int64_t _isneq_(double x, double y){ return x != y; }
    static int64_t _isgt_(double x, double y){ return isgreater(x, y); }
    static int64_t _isge_(double x, double y){ return isgreaterequal(x, y); }
    static int64_t _islt_(double x, double y){ return isless(x, y); }
    static int64_t _isle_(double x, double y){ return islessequal(x, y); }
    static int64_t _islg_(double x, double y){ return islessgreater(x, y); }
    static int64_t _isfin_(double x){ return isfinite(x); }
    static int64_t _isinf_(double x){ return ::isinf(x); }
    static int64_t _isnan_(double x){ return ::isnan(x); }
    static int64_t _isnorm_(double x){ return isnormal(x); }
    static int64_t _isord_(double x, double y){ return !isunordered(x, y); }
    static int64_t _isuord_(double x, double y){ return isunordered(x, y); }
    static int64_t _signbit_(double x){ return signbit(x); }

    DEFINE_BUILTIN(all)
    {
      int num = 1;
      if (ARG(0)->getType()->isVectorTy())
      {
        num = ARG(0)->getType()->getVectorNumElements();
      }

      for (int i = 0; i < num; i++)
      {
        if (!(SARGV(0, i) & INT64_MIN))
        {
          result.setSInt(0);
          return;
        }
      }
      result.setSInt(1);
    }

    DEFINE_BUILTIN(any)
    {
      int num = 1;
      if (ARG(0)->getType()->isVectorTy())
      {
        num = ARG(0)->getType()->getVectorNumElements();
      }

      for (int i = 0; i < num; i++)
      {
        if (SARGV(0, i) & INT64_MIN)
        {
          result.setSInt(1);
          return;
        }
      }
      result.setSInt(0);
    }

    static uint64_t _ibitselect_(uint64_t a, uint64_t b, uint64_t c)
    {
      return ((a & ~c) | (b & c));
    }

    static double _fbitselect_(double a, double b, double c)
    {
      uint64_t _a = *(uint64_t*)&a;
      uint64_t _b = *(uint64_t*)&b;
      uint64_t _c = *(uint64_t*)&c;
      uint64_t _r = _ibitselect_(_a, _b, _c);
      return *(double*)&_r;
    }

    DEFINE_BUILTIN(bitselect)
    {
      switch (getOverloadArgType(overload))
      {
        case 'f':
        case 'd':
          f3arg(workItem, callInst, fnName, overload, result, _fbitselect_);
          break;
        case 'h':
        case 't':
        case 'j':
        case 'm':
        case 'c':
        case 's':
        case 'i':
        case 'l':
          u3arg(workItem, callInst, fnName, overload, result, _ibitselect_);
          break;
        default:
          FATAL_ERROR("Unsupported argument type: %c",
                      getOverloadArgType(overload));
      }
    }

    DEFINE_BUILTIN(select_builtin)
    {
      char type = getOverloadArgType(overload);
      for (int i = 0; i < result.num; i++)
      {
        int64_t c = SARGV(2, i);
        bool _c = (result.num > 1) ? c & INT64_MIN : c;
        switch (type)
        {
          case 'f':
          case 'd':
            result.setFloat(_c ? FARGV(1, i) : FARGV(0, i), i);
            break;
          case 'h':
          case 't':
          case 'j':
          case 'm':
          case 'c':
          case 's':
          case 'i':
          case 'l':
            result.setSInt(_c ? SARGV(1, i) : SARGV(0, i), i);
            break;
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }


    ///////////////////////////////
    // Synchronization Functions //
    ///////////////////////////////

    DEFINE_BUILTIN(barrier)
    {
      workItem->m_state = WorkItem::BARRIER;
      workItem->m_workGroup->notifyBarrier(workItem, callInst, UARG(0));
    }

    DEFINE_BUILTIN(mem_fence)
    {
      // TODO: Implement?
    }


    //////////////////////////////////////////
    // Vector Data Load and Store Functions //
    //////////////////////////////////////////

    DEFINE_BUILTIN(vload)
    {
      size_t base = PARG(1);
      unsigned int addressSpace = ARG(1)->getType()->getPointerAddressSpace();
      uint64_t offset = UARG(0);

      size_t address = base + offset*result.size*result.num;
      size_t size = result.size*result.num;
      workItem->getMemory(addressSpace)->load(result.data, address, size);
    }

    DEFINE_BUILTIN(vstore)
    {
      const llvm::Value *value = ARG(0);
      size_t size = getTypeSize(value->getType());
      if (isVector3(value))
      {
        // 3-element vectors are same size as 4-element vectors,
        // but vstore address offset shouldn't use this.
        size = (size/4) * 3;
      }

      size_t base = PARG(2);
      unsigned int addressSpace = ARG(2)->getType()->getPointerAddressSpace();
      uint64_t offset = UARG(1);

      size_t address = base + offset*size;
      unsigned char *data = workItem->getOperand(value).data;
      workItem->getMemory(addressSpace)->store(data, address, size);
    }

    DEFINE_BUILTIN(vload_half)
    {
      size_t base = PARG(1);
      unsigned int addressSpace = ARG(1)->getType()->getPointerAddressSpace();
      uint64_t offset = UARG(0);

      size_t address;
      if (fnName.compare(0, 6, "vloada") == 0 && result.num == 3)
      {
        address = base + offset*sizeof(cl_half)*4;
      }
      else
      {
        address = base + offset*sizeof(cl_half)*result.num;
      }
      size_t size = sizeof(cl_half)*result.num;
      uint16_t *halfData = (uint16_t*)workItem->m_pool.alloc(2*result.num);
      workItem->getMemory(addressSpace)->load((unsigned char*)halfData,
                                              address, size);

      // Convert to floats
      for (int i = 0; i < result.num; i++)
      {
        ((float*)result.data)[i] = halfToFloat(halfData[i]);
      }
    }

    DEFINE_BUILTIN(vstore_half)
    {
      const llvm::Value *value = ARG(0);
      size_t size = getTypeSize(value->getType());
      if (isVector3(value))
      {
        // 3-element vectors are same size as 4-element vectors,
        // but vstore address offset shouldn't use this.
        size = (size/4) * 3;
      }


      size_t base = PARG(2);
      unsigned int addressSpace = ARG(2)->getType()->getPointerAddressSpace();
      uint64_t offset = UARG(1);

      // Convert to halfs
      unsigned char *data = workItem->getOperand(value).data;
      size_t num = size / sizeof(float);
      size = num*sizeof(cl_half);
      uint16_t *halfData = (uint16_t*)workItem->m_pool.alloc(2*num);
      HalfRoundMode rmode = Half_RTE; //  The Oclgrind device's round mode
      if (fnName.find("_rtz") != std::string::npos)
        rmode = Half_RTZ;
      else if (fnName.find("_rtn") != std::string::npos)
        rmode = Half_RTN;
      else if (fnName.find("_rtp") != std::string::npos)
        rmode = Half_RTP;

      for (int i = 0; i < num; i++)
      {
        halfData[i] = floatToHalf(((float*)data)[i], rmode);
      }

      size_t address;
      if (fnName.compare(0, 7, "vstorea") == 0 && num == 3)
      {
        address = base + offset*sizeof(cl_half)*4;
      }
      else
      {
        address = base + offset*sizeof(cl_half)*num;
      }

      workItem->getMemory(addressSpace)->store((unsigned char*)halfData,
                                               address, size);
    }


    /////////////////////////
    // Work-Item Functions //
    /////////////////////////

    DEFINE_BUILTIN(get_global_id)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_globalID[dim] : 0;
      result.setUInt(r);
    }

    DEFINE_BUILTIN(get_global_size)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_kernelInvocation->globalSize[dim] : 0;
      result.setUInt(r);
    }

    DEFINE_BUILTIN(get_global_offset)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_kernelInvocation->globalOffset[dim] : 0;
      result.setUInt(r);
    }

    DEFINE_BUILTIN(get_group_id)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_workGroup->getGroupID()[dim] : 0;
      result.setUInt(r);
    }

    DEFINE_BUILTIN(get_local_id)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_localID[dim] : 0;
      result.setUInt(r);
    }

    DEFINE_BUILTIN(get_local_size)
    {
      uint64_t dim = UARG(0);
      size_t r = dim < 3 ? workItem->m_workGroup->getGroupSize()[dim] : 0;
      result.setUInt(r);
    }

    DEFINE_BUILTIN(get_num_groups)
    {
      uint64_t dim = UARG(0);
      size_t r = 0;
      if (dim < 3)
      {
        r = workItem->m_kernelInvocation->numGroups[dim];
      }
      result.setUInt(r);
    }

    DEFINE_BUILTIN(get_work_dim)
    {
      result.setUInt(workItem->m_kernelInvocation->workDim);
    }


    /////////////////////
    // Other Functions //
    /////////////////////

    DEFINE_BUILTIN(convert_float)
    {
      for (int i = 0; i < result.num; i++)
      {
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
            result.setFloat((float)UARGV(0, i), i);
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
            result.setFloat((float)SARGV(0, i), i);
            break;
          case 'f':
          case 'd':
            result.setFloat(FARGV(0, i), i);
            break;
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
      }
    }

    DEFINE_BUILTIN(convert_half)
    {
      float f;
      HalfRoundMode rmode = Half_RTE;
      if (fnName.find("_rtz") != std::string::npos)
        rmode = Half_RTZ;
      else if (fnName.find("_rtn") != std::string::npos)
        rmode = Half_RTN;
      else if (fnName.find("_rtp") != std::string::npos)
        rmode = Half_RTP;
      const char srcType = getOverloadArgType(overload);
      for (int i = 0; i < result.num; i++)
      {
        switch (srcType)
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
            f = (float)UARGV(0, i);
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
            f = (float)SARGV(0, i);
            break;
          case 'd':
          case 'f':
            f = FARGV(0, i);
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }
        result.setUInt(floatToHalf(f, rmode), i);
      }
    }

    static void setConvertRoundingMode(const string& name)
    {
      size_t rpos = name.find("_rt");
      if (rpos != string::npos)
      {
        switch (name[rpos+3])
        {
        case 'e':
          fesetround(FE_TONEAREST);
          break;
        case 'z':
          fesetround(FE_TOWARDZERO);
          break;
        case 'p':
          fesetround(FE_UPWARD);
          break;
        case 'n':
          fesetround(FE_DOWNWARD);
          break;
        default:
          FATAL_ERROR("Unsupported rounding mode: %c", name[rpos=3]);
        }
      }
      else
      {
        fesetround(FE_TOWARDZERO);
      }
    }

    DEFINE_BUILTIN(convert_uint)
    {
      // Check for saturation modifier
      bool sat = fnName.find("_sat") != string::npos;
      uint64_t max = (1UL<<(result.size*8)) - 1;

      // Use rounding mode
      const int origRnd = fegetround();
      setConvertRoundingMode(fnName);

      for (int i = 0; i < result.num; i++)
      {
        uint64_t r;
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
            r = UARGV(0, i);
            if (sat)
            {
              r = _min_(r, max);
            }
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
          {
            int64_t si = SARGV(0, i);
            r = si;
            if (sat)
            {
              if (si < 0)
              {
                r = 0;
              }
              else if (si > max)
              {
                r = max;
              }
            }
            break;
          }
          case 'f':
          case 'd':
            if (sat)
            {
              r = rint(_clamp_(FARGV(0, i), 0.0, (double)max));
            }
            else
            {
              r = rint(FARGV(0, i));
            }
            break;
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }

        result.setUInt(r, i);
      }
    }

    DEFINE_BUILTIN(convert_sint)
    {
      // Check for saturation modifier
      bool sat = fnName.find("_sat") != string::npos;
      int64_t min, max;
      switch (result.size)
      {
      case 1:
        min = INT8_MIN;
        max = INT8_MAX;
        break;
      case 2:
        min = INT16_MIN;
        max = INT16_MAX;
        break;
      case 4:
        min = INT32_MIN;
        max = INT32_MAX;
        break;
      case 8:
        min = INT64_MIN;
        max = INT64_MAX;
        break;
      }

      // Use rounding mode
      const int origRnd = fegetround();
      setConvertRoundingMode(fnName);

      for (int i = 0; i < result.num; i++)
      {
        int64_t r;
        switch (getOverloadArgType(overload))
        {
          case 'h':
          case 't':
          case 'j':
          case 'm':
            r = UARGV(0, i);
            if (sat)
            {
              r = _min_((uint64_t)r, (uint64_t)max);
            }
            break;
          case 'c':
          case 's':
          case 'i':
          case 'l':
            r = SARGV(0, i);
            if (sat)
            {
              r = _clamp_(r, min, max);
            }
            break;
          case 'f':
          case 'd':
            if (sat)
            {
              r = rint(_clamp_(FARGV(0, i), (double)min, (double)max));
            }
            else
            {
              r = rint(FARGV(0, i));
            }
            break;
          default:
            FATAL_ERROR("Unsupported argument type: %c",
                        getOverloadArgType(overload));
        }

        result.setSInt(r, i);
      }
      fesetround(origRnd);
    }

    DEFINE_BUILTIN(printf_builtin)
    {
      size_t formatPtr = workItem->getOperand(ARG(0)).getPointer();
      Memory *memory = workItem->getMemory(AddrSpaceGlobal);

      int arg = 1;
      while (true)
      {
        char c;
        memory->load((unsigned char*)&c, formatPtr++);
        if (c == '\0')
        {
          break;
        }

        if (c == '%')
        {
          string format = "%";
          while (true)
          {
            memory->load((unsigned char*)&c, formatPtr++);
            if (c == '\0')
            {
              cout << format;
              break;
            }

            format += c;
            bool done = false;
            switch (c)
            {
              case 'd':
              case 'i':
                printf(format.c_str(), SARG(arg++));
                done = true;
                break;
              case 'o':
              case 'u':
              case 'x':
              case 'X':
                printf(format.c_str(), UARG(arg++));
                done = true;
                break;
              case 'f':
              case 'F':
              case 'e':
              case 'E':
              case 'g':
              case 'G':
              case 'a':
              case 'A':
                printf(format.c_str(), FARG(arg++));
                done = true;
                break;
              case '%':
                printf("%%");
                done = true;
                break;
            }
            if (done)
            {
              break;
            }
          }
          if (c == '\0')
          {
            break;
          }
        }
        else
        {
          cout << c;
        }
      }
    }


    /////////////////////
    // LLVM Intrinsics //
    /////////////////////

    DEFINE_BUILTIN(llvm_dbg_declare)
    {
      if (!workItem->m_context->getDevice()->isInteractive())
      {
        return;
      }

      const llvm::DbgDeclareInst *dbgInst =
        (const llvm::DbgDeclareInst*)callInst;
      const llvm::Value *addr = dbgInst->getAddress();
      const llvm::MDNode *var = dbgInst->getVariable();
      const llvm::MDString *name = ((const llvm::MDString*)var->getOperand(2));
      workItem->m_variables[name->getString().str()] = addr;
    }

    DEFINE_BUILTIN(llvm_dbg_value)
    {
      if (!workItem->m_context->getDevice()->isInteractive())
      {
        return;
      }

      const llvm::DbgValueInst *dbgInst = (const llvm::DbgValueInst*)callInst;
      const llvm::Value *value = dbgInst->getValue();
      const llvm::MDNode *var = dbgInst->getVariable();
      uint64_t offset = dbgInst->getOffset();

      // TODO: Use offset?
      const llvm::MDString *name = ((const llvm::MDString*)var->getOperand(2));
      workItem->m_variables[name->getString().str()] = value;
    }

    DEFINE_BUILTIN(llvm_lifetime_start)
    {
      // TODO: Implement?
    }

    DEFINE_BUILTIN(llvm_lifetime_end)
    {
      // TODO: Implement?
    }

    DEFINE_BUILTIN(llvm_memcpy)
    {
      const llvm::MemCpyInst *memcpyInst = (const llvm::MemCpyInst*)callInst;
      size_t dest = workItem->getOperand(memcpyInst->getDest()).getPointer();
      size_t src = workItem->getOperand(memcpyInst->getSource()).getPointer();
      size_t size = workItem->getOperand(memcpyInst->getLength()).getUInt();
      unsigned destAddrSpace = memcpyInst->getDestAddressSpace();
      unsigned srcAddrSpace = memcpyInst->getSourceAddressSpace();

      unsigned char *buffer = workItem->m_pool.alloc(size);
      workItem->getMemory(srcAddrSpace)->load(buffer, src, size);
      workItem->getMemory(destAddrSpace)->store(buffer, dest, size);
    }

    DEFINE_BUILTIN(llvm_memset)
    {
      const llvm::MemSetInst *memsetInst = (const llvm::MemSetInst*)callInst;
      size_t dest = workItem->getOperand(memsetInst->getDest()).getPointer();
      size_t size = workItem->getOperand(memsetInst->getLength()).getUInt();
      unsigned addressSpace = memsetInst->getDestAddressSpace();

      unsigned char *buffer = workItem->m_pool.alloc(size);
      unsigned char value = UARG(1);
      memset(buffer, value, size);
      workItem->getMemory(addressSpace)->store(buffer, dest, size);
    }

    DEFINE_BUILTIN(llvm_trap)
    {
      FATAL_ERROR("Encountered trap instruction");
    }

  public:
    static BuiltinFunctionMap initBuiltins();
  };

  // Utility macros for generating builtin function map
#define CAST                                \
  void(*)(WorkItem*, const llvm::CallInst*, \
  const std::string&, const std::string&, TypedValue& result, void*)
#define F1ARG(name) (double(*)(double))name
#define F2ARG(name) (double(*)(double,double))name
#define F3ARG(name) (double(*)(double,double,double))name
#define ADD_BUILTIN(name, func, op)         \
  builtins[name] = BuiltinFunction((CAST)func, (void*)op);
#define ADD_PREFIX_BUILTIN(name, func, op)  \
  workItemPrefixBuiltins.push_back(                 \
    make_pair(name, BuiltinFunction((CAST)func, (void*)op)));

  // Generate builtin function map
  BuiltinFunctionPrefixList workItemPrefixBuiltins;
  BuiltinFunctionMap workItemBuiltins = WorkItemBuiltins::initBuiltins();
  BuiltinFunctionMap WorkItemBuiltins::initBuiltins()
  {
    BuiltinFunctionMap builtins;

    // Async Copy and Prefetch Functions
    ADD_BUILTIN("async_work_group_copy", async_work_group_copy, NULL);
    ADD_BUILTIN("async_work_group_strided_copy", async_work_group_copy, NULL);
    ADD_BUILTIN("wait_group_events", wait_group_events, NULL);
    ADD_BUILTIN("prefetch", prefetch, NULL);

    // Atomic Functions
    ADD_BUILTIN("atom_add", atomic_add, NULL);
    ADD_BUILTIN("atomic_add", atomic_add, NULL);
    ADD_BUILTIN("atom_and", atomic_and, NULL);
    ADD_BUILTIN("atomic_and", atomic_and, NULL);
    ADD_BUILTIN("atom_cmpxchg", atomic_cmpxchg, NULL);
    ADD_BUILTIN("atomic_cmpxchg", atomic_cmpxchg, NULL);
    ADD_BUILTIN("atom_dec", atomic_dec, NULL);
    ADD_BUILTIN("atomic_dec", atomic_dec, NULL);
    ADD_BUILTIN("atom_inc", atomic_inc, NULL);
    ADD_BUILTIN("atomic_inc", atomic_inc, NULL);
    ADD_BUILTIN("atom_max", atomic_max, NULL);
    ADD_BUILTIN("atomic_max", atomic_max, NULL);
    ADD_BUILTIN("atom_min", atomic_min, NULL);
    ADD_BUILTIN("atomic_min", atomic_min, NULL);
    ADD_BUILTIN("atom_or", atomic_or, NULL);
    ADD_BUILTIN("atomic_or", atomic_or, NULL);
    ADD_BUILTIN("atom_sub", atomic_sub, NULL);
    ADD_BUILTIN("atomic_sub", atomic_sub, NULL);
    ADD_BUILTIN("atom_xchg", atomic_xchg, NULL);
    ADD_BUILTIN("atomic_xchg", atomic_xchg, NULL);
    ADD_BUILTIN("atom_xor", atomic_xor, NULL);
    ADD_BUILTIN("atomic_xor", atomic_xor, NULL);

    // Common Functions
    ADD_BUILTIN("clamp", clamp, NULL);
    ADD_BUILTIN("degrees", f1arg, _degrees_);
    ADD_BUILTIN("max", max, NULL);
    ADD_BUILTIN("min", min, NULL);
    ADD_BUILTIN("mix", mix, NULL);
    ADD_BUILTIN("radians", f1arg, _radians_);
    ADD_BUILTIN("sign", f1arg, _sign_);
    ADD_BUILTIN("smoothstep", smoothstep, NULL);
    ADD_BUILTIN("step", step, NULL);

    // Geometric Functions
    ADD_BUILTIN("cross", cross, NULL);
    ADD_BUILTIN("dot", dot, NULL);
    ADD_BUILTIN("distance", distance, NULL);
    ADD_BUILTIN("length", length, NULL);
    ADD_BUILTIN("normalize", normalize, NULL);
    ADD_BUILTIN("fast_distance", distance, NULL);
    ADD_BUILTIN("fast_length", length, NULL);
    ADD_BUILTIN("fast_normalize", normalize, NULL);

    // Image Functions
    ADD_BUILTIN("get_image_array_size", get_image_array_size, NULL);
    ADD_BUILTIN("get_image_channel_data_type",
                get_image_channel_data_type, NULL);
    ADD_BUILTIN("get_image_channel_order", get_image_channel_order, NULL);
    ADD_BUILTIN("get_image_dim", get_image_dim, NULL);
    ADD_BUILTIN("get_image_depth", get_image_depth, NULL);
    ADD_BUILTIN("get_image_height", get_image_height, NULL);
    ADD_BUILTIN("get_image_width", get_image_width, NULL);
    ADD_BUILTIN("read_imagef", read_imagef, NULL);
    ADD_BUILTIN("read_imagei", read_imagei, NULL);
    ADD_BUILTIN("read_imageui", read_imageui, NULL);
    ADD_BUILTIN("write_imagef", write_imagef, NULL);
    ADD_BUILTIN("write_imagei", write_imagei, NULL);
    ADD_BUILTIN("write_imageui", write_imageui, NULL);

    // Integer Functions
    ADD_BUILTIN("abs", abs_builtin, NULL);
    ADD_BUILTIN("abs_diff", abs_diff, NULL);
    ADD_BUILTIN("add_sat", add_sat, NULL);
    ADD_BUILTIN("clz", clz, NULL);
    ADD_BUILTIN("hadd", hadd, NULL);
    ADD_BUILTIN("mad24", u3arg, _mad_);
    ADD_BUILTIN("mad_hi", mad_hi, NULL);
    ADD_BUILTIN("mad_sat", mad_sat, NULL);
    ADD_BUILTIN("mul24", u2arg, _mul_);
    ADD_BUILTIN("mul_hi", mul_hi, NULL);
    ADD_BUILTIN("popcount", u1arg, _popcount_);
    ADD_BUILTIN("rhadd", rhadd, NULL);
    ADD_BUILTIN("rotate", rotate, NULL);
    ADD_BUILTIN("sub_sat", sub_sat, NULL);
    ADD_BUILTIN("upsample", upsample, NULL);

    // Math Functions
    ADD_BUILTIN("acos", f1arg, F1ARG(acos));
    ADD_BUILTIN("acosh", f1arg, F1ARG(acosh));
    ADD_BUILTIN("acospi", f1arg, _acospi_);
    ADD_BUILTIN("asin", f1arg, F1ARG(asin));
    ADD_BUILTIN("asinh", f1arg, F1ARG(asinh));
    ADD_BUILTIN("asinpi", f1arg, _asinpi_);
    ADD_BUILTIN("atan", f1arg, F1ARG(atan));
    ADD_BUILTIN("atan2", f2arg, F2ARG(atan2));
    ADD_BUILTIN("atanh", f1arg, F1ARG(atanh));
    ADD_BUILTIN("atanpi", f1arg, _atanpi_);
    ADD_BUILTIN("atan2pi", f2arg, _atan2pi_);
    ADD_BUILTIN("cbrt", f1arg, F1ARG(cbrt));
    ADD_BUILTIN("ceil", f1arg, F1ARG(ceil));
    ADD_BUILTIN("copysign", f2arg, F2ARG(copysign));
    ADD_BUILTIN("cos", f1arg, F1ARG(cos));
    ADD_BUILTIN("cosh", f1arg, F1ARG(cosh));
    ADD_BUILTIN("cospi", f1arg, _cospi_);
    ADD_BUILTIN("erfc", f1arg, F1ARG(erfc));
    ADD_BUILTIN("erf", f1arg, F1ARG(erf));
    ADD_BUILTIN("exp", f1arg, F1ARG(exp));
    ADD_BUILTIN("exp2", f1arg, F1ARG(exp2));
    ADD_BUILTIN("exp10", f1arg, _exp10_);
    ADD_BUILTIN("expm1", f1arg, F1ARG(expm1));
    ADD_BUILTIN("fabs", f1arg, F1ARG(fabs));
    ADD_BUILTIN("fdim", f2arg, F2ARG(fdim));
    ADD_BUILTIN("floor", f1arg, F1ARG(floor));
    ADD_BUILTIN("fma", f3arg, F3ARG(fma));
    ADD_BUILTIN("fmax", f2arg, F2ARG(fmax));
    ADD_BUILTIN("fmin", f2arg, F2ARG(fmin));
    ADD_BUILTIN("fmod", f2arg, F2ARG(fmod));
    ADD_BUILTIN("fract", fract, NULL);
    ADD_BUILTIN("frexp", frexp_builtin, NULL);
    ADD_BUILTIN("hypot", f2arg, F2ARG(hypot));
    ADD_BUILTIN("ilogb", ilogb_builtin, NULL);
    ADD_BUILTIN("ldexp", ldexp_builtin, NULL);
    ADD_BUILTIN("lgamma", f1arg, F1ARG(lgamma));
    ADD_BUILTIN("lgamma_r", lgamma_r, NULL);
    ADD_BUILTIN("log", f1arg, F1ARG(log));
    ADD_BUILTIN("log2", f1arg, F1ARG(log2));
    ADD_BUILTIN("log10", f1arg, F1ARG(log10));
    ADD_BUILTIN("log1p", f1arg, F1ARG(log1p));
    ADD_BUILTIN("logb", f1arg, F1ARG(logb));
    ADD_BUILTIN("mad", f3arg, F3ARG(fma));
    ADD_BUILTIN("maxmag", f2arg, _maxmag_);
    ADD_BUILTIN("minmag", f2arg, _minmag_);
    ADD_BUILTIN("modf", modf_builtin, NULL);
    ADD_BUILTIN("nan", nan_builtin, NULL);
    ADD_BUILTIN("nextafter", f2arg, F2ARG(nextafter));
    ADD_BUILTIN("pow", f2arg, F2ARG(pow));
    ADD_BUILTIN("pown", pown, NULL);
    ADD_BUILTIN("powr", f2arg, F2ARG(pow));
    ADD_BUILTIN("remainder", f2arg, F2ARG(remainder));
    ADD_BUILTIN("remquo", remquo_builtin, NULL);
    ADD_BUILTIN("rint", f1arg, F1ARG(rint));
    ADD_BUILTIN("rootn", rootn, NULL);
    ADD_BUILTIN("round", f1arg, F1ARG(round));
    ADD_BUILTIN("rsqrt", f1arg, _rsqrt_);
    ADD_BUILTIN("sin", f1arg, F1ARG(sin));
    ADD_BUILTIN("sinh", f1arg, F1ARG(sinh));
    ADD_BUILTIN("sinpi", f1arg, _sinpi_);
    ADD_BUILTIN("sincos", sincos, NULL);
    ADD_BUILTIN("sqrt", f1arg, F1ARG(sqrt));
    ADD_BUILTIN("tan", f1arg, F1ARG(tan));
    ADD_BUILTIN("tanh", f1arg, F1ARG(tanh));
    ADD_BUILTIN("tanpi", f1arg, _tanpi_);
    ADD_BUILTIN("tgamma", f1arg, F1ARG(tgamma));
    ADD_BUILTIN("trunc", f1arg, F1ARG(trunc));

    // Native Math Functions
    ADD_BUILTIN("half_cos", f1arg, F1ARG(cos));
    ADD_BUILTIN("native_cos", f1arg, F1ARG(cos));
    ADD_BUILTIN("half_divide", f2arg, _fdivide_);
    ADD_BUILTIN("native_divide", f2arg, _fdivide_);
    ADD_BUILTIN("half_exp", f1arg, F1ARG(exp));
    ADD_BUILTIN("native_exp", f1arg, F1ARG(exp));
    ADD_BUILTIN("half_exp2", f1arg, F1ARG(exp2));
    ADD_BUILTIN("native_exp2", f1arg, F1ARG(exp2));
    ADD_BUILTIN("half_exp10", f1arg, _exp10_);
    ADD_BUILTIN("native_exp10", f1arg, _exp10_);
    ADD_BUILTIN("half_log", f1arg, F1ARG(log));
    ADD_BUILTIN("native_log", f1arg, F1ARG(log));
    ADD_BUILTIN("half_log2", f1arg, F1ARG(log2));
    ADD_BUILTIN("native_log2", f1arg, F1ARG(log2));
    ADD_BUILTIN("half_log10", f1arg, F1ARG(log10));
    ADD_BUILTIN("native_log10", f1arg, F1ARG(log10));
    ADD_BUILTIN("half_powr", f2arg, F2ARG(pow));
    ADD_BUILTIN("native_powr", f2arg, F2ARG(pow));
    ADD_BUILTIN("half_recip", f1arg, _frecip_);
    ADD_BUILTIN("native_recip", f1arg, _frecip_);
    ADD_BUILTIN("half_rsqrt", f1arg, _rsqrt_);
    ADD_BUILTIN("native_rsqrt", f1arg, _rsqrt_);
    ADD_BUILTIN("half_sin", f1arg, F1ARG(sin));
    ADD_BUILTIN("native_sin", f1arg, F1ARG(sin));
    ADD_BUILTIN("half_sqrt", f1arg, F1ARG(sqrt));
    ADD_BUILTIN("native_sqrt", f1arg, F1ARG(sqrt));
    ADD_BUILTIN("half_tan", f1arg, F1ARG(tan));
    ADD_BUILTIN("native_tan", f1arg, F1ARG(tan));

    // Misc. Vector Functions
    ADD_BUILTIN("shuffle", shuffle_builtin, NULL);
    ADD_BUILTIN("shuffle2", shuffle2_builtin, NULL);

    // Relational Functional
    ADD_BUILTIN("all", all, NULL);
    ADD_BUILTIN("any", any, NULL);
    ADD_BUILTIN("bitselect", bitselect, NULL);
    ADD_BUILTIN("isequal", rel2arg, _iseq_);
    ADD_BUILTIN("isnotequal", rel2arg, _isneq_);
    ADD_BUILTIN("isgreater", rel2arg, _isgt_);
    ADD_BUILTIN("isgreaterequal", rel2arg, _isge_);
    ADD_BUILTIN("isless", rel2arg, _islt_);
    ADD_BUILTIN("islessequal", rel2arg, _isle_);
    ADD_BUILTIN("islessgreater", rel2arg, _islg_);
    ADD_BUILTIN("isfinite", rel1arg, _isfin_);
    ADD_BUILTIN("isinf", rel1arg, _isinf_);
    ADD_BUILTIN("isnan", rel1arg, _isnan_);
    ADD_BUILTIN("isnormal", rel1arg, _isnorm_);
    ADD_BUILTIN("isordered", rel2arg, _isord_);
    ADD_BUILTIN("isunordered", rel2arg, _isuord_);
    ADD_BUILTIN("select", select_builtin, NULL);
    ADD_BUILTIN("signbit", rel1arg, _signbit_);

    // Synchronization Functions
    ADD_BUILTIN("barrier", barrier, NULL);
    ADD_BUILTIN("mem_fence", mem_fence, NULL);
    ADD_BUILTIN("read_mem_fence", mem_fence, NULL);
    ADD_BUILTIN("write_mem_fence", mem_fence, NULL);

    // Vector Data Load and Store Functions
    ADD_PREFIX_BUILTIN("vload_half", vload_half, NULL);
    ADD_PREFIX_BUILTIN("vloada_half", vload_half, NULL);
    ADD_PREFIX_BUILTIN("vstore_half", vstore_half, NULL);
    ADD_PREFIX_BUILTIN("vstorea_half", vstore_half, NULL);
    ADD_PREFIX_BUILTIN("vload", vload, NULL);
    ADD_PREFIX_BUILTIN("vstore", vstore, NULL);

    // Work-Item Functions
    ADD_BUILTIN("get_global_id", get_global_id, NULL);
    ADD_BUILTIN("get_global_size", get_global_size, NULL);
    ADD_BUILTIN("get_global_offset", get_global_offset, NULL);
    ADD_BUILTIN("get_group_id", get_group_id, NULL);
    ADD_BUILTIN("get_local_id", get_local_id, NULL);
    ADD_BUILTIN("get_local_size", get_local_size, NULL);
    ADD_BUILTIN("get_num_groups", get_num_groups, NULL);
    ADD_BUILTIN("get_work_dim", get_work_dim, NULL);

    // Other Functions
    ADD_PREFIX_BUILTIN("convert_half",   convert_half, NULL);
    ADD_PREFIX_BUILTIN("convert_float",  convert_float, NULL);
    ADD_PREFIX_BUILTIN("convert_double", convert_float, NULL);
    ADD_PREFIX_BUILTIN("convert_u",      convert_uint, NULL);
    ADD_PREFIX_BUILTIN("convert_",       convert_sint, NULL);
    ADD_BUILTIN("printf", printf_builtin, NULL);

    // LLVM Intrinsics
    ADD_BUILTIN("llvm.dbg.declare", llvm_dbg_declare, NULL);
    ADD_BUILTIN("llvm.dbg.value", llvm_dbg_value, NULL);
    ADD_BUILTIN("llvm.lifetime.start", llvm_lifetime_start, NULL);
    ADD_BUILTIN("llvm.lifetime.end", llvm_lifetime_end, NULL);
    ADD_PREFIX_BUILTIN("llvm.memcpy", llvm_memcpy, NULL);
    ADD_PREFIX_BUILTIN("llvm.memset", llvm_memset, NULL);
    ADD_PREFIX_BUILTIN("llvm.fmuladd", f3arg, F3ARG(fma));
    ADD_BUILTIN("llvm.trap", llvm_trap, NULL);

    return builtins;
  }
}