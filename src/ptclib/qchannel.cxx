/*
 * qchannel.cxx
 *
 * Class for implementing a serial queue channel in memory.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: qchannel.cxx,v $
 * Revision 1.1  2001/07/10 03:07:07  robertj
 * Added queue channel and delay channel classes to ptclib.
 *
 */

#ifdef __GNUC__
#pragma implementation "qchannel.h"
#endif

#include <ptlib.h>
#include <ptclib/qchannel.h>


#define new PNEW


/////////////////////////////////////////////////////////

PQueueChannel::PQueueChannel(PINDEX size)
{
  if (size > 0) {
    queueBuffer = new BYTE[size];
    os_handle = 1;
  }
  else {
    queueBuffer = NULL;
    os_handle = -1;
  }
  queueSize = size;
  queueLength = enqueuePos = dequeuePos = 0;
}


PQueueChannel::~PQueueChannel()
{
  delete queueBuffer;
}


BOOL PQueueChannel::Open(PINDEX size)
{
  if (size == 0)
    Close();
  else {
    mutex.Wait();
    delete queueBuffer;
    queueBuffer = new BYTE[size];
    queueSize = size;
    queueLength = enqueuePos = dequeuePos = 0;
    os_handle = 1;
    mutex.Signal();

    unempty.Signal();
    unfull.Signal();
  }

  return TRUE;
}


BOOL PQueueChannel::Close()
{
  if (!IsOpen())
    return FALSE;

  mutex.Wait();
  delete queueBuffer;
  queueBuffer = NULL;
  os_handle = -1;
  mutex.Signal();
  unempty.Signal();
  unfull.Signal();
  return TRUE;
}


BOOL PQueueChannel::Read(void * buf, PINDEX count)
{
  lastReadCount = 0;

  if (!IsOpen())
    return FALSE;

  BYTE * buffer = (BYTE *)buf;

  while (count > 0) {

    mutex.Wait();

    /* If queue is empty then we should block for the time specifed in the
       read timeout.
     */
    while (queueLength == 0) {
      mutex.Signal();

      PTRACE_IF(6, readTimeout > 0, "QChan\tBlocking on empty queue");
      if (!unempty.Wait(readTimeout)) {
        PTRACE(6, "QChan\tRead timeout on empty queue");
        lastError = Timeout;
        osError = EAGAIN;
        return lastReadCount > 0;
      }

      if (!IsOpen()) {
        lastError = Interrupted;
        osError = EINTR;
        return FALSE;
      }

      mutex.Wait();
    }

    // To make things simpler, limit to amount to copy out of queue to till
    // the end of the linear part of memory. Another loop around will get
    // rest of data to dequeue
    PINDEX copyLen = queueSize - dequeuePos;

    // But do not copy more than has actually been queued
    if (copyLen > queueLength)
      copyLen = queueLength;

    // Or more than has been requested
    if (copyLen > count)
      copyLen = count;

    // Copy data out and increment pointer, decrement bytes yet to dequeue
    memcpy(buffer, queueBuffer+dequeuePos, copyLen);
    lastReadCount += copyLen;
    buffer += copyLen;
    count -= copyLen;

    // Move the queue pointer along, wrapping to beginning
    dequeuePos += copyLen;
    if (dequeuePos >= queueSize)
      dequeuePos = 0;

    // If buffer was full, signal possibly blocked write of data to queue
    // that it can write to queue now.
    if (queueLength == queueSize) {
      PTRACE(6, "QChan\tSignalling queue no longer full");
      unfull.Signal();
    }

    // Now decrement queue length by the amount we copied
    queueLength -= copyLen;

    mutex.Signal();

  }

  return TRUE;
}


BOOL PQueueChannel::Write(const void * buf, PINDEX count)
{
  lastWriteCount = 0;

  if (!IsOpen())
    return FALSE;

  const BYTE * buffer = (BYTE *)buf;

  while (count > 0) {

    mutex.Wait();

    /* If queue is full then we should block for the time specifed in the
       write timeout.
     */
    while (queueLength == queueSize) {
      mutex.Signal();

      PTRACE_IF(6, writeTimeout > 0, "QChan\tBlocking on full queue");
      if (!unfull.Wait(writeTimeout)) {
        PTRACE(6, "QChan\tWrite timeout on full queue");
        lastError = Timeout;
        osError = EAGAIN;
        return FALSE;
      }

      if (!IsOpen()) {
        lastError = Interrupted;
        osError = EINTR;
        return FALSE;
      }

      mutex.Wait();
    }

    // Calculate number of bytes to copy
    PINDEX copyLen = count;

    // First don't copy more than are availble in queue
    PINDEX bytesLeftInQueue = queueSize - queueLength;
    if (copyLen > bytesLeftInQueue)
      copyLen = bytesLeftInQueue;

    // Then to make things simpler, limit to amount left till the end of the
    // linear part of memory. Another loop around will get rest of data to queue
    PINDEX bytesLeftInUnwrapped = queueSize - enqueuePos;
    if (copyLen > bytesLeftInUnwrapped)
      copyLen = bytesLeftInUnwrapped;

    // Move the data in and increment pointer, decrement bytes yet to queue
    memcpy(queueBuffer + enqueuePos, buffer, copyLen);
    lastWriteCount += copyLen;
    buffer += copyLen;
    count -= copyLen;

    // Move the queue pointer along, wrapping to beginning
    enqueuePos += copyLen;
    if (enqueuePos >= queueSize)
      enqueuePos = 0;

    // If buffer was empty, signal possibly blocked reader of data from queue
    // that it can read from queue now.
    if (queueLength == 0) {
      PTRACE(6, "QChan\tSignalling queue no longer empty");
      unempty.Signal();
    }

    // Now increment queue length by the amount we copied
    queueLength += copyLen;

    mutex.Signal();
  }

  return TRUE;
}


// End of File ///////////////////////////////////////////////////////////////
