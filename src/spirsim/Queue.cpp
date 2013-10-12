// Queue.cpp (oclgrind)
// Copyright (C) 2013 James Price
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#include "common.h"
#include <cassert>
#include <sys/time.h>

#include <CL/cl.h>
#include "Device.h"
#include "Memory.h"
#include "Queue.h"

using namespace spirsim;
using namespace std;

static double now();

Queue::Queue(Device& device)
  : m_device(device)
{
}

Queue::~Queue()
{
}

Event::Event()
{
  state = CL_QUEUED;
  queueTime = now();
  startTime = endTime = 0;
}

Event* Queue::enqueue(Command *cmd)
{
  Event *event = new Event();
  cmd->event = event;
  m_queue.push(cmd);
  return event;
}

void Queue::executeCopyBuffer(CopyCommand *cmd)
{
  m_device.getGlobalMemory()->copy(cmd->dst, cmd->src, cmd->size);
}

void Queue::executeCopyBufferRect(CopyRectCommand *cmd)
{
  // Perform copy
  Memory *memory = m_device.getGlobalMemory();
  for (int z = 0; z < cmd->region[2]; z++)
  {
    for (int y = 0; y < cmd->region[1]; y++)
    {
      // Compute addresses
      size_t src =
        cmd->src +
        cmd->src_offset[0] +
        y * cmd->src_offset[1] +
        z * cmd->src_offset[2];
      size_t dst =
        cmd->dst +
        cmd->dst_offset[0] +
        y * cmd->dst_offset[1] +
        z * cmd->dst_offset[2];

      // Copy data
      memory->copy(dst, src, cmd->region[0]);
    }
  }
}

void Queue::executeFillBuffer(FillCommand *cmd)
{
  Memory *memory = m_device.getGlobalMemory();
  for (int i = 0; i < cmd->size/cmd->pattern_size; i++)
  {
    memory->store(cmd->pattern,
                  cmd->address + i*cmd->pattern_size,
                  cmd->pattern_size);
  }
}

void Queue::executeKernel(KernelCommand *cmd)
{
  // Run kernel
  m_device.run(*cmd->kernel,
               cmd->work_dim,
               cmd->global_offset,
               cmd->global_size,
               cmd->local_size);
}

void Queue::executeReadBuffer(BufferCommand *cmd)
{
  m_device.getGlobalMemory()->load(cmd->ptr, cmd->address, cmd->size);
}

void Queue::executeReadBufferRect(BufferRectCommand *cmd)
{
  Memory *memory = m_device.getGlobalMemory();
  for (int z = 0; z < cmd->region[2]; z++)
  {
    for (int y = 0; y < cmd->region[1]; y++)
    {
      unsigned char *host =
        cmd->ptr +
        cmd->host_offset[0] +
        y * cmd->host_offset[1] +
        z * cmd->host_offset[2];
      size_t buff =
        cmd->address +
        cmd->buffer_offset[0] +
        y * cmd->buffer_offset[1] +
        z * cmd->buffer_offset[2];
      memory->load(host, buff, cmd->region[0]);
    }
  }
}

void Queue::executeWriteBuffer(BufferCommand *cmd)
{
  m_device.getGlobalMemory()->store(cmd->ptr, cmd->address, cmd->size);
}

void Queue::executeWriteBufferRect(BufferRectCommand *cmd)
{
  // Perform write
  Memory *memory = m_device.getGlobalMemory();
  for (int z = 0; z < cmd->region[2]; z++)
  {
    for (int y = 0; y < cmd->region[1]; y++)
    {
      const unsigned char *host =
        cmd->ptr +
        cmd->host_offset[0] +
        y * cmd->host_offset[1] +
        z * cmd->host_offset[2];
      size_t buff =
        cmd->address +
        cmd->buffer_offset[0] +
        y * cmd->buffer_offset[1] +
        z * cmd->buffer_offset[2];
      memory->store(host, buff, cmd->region[0]);
    }
  }
}

bool Queue::isEmpty() const
{
  return m_queue.empty();
}

double now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_usec*1e3 + tv.tv_sec*1e9;
}

Queue::Command* Queue::update()
{
  if (m_queue.empty())
  {
    return NULL;
  }

  // Get next command
  Command *cmd = m_queue.front();

  // Check if all events in wait list have completed
  while (!cmd->waitList.empty())
  {
    if (cmd->waitList.front()->state == CL_COMPLETE)
    {
      cmd->waitList.pop_front();
    }
    else if (cmd->waitList.front()->state < 0)
    {
      cmd->event->state = cmd->waitList.front()->state;
      m_queue.pop();
      return cmd;
    }
    else
    {
      return NULL;
    }
  }

  cmd->event->startTime = now();
  cmd->event->state = CL_RUNNING;

  // Dispatch command
  switch (cmd->type)
  {
  case COPY:
    executeCopyBuffer((CopyCommand*)cmd);
    break;
  case COPY_RECT:
    executeCopyBufferRect((CopyRectCommand*)cmd);
    break;
  case EMPTY:
    break;
  case FILL:
    executeFillBuffer((FillCommand*)cmd);
    break;
  case READ:
    executeReadBuffer((BufferCommand*)cmd);
    break;
  case READ_RECT:
    executeReadBufferRect((BufferRectCommand*)cmd);
    break;
  case KERNEL:
    executeKernel((KernelCommand*)cmd);
    break;
  case WRITE:
    executeWriteBuffer((BufferCommand*)cmd);
    break;
  case WRITE_RECT:
    executeWriteBufferRect((BufferRectCommand*)cmd);
    break;
  default:
    assert(false && "Unhandled command type in queue.");
  }

  cmd->event->endTime = now();
  cmd->event->state = CL_COMPLETE;

  // Remove command from queue and delete
  m_queue.pop();
  return cmd;
}