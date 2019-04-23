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


/**
 * File:    Matrix.h
 * Author:  mtdcy.chen
 * Changes:
 *          1. 20181214     initial version
 *
 */

#ifndef _ABE_MATH_MATRIX_H
#define _ABE_MATH_MATRIX_H

#include <ABE/basic/Types.h>
#include <ABE/basic/SharedObject.h>

__BEGIN_NAMESPACE_ABE

/**
 * N x N matrix
 * https://en.wikipedia.org/wiki/Matrix_(mathematics)
 */
template <typename TYPE, size_t N> struct Matrix : public NonSharedObject {
    /**
     * 1 x N row vector, tranpose as operator need
     */
    struct Vector {
        TYPE mEntries[N];
        Vector() { for (size_t i = 0; i < N; ++i) mEntries[i] = 0; }
        Vector(TYPE v) { for (size_t i = 0; i < N; ++i) mEntries[i] = v; }
        Vector(const TYPE v[N]) { for (size_t i = 0; i < N; ++i) mEntries[i] = v[i]; }
        
        Vector& operator=(TYPE v) { for (size_t i = 0; i < N; ++i) mEntries[i] = v; return *this; }
        Vector& operator=(const TYPE v[N]) { for (size_t i = 0; i < N; ++i) mEntries[i] = v[i]; return *this; }
        
        __ABE_INLINE TYPE& operator[](size_t i) { return mEntries[i]; }
        __ABE_INLINE const TYPE& operator[](size_t i) const { return mEntries[i]; }
        
        // per-entry calculation
        __ABE_INLINE Vector operator+(TYPE rhs) { Vector result; for (size_t i = 0; i < N; ++i) result[i] = mEntries[i] + rhs; return result; }
        __ABE_INLINE Vector operator*(TYPE rhs) { Vector result; for (size_t i = 0; i < N; ++i) result[i] = mEntries[i] * rhs; return result; }
        __ABE_INLINE Vector operator+(const Vector& rhs) { Vector result; for (size_t i = 0; i < N; ++i) result[i] = mEntries[i] + rhs[i]; return result; }
        __ABE_INLINE Vector operator*(const Vector& rhs) { Vector result; for (size_t i = 0; i < N; ++i) result[i] = mEntries[i] * rhs[i]; return result; }
        
        // matrix multiplication
        __ABE_INLINE Vector operator*(const Matrix& rhs) {
            Vector result;
            for (size_t i = 0; i < N; ++i) {
                TYPE sum = 0;
                for (size_t j = 0; j < N; ++j) {
                    sum += mEntries[j] * rhs[j][i];
                }
                result[i] = sum;
            }
            return result;
        }
        
        // A * B.transpose()
        __ABE_INLINE TYPE mulT(const Vector& rhs) {
            TYPE sum = 0;
            for (size_t i = 0; i < N; ++i) {
                sum += mEntries[i] * rhs[i];
            }
            return sum;
        }
    };
    
    Vector mEntries[N];     // rows
    Matrix() { for (size_t i = 0; i < N; ++i) mEntries[i][i] = 1; }
    Matrix(const Vector v[N]) { for (size_t i = 0; i < N; ++i) mEntries[i] = v[i]; }
    Matrix(const TYPE v[N*N]) { for (size_t i = 0; i < N; ++i) for (size_t j = 0; j < N; ++j) mEntries[i][j] = v[i * N + j]; }
    
    Matrix& operator=(const Vector v[N]) { for (size_t i = 0; i < N; ++i) mEntries[i] = v[i]; return *this; }
    Matrix& operator=(const TYPE v[N*N]) { for (size_t i = 0; i < N; ++i) for (size_t j = 0; j < N; ++j) mEntries[i][j] = v[i * N + j]; return *this; }
    
    __ABE_INLINE Vector& operator[](size_t i) { return mEntries[i]; }
    __ABE_INLINE const Vector& operator[](size_t i) const { return mEntries[i]; }
    
    __ABE_INLINE Matrix& transpose() {
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = i; j < N; ++j) {
                TYPE tmp = mEntries[i][j]; mEntries[i][j] = mEntries[j][i]; mEntries[j][i] = tmp;
            }
        }
        return *this;
    }
    
    // per-entry calculation
    __ABE_INLINE Matrix operator+(TYPE rhs) { Matrix result; for (size_t i = 0; i < N; ++i) result[i] = mEntries[i] + rhs; return result; }
    __ABE_INLINE Matrix operator*(TYPE rhs) { Matrix result; for (size_t i = 0; i < N; ++i) result[i] = mEntries[i] * rhs; return result; }
    __ABE_INLINE Matrix operator+(const Matrix& rhs) { Matrix result; for (size_t i = 0; i < N; ++i) result[i] = mEntries[i] + rhs[i]; return result; }
    
    // matrix multiplication
    __ABE_INLINE Vector operator*(const Vector& rhs) {
        Vector result;
        for (size_t i = 0; i < N; ++i) {
            TYPE sum = 0;
            for (size_t j = 0; j < N; ++j) {
                sum += mEntries[i][j] * rhs[j];
            }
            result[i] = sum;
        }
        return result;
    }
    
    __ABE_INLINE Matrix operator*(const Matrix& rhs) {
        Matrix result;
        for (size_t i = 0; i < N; ++i) {        // row
            for (size_t j = 0; j < N; ++j) {    // column
                TYPE sum = 0;
                for (size_t k = 0; k < N; ++k) {
                    sum += mEntries[i][k] * rhs[k][j];
                }
                result[i][j] = 0;
            }
        }
        return result;
    }
    
    // A * B.transpose()
    __ABE_INLINE Matrix mulT(const Matrix& rhs) {
        Matrix result;
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                result[i][j] = mEntries[i].mulT(rhs[j]);
            }
        }
        return result;
    }
};

__END_NAMESPACE_ABE

#endif // _ABE_MATH_MATRIX_H
