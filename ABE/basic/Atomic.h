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


// File:    Log.h
// Author:  mtdcy.chen
// Changes: 
//          1. 20160701     initial version
//

#ifndef ABE_HEADERS_ATOMIC_H
#define ABE_HEADERS_ATOMIC_H

#include <ABE/basic/Types.h>

// we don't understand the memmodel, always using the same
#ifndef ABE_ATOMIC_MEMMODEL
#define ABE_ATOMIC_MEMMODEL             __ATOMIC_SEQ_CST
#endif

#if defined(__GNUC__) || defined(__clang__)
// The ‘__atomic’ builtins can be used with any integral scalar or pointer type
// that is 1, 2, 4, or 8 bytes in length. 16-byte integral types are also allowed
// if ‘__int128’ (see __int128) is supported by the architecture.
#define ABE_ATOMIC_STORE(p, val)        __atomic_store_n(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_LOAD(p)              __atomic_load_n(p, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_EXCHANGE(p, val)     __atomic_exchange_n(p, val, ABE_ATOMIC_MEMMODEL)
// if *p0 == *p1: val => *p0 : return true; else: *p0 => *p1 : return false
#define ABE_ATOMIC_CAS(p0, p1, val)     __atomic_compare_exchange_n(p0, p1, val, false, ABE_ATOMIC_MEMMODEL, ABE_ATOMIC_MEMMODEL)    // compare and swap

#define ABE_ATOMIC_ADD(p, val)          __atomic_add_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_SUB(p, val)          __atomic_sub_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_AND(p, val)          __atomic_and_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_OR(p, val)           __atomic_or_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_XOR(p, val)          __atomic_xor_fetch(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_NAND(p, val)         __atomic_nand_fetch(p, val, ABE_ATOMIC_MEMMODEL)

#define ABE_ATOMIC_FETCH_ADD(p, val)    __atomic_fetch_add(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_SUB(p, val)    __atomic_fetch_sub(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_AND(p, val)    __atomic_fetch_and(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_OR(p, val)     __atomic_fetch_or(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_XOR(p, val)    __atomic_fetch_xor(p, val, ABE_ATOMIC_MEMMODEL)
#define ABE_ATOMIC_FETCH_NAND(p, val)   __atomic_fetch_nand(p, val, ABE_ATOMIC_MEMMODEL)
#elif _MSC_VER
#error "TODO: ATOMIC is missing for MSVC"
#endif

#ifdef __cplusplus
__BEGIN_NAMESPACE_ABE
template <typename T> class Atomic {
private:
    volatile T value;
    
public:
    ABE_INLINE Atomic()                     { ABE_ATOMIC_STORE(&value, static_cast<T>(0));  }
    ABE_INLINE explicit Atomic(T _value)    { ABE_ATOMIC_STORE(&value, _value);             }
    ABE_INLINE T operator=(T _value)        { return ABE_ATOMIC_STORE(&value, _value);      }
    
    ABE_INLINE void   store(T val)          { ABE_ATOMIC_STORE(&value, val);                }
    ABE_INLINE T      load() const          { return ABE_ATOMIC_LOAD(&value);               }
    ABE_INLINE T      exchange(T val)       { return ABE_ATOMIC_EXCHANGE(&value, val);      }
    ABE_INLINE bool   cas(T& to, T val)     { return ABE_ATOMIC_CAS(&value, &to, val);      }   // compare and swap
    
    ABE_INLINE T      operator++()          { return ABE_ATOMIC_ADD(&value, 1);             }   // pre-increment
    ABE_INLINE T      operator++(int)       { return ABE_ATOMIC_FETCH_ADD(&value, 1);       }   // post_increment
    ABE_INLINE T      operator--()          { return ABE_ATOMIC_SUB(&value, 1);             }
    ABE_INLINE T      operator--(int)       { return ABE_ATOMIC_FETCH_SUB(&value, 1);       }
    
    ABE_INLINE T      operator+=(T val)     { return ABE_ATOMIC_ADD(&value, val);           }
    ABE_INLINE T      operator-=(T val)     { return ABE_ATOMIC_SUB(&value, val);           }
    ABE_INLINE T      operator&=(T val)     { return ABE_ATOMIC_AND(&value, val);           }
    ABE_INLINE T      operator|=(T val)     { return ABE_ATOMIC_OR(&value, val);            }
    ABE_INLINE T      operator^=(T val)     { return ABE_ATOMIC_XOR(&value, val);           }
    
    ABE_INLINE T      fetch_add(T val)      { return ABE_ATOMIC_FETCH_ADD(&value, val);     }
    ABE_INLINE T      fetch_sub(T val)      { return ABE_ATOMIC_FETCH_SUB(&value, val);     }
    ABE_INLINE T      fetch_and(T val)      { return ABE_ATOMIC_FETCH_AND(&value, val);     }
    ABE_INLINE T      fetch_or(T val)       { return ABE_ATOMIC_FETCH_OR(&value, val);      }
    ABE_INLINE T      fetch_xor(T val)      { return ABE_ATOMIC_FETCH_XOR(&value, val);     }
    
    DISALLOW_DYNAMIC(Atomic);
    DISALLOW_EVILS(Atomic);
};
__END_NAMESPACE_ABE
#endif

// fence
#define atomic_fence()          __atomic_thread_fence(ABE_ATOMIC_MEMMODEL)

#endif // ABE_HEADERS_ATOMIC_H 
