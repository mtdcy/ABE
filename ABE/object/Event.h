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


// File:    Event.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef _TOOLKIT_HEADERS_EVENT_H
#define _TOOLKIT_HEADERS_EVENT_H 

#ifdef __cplusplus
#include <ABE/object/Looper.h>
#include <ABE/stl/Queue.h>

__BEGIN_NAMESPACE_ABE

/**
 * a simple async/sync event handle class.
 * if looper exists, it will run async by runnable.
 * otherwise it run directly and blocked.
 */
class Event : public SharedObject {
    public:
        /**
         * construct a event
         * @param looper    if provided, event will run async
         */
        Event();
        Event(const sp<Looper>& looper);
        Event(const String& name, const sp<Looper>& looper);

        /**
         * FIXME: NOT thread safe
         */
        virtual ~Event();

    public:
        /**
         * triggle the event
         * @param delay delay time in us
         * @note if looper no exists, delay will be ignored
         * @note thread safe
         */
        virtual void fire(int64_t delay = 0);

    protected:
        /**
         * handle this event after it been triggled.
         * @note thread safe
         */
        virtual void onEvent() = 0;

    protected:
        SharedObject *  mShared;

    private:
        struct EventRunnable;

    private:
        DISALLOW_EVILS(Event);
};

template <class TYPE> class TypedEvent : public Event {
    public:
        __ALWAYS_INLINE TypedEvent() : Event() { }
        __ALWAYS_INLINE TypedEvent(const sp<Looper>& looper) : Event(looper) { }
        __ALWAYS_INLINE TypedEvent(const String& name, const sp<Looper>& looper) : Event(name, looper) { }

        __ALWAYS_INLINE virtual ~TypedEvent() { }

        __ALWAYS_INLINE void fire(const TYPE& v, int64_t delay = 0) { mQueue.push(v); Event::fire(); }

    protected:
        virtual void fire() { Event::fire(); }
        virtual void onEvent(const TYPE& v) = 0;

    private:
        virtual void onEvent() { TYPE value; if (mQueue.pop(value)) onEvent(value); }
        // TypedEvent is multi producer & single consumer
        LockFree::Queue<TYPE>   mQueue;

    private:
        DISALLOW_EVILS(TypedEvent);
};
__END_NAMESPACE_ABE

#endif // __cplusplus 

#endif // _TOOLKIT_HEADERS_EVENT_H
