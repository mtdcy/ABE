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


// File:    Traits.h
// Author:  mtdcy.chen
// Changes:
//          1. 20160829     initial version
//

#ifndef _TOOLKIT_STL_TRAITS_H
#define _TOOLKIT_STL_TRAITS_H

#include <ABE/basic/Types.h>

__BEGIN_NAMESPACE_ABE

//////////////////////////////////////////////////////////////////////////////
template <typename TYPE> struct is_pointer          { enum { value = false }; };
template <typename TYPE> struct is_pointer<TYPE *>  { enum { value = true  }; };

template <typename TYPE> struct is_builtin          { enum { value = is_pointer<TYPE>::value }; };
template <> struct is_builtin< void     >           { enum { value = true  }; };
template <> struct is_builtin< bool     >           { enum { value = true  }; };
template <> struct is_builtin< char     >           { enum { value = true  }; };
template <> struct is_builtin< int8_t   >           { enum { value = true  }; };
template <> struct is_builtin< uint8_t  >           { enum { value = true  }; };
template <> struct is_builtin< int16_t  >           { enum { value = true  }; };
template <> struct is_builtin< uint16_t >           { enum { value = true  }; };
template <> struct is_builtin< int32_t  >           { enum { value = true  }; };
template <> struct is_builtin< uint32_t >           { enum { value = true  }; };
template <> struct is_builtin< int64_t  >           { enum { value = true  }; };
template <> struct is_builtin< uint64_t >           { enum { value = true  }; };
template <> struct is_builtin< float    >           { enum { value = true  }; };
template <> struct is_builtin< double   >           { enum { value = true  }; };
#if !defined(__GLIBC__)
template <> struct is_builtin< size_t   >           { enum { value = true  }; };
template <> struct is_builtin< ssize_t  >           { enum { value = true  }; };
#endif

template <typename TYPE> struct is_trivial_ctor     { enum { value = is_builtin<TYPE>::value }; };
template <typename TYPE> struct is_trivial_dtor     { enum { value = is_builtin<TYPE>::value }; };
template <typename TYPE> struct is_trivial_copy     { enum { value = is_builtin<TYPE>::value }; };
template <typename TYPE> struct is_trivial_move     { enum { value = is_builtin<TYPE>::value }; };

#ifdef _TOOLKIT_HEADERS_SHARED_BUFFER_H
template <typename T> struct is_trivial_move<sp<T> >    { enum { value = true }; };
#endif

#ifdef _TOOLKIT_HEADERS_STRING_H    // String.h
template <> struct is_trivial_move<String>          { enum { value = true }; };
#endif

__END_NAMESPACE_ABE

#endif // _TOOLKIT_STL_TRAITS_H
