/*
 * Copyright (c) 1999, 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef OS_CPU_LINUX_X86_VM_ATOMIC_LINUX_X86_INLINE_HPP
#define OS_CPU_LINUX_X86_VM_ATOMIC_LINUX_X86_INLINE_HPP

#include "runtime/atomic.hpp"
#include "runtime/os.hpp"
#include "vm_version_x86.hpp"

// Implementation of class atomic

inline void Atomic::store    (jbyte    store_value, jbyte*    dest) { *dest = store_value; }
inline void Atomic::store    (jshort   store_value, jshort*   dest) { *dest = store_value; }
inline void Atomic::store    (jint     store_value, jint*     dest) { *dest = store_value; }
inline void Atomic::store_ptr(intptr_t store_value, intptr_t* dest) { *dest = store_value; }
inline void Atomic::store_ptr(void*    store_value, void*     dest) { *(void**)dest = store_value; }

inline void Atomic::store    (jbyte    store_value, volatile jbyte*    dest) { *dest = store_value; }
inline void Atomic::store    (jshort   store_value, volatile jshort*   dest) { *dest = store_value; }
inline void Atomic::store    (jint     store_value, volatile jint*     dest) { *dest = store_value; }
inline void Atomic::store_ptr(intptr_t store_value, volatile intptr_t* dest) { *dest = store_value; }
inline void Atomic::store_ptr(void*    store_value, volatile void*     dest) { *(void* volatile *)dest = store_value; }


// Adding a lock prefix to an instruction on MP machine
#define LOCK_IF_MP(mp) "cmp $0, " #mp "; je 1f; lock; 1: "

inline jint     Atomic::add    (jint     add_value, volatile jint*     dest) {
  jint addend = add_value;
  int mp = os::is_MP();
  __asm__ volatile (  LOCK_IF_MP(%3) "xaddl %0,(%2)"
                    : "=r" (addend)
                    : "0" (addend), "r" (dest), "r" (mp)
                    : "cc", "memory");
  return addend + add_value;
}

inline void Atomic::inc    (volatile jint*     dest) {
  int mp = os::is_MP();
  __asm__ volatile (LOCK_IF_MP(%1) "addl $1,(%0)" :
                    : "r" (dest), "r" (mp) : "cc", "memory");
}

inline void Atomic::inc_ptr(volatile void*     dest) {
  inc_ptr((volatile intptr_t*)dest);
}

inline void Atomic::dec    (volatile jint*     dest) {
  int mp = os::is_MP();
  __asm__ volatile (LOCK_IF_MP(%1) "subl $1,(%0)" :
                    : "r" (dest), "r" (mp) : "cc", "memory");
}

inline void Atomic::dec_ptr(volatile void*     dest) {
  dec_ptr((volatile intptr_t*)dest);
}

/**
出自《64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf》
Any locked instruction (either the XCHG instruction or another read-modify-write instruction with a LOCK prefix) appears to execute as an indivisible and uninterruptible sequence of load(s) followed by store(s) regardless of alignment.
任何被锁定的指令(无论是XCHG指令还是另一条带有LOCK前缀的读-修改-写指令)看起来都是作为一个不可分割和不可中断的加载序列执行的。

XCHG（交换数据）指令交换两个操作数内容
**/
inline jint     Atomic::xchg    (jint     exchange_value, volatile jint*     dest) {
  __asm__ volatile (  "xchgl (%2),%0"
                    : "=r" (exchange_value)
                    : "0" (exchange_value), "r" (dest)
                    : "memory");
  return exchange_value;
}

inline void*    Atomic::xchg_ptr(void*    exchange_value, volatile void*     dest) {
  return (void*)xchg_ptr((intptr_t)exchange_value, (volatile intptr_t*)dest);
}


inline jint     Atomic::cmpxchg    (jint     exchange_value, volatile jint*     dest, jint     compare_value) {
  // 判断当前执行环境是否为多处理器环境
  int mp = os::is_MP();
  // LOCK_IF_MP(%4) 在多处理器环境下，为 cmpxchgl 指令添加 lock 前缀，以达到内存屏障的效果
  // cmpxchgl 指令是包含在 x86 架构及 IA-64 架构中的一个原子条件指令，
  // 它会首先比较 dest 指针指向的内存值是否和 compare_value 的值相等，
  // 如果相等，则双向交换 dest 与 exchange_value，否则就单方面地将 dest 指向的内存值交给exchange_value。
  // 这条指令完成了整个 CAS 操作，因此它也被称为 CAS 指令。
  __asm__ volatile (LOCK_IF_MP(%4) "cmpxchgl %1,(%3)"
                    : "=a" (exchange_value)
                    : "r" (exchange_value), "a" (compare_value), "r" (dest), "r" (mp)
                    : "cc", "memory");
  return exchange_value;
}

#ifdef AMD64
inline void Atomic::store    (jlong    store_value, jlong*    dest) { *dest = store_value; }
inline void Atomic::store    (jlong    store_value, volatile jlong*    dest) { *dest = store_value; }

inline intptr_t Atomic::add_ptr(intptr_t add_value, volatile intptr_t* dest) {
  intptr_t addend = add_value;
  bool mp = os::is_MP();
  __asm__ __volatile__ (LOCK_IF_MP(%3) "xaddq %0,(%2)"
                        : "=r" (addend)
                        : "0" (addend), "r" (dest), "r" (mp)
                        : "cc", "memory");
  return addend + add_value;
}

inline void*    Atomic::add_ptr(intptr_t add_value, volatile void*     dest) {
  return (void*)add_ptr(add_value, (volatile intptr_t*)dest);
}

inline void Atomic::inc_ptr(volatile intptr_t* dest) {
  bool mp = os::is_MP();
  __asm__ __volatile__ (LOCK_IF_MP(%1) "addq $1,(%0)"
                        :
                        : "r" (dest), "r" (mp)
                        : "cc", "memory");
}

inline void Atomic::dec_ptr(volatile intptr_t* dest) {
  bool mp = os::is_MP();
  __asm__ __volatile__ (LOCK_IF_MP(%1) "subq $1,(%0)"
                        :
                        : "r" (dest), "r" (mp)
                        : "cc", "memory");
}

inline intptr_t Atomic::xchg_ptr(intptr_t exchange_value, volatile intptr_t* dest) {
  __asm__ __volatile__ ("xchgq (%2),%0"
                        : "=r" (exchange_value)
                        : "0" (exchange_value), "r" (dest)
                        : "memory");
  return exchange_value;
}

inline jlong    Atomic::cmpxchg    (jlong    exchange_value, volatile jlong*    dest, jlong    compare_value) {
  bool mp = os::is_MP();
  __asm__ __volatile__ (LOCK_IF_MP(%4) "cmpxchgq %1,(%3)"
                        : "=a" (exchange_value)
                        : "r" (exchange_value), "a" (compare_value), "r" (dest), "r" (mp)
                        : "cc", "memory");
  return exchange_value;
}

inline intptr_t Atomic::cmpxchg_ptr(intptr_t exchange_value, volatile intptr_t* dest, intptr_t compare_value) {
  return (intptr_t)cmpxchg((jlong)exchange_value, (volatile jlong*)dest, (jlong)compare_value);
}

inline void*    Atomic::cmpxchg_ptr(void*    exchange_value, volatile void*     dest, void*    compare_value) {
  return (void*)cmpxchg((jlong)exchange_value, (volatile jlong*)dest, (jlong)compare_value);
}

inline jlong Atomic::load(volatile jlong* src) { return *src; }

#else // !AMD64

inline intptr_t Atomic::add_ptr(intptr_t add_value, volatile intptr_t* dest) {
  return (intptr_t)Atomic::add((jint)add_value, (volatile jint*)dest);
}

inline void*    Atomic::add_ptr(intptr_t add_value, volatile void*     dest) {
  return (void*)Atomic::add((jint)add_value, (volatile jint*)dest);
}


inline void Atomic::inc_ptr(volatile intptr_t* dest) {
  inc((volatile jint*)dest);
}

inline void Atomic::dec_ptr(volatile intptr_t* dest) {
  dec((volatile jint*)dest);
}

inline intptr_t Atomic::xchg_ptr(intptr_t exchange_value, volatile intptr_t* dest) {
  return (intptr_t)xchg((jint)exchange_value, (volatile jint*)dest);
}

extern "C" {
  // defined in linux_x86.s
  jlong _Atomic_cmpxchg_long(jlong, volatile jlong*, jlong, bool);
  void _Atomic_move_long(volatile jlong* src, volatile jlong* dst);
}

inline jlong    Atomic::cmpxchg    (jlong    exchange_value, volatile jlong*    dest, jlong    compare_value) {
  return _Atomic_cmpxchg_long(exchange_value, dest, compare_value, os::is_MP());
}

inline intptr_t Atomic::cmpxchg_ptr(intptr_t exchange_value, volatile intptr_t* dest, intptr_t compare_value) {
  return (intptr_t)cmpxchg((jint)exchange_value, (volatile jint*)dest, (jint)compare_value);
}

inline void*    Atomic::cmpxchg_ptr(void*    exchange_value, volatile void*     dest, void*    compare_value) {
  return (void*)cmpxchg((jint)exchange_value, (volatile jint*)dest, (jint)compare_value);
}

inline jlong Atomic::load(volatile jlong* src) {
  volatile jlong dest;
  _Atomic_move_long(src, &dest);
  return dest;
}

inline void Atomic::store(jlong store_value, jlong* dest) {
  _Atomic_move_long((volatile jlong*)&store_value, (volatile jlong*)dest);
}

inline void Atomic::store(jlong store_value, volatile jlong* dest) {
  _Atomic_move_long((volatile jlong*)&store_value, dest);
}

#endif // AMD64

#endif // OS_CPU_LINUX_X86_VM_ATOMIC_LINUX_X86_INLINE_HPP
