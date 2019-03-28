/******************************************************************************
 * Copyright (c) 2016, Chen Fang <mtdcy.chen@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/


#ifndef _TOOLKIT_HEADERS_ALL_H
#define _TOOLKIT_HEADERS_ALL_H

// basic [c & c++]
#include <ABE/basic/Version.h>
#include <ABE/basic/Types.h>

#ifdef LOG_TAG
#include <ABE/basic/Log.h>
#endif

#include <ABE/basic/Hardware.h>
#include <ABE/basic/Time.h>
#include <ABE/basic/SharedObject.h>
#include <ABE/basic/Allocator.h>
#include <ABE/basic/SharedBuffer.h>
#include <ABE/basic/String.h>

// object types [SharedObject] [c & c++]
#include <ABE/object/Buffer.h>
#include <ABE/object/Message.h>
#include <ABE/object/Content.h>
#include <ABE/object/Runnable.h>
#include <ABE/object/Looper.h>
#include <ABE/object/Event.h>

#ifdef __cplusplus  // only available for c++

// tools [non-SharedObject]
#include <ABE/tools/Mutex.h>
#include <ABE/tools/Thread.h>
#include <ABE/tools/Bits.h>

// containers [non-SharedObject]
#include <ABE/stl/TypeHelper.h>
#include <ABE/stl/List.h>
#include <ABE/stl/Vector.h>
#include <ABE/stl/HashTable.h>
#include <ABE/stl/Queue.h>

#endif //  __cplusplus

#endif // _TOOLKIT_HEADERS_ALL_H
