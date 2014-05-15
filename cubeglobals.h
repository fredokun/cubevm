
#ifndef CUBE_GLOBALS_H
#define CUBE_GLOBALS_H

/* Some parts grabbed from glib, so the glib license also applies to this
 * file
*/

/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#define FALSE 0
#define TRUE 1

/* Integer types used when bit-level access */
/* access is needed                         */
/* XXX: should autoconfize this             */

typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int int16;
typedef unsigned short int uint16;
typedef unsigned long uint32;
typedef signed long int32;
typedef signed long long int64;
typedef unsigned long long uint64;

typedef uint8 byte;
typedef uint16 word;
typedef uint32 dword;
typedef uint64 qword;

typedef int Bool;

/* Define min and max constants for the fixed size numerical types */
#define MININT8	((int8)  0x80)
#define MAXINT8	((int8)  0x7f)
#define MAXUINT8	((uint8) 0xff)

#define MININT16	((int16)  0x8000)
#define MAXINT16	((int16)  0x7fff)
#define MAXUINT16	((uint16) 0xffff)

#define MININT32	((int32)  0x80000000)
#define MAXINT32	((int32)  0x7fffffff)
#define MAXUINT32	((uint32) 0xffffffff)

#define MININT64	((int64) 0x8000000000000000ll)
#define MAXINT64	(0x7fffffffffffffffll)
#define MAXUINT64	(0xffffffffffffffffllU)

/* Define some mathematical constants that aren't available
 * symbolically in some strict ISO C implementations.
 */
#define MATH_E     2.7182818284590452353602874713526624977572470937000
#define MATH_LN2   0.69314718055994530941723212145817656807550013436026
#define MATH_LN10  2.3025850929940456840179914546843642076011014886288
#define MATH_PI    3.1415926535897932384626433832795028841971693993751
#define MATH_PI_2  1.5707963267948966192313216916397514420985846996876
#define MATH_PI_4  0.78539816339744830961566084581987572104929234984378
#define MATH_SQRT2 1.4142135623730950488016887242096980785696718753769

/* Portable endian checks and conversions
 */
#define BO_LITTLE_ENDIAN  1234 /* XXX: should autoconfize this */
/* #define BO_BIG_ENDIAN    4321 */

/* Basic bit swapping functions
 */
#define UINT16_SWAP_LE_BE_CONSTANT(val)	((uint16) ( \
    (uint16) ((uint16) (val) >> 8) |	\
    (uint16) ((uint16) (val) << 8)))

#define UINT32_SWAP_LE_BE_CONSTANT(val)	((uint32) ( \
    (((uint32) (val) & (uint32) 0x000000ffU) << 24) | \
    (((uint32) (val) & (uint32) 0x0000ff00U) <<  8) | \
    (((uint32) (val) & (uint32) 0x00ff0000U) >>  8) | \
    (((uint32) (val) & (uint32) 0xff000000U) >> 24)))

#define UINT64_SWAP_LE_BE_CONSTANT(val)	((uint64) ( \
      (((uint64) (val) &						\
	(uint64) (0x00000000000000ffllU) << 56) |	\
      (((uint64) (val) &						\
	(uint64) (0x000000000000ff00llU)) << 40) |	\
      (((uint64) (val) &						\
	(uint64) (0x0000000000ff0000llU)) << 24) |	\
      (((uint64) (val) &						\
	(uint64) (0x00000000ff000000llU)) <<  8) |	\
      (((uint64) (val) &						\
	(uint64) (0x000000ff00000000llU)) >>  8) |	\
      (((uint64) (val) &						\
	(uint64) (0x0000ff0000000000llU)) >> 24) |	\
      (((uint64) (val) &						\
	(uint64) (0x00ff000000000000llU)) >> 40) |	\
      (((uint64) (val) &						\
	(uint64) (0xff00000000000000llU)) >> 56)))

/* Arch specific stuff for speed
 */
#if defined (__GNUC__) && (__GNUC__ >= 2) && defined (__OPTIMIZE__)
#  if defined (__i386__)
#    define UINT16_SWAP_LE_BE_IA32(val) \
       (__extension__						\
	({ register uint16 __v, __x = ((uint16) (val));	\
	   if (__builtin_constant_p (__x))			\
	     __v = UINT16_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ ("rorw $8, %w0"				\
		      : "=r" (__v)				\
		      : "0" (__x)				\
		      : "cc");					\
	    __v; }))
#    if !defined (__i486__) && !defined (__i586__) \
	&& !defined (__pentium__) && !defined (__i686__) \
	&& !defined (__pentiumpro__) && !defined (__pentium4__)
#       define UINT32_SWAP_LE_BE_IA32(val) \
	  (__extension__					\
	   ({ register uint32 __v, __x = ((uint32) (val));	\
	      if (__builtin_constant_p (__x))			\
		__v = UINT32_SWAP_LE_BE_CONSTANT (__x);	\
	      else						\
		__asm__ ("rorw $8, %w0\n\t"			\
			 "rorl $16, %0\n\t"			\
			 "rorw $8, %w0"				\
			 : "=r" (__v)				\
			 : "0" (__x)				\
			 : "cc");				\
	      __v; }))
#    else /* 486 and higher has bswap */
#       define UINT32_SWAP_LE_BE_IA32(val) \
	  (__extension__					\
	   ({ register uint32 __v, __x = ((uint32) (val));	\
	      if (__builtin_constant_p (__x))			\
		__v = UINT32_SWAP_LE_BE_CONSTANT (__x);	\
	      else						\
		__asm__ ("bswap %0"				\
			 : "=r" (__v)				\
			 : "0" (__x));				\
	      __v; }))
#    endif /* processor specific 32-bit stuff */
#    define GUINT64_SWAP_LE_BE_IA32(val) \
       (__extension__							\
	({ union { uint64 __ll;					\
		   uint32 __l[2]; } __w, __r;				\
	   __w.__ll = ((uint64) (val));				\
	   if (__builtin_constant_p (__w.__ll))				\
	     __r.__ll = UINT64_SWAP_LE_BE_CONSTANT (__w.__ll);		\
	   else								\
	     {								\
	       __r.__l[0] = UINT32_SWAP_LE_BE (__w.__l[1]);		\
	       __r.__l[1] = UINT32_SWAP_LE_BE (__w.__l[0]);		\
	     }								\
	   __r.__ll; }))
     /* Possibly just use the constant version and let gcc figure it out? */
#    define UINT16_SWAP_LE_BE(val) (UINT16_SWAP_LE_BE_IA32 (val))
#    define UINT32_SWAP_LE_BE(val) (UINT32_SWAP_LE_BE_IA32 (val))
#    define UINT64_SWAP_LE_BE(val) (UINT64_SWAP_LE_BE_IA32 (val))
#  elif defined (__ia64__)
#    define UINT16_SWAP_LE_BE_IA64(val) \
       (__extension__						\
	({ register uint16 __v, __x = ((uint16) (val));	\
	   if (__builtin_constant_p (__x))			\
	     __v = UINT16_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ __volatile__ ("shl %0 = %1, 48 ;;"		\
				   "mux1 %0 = %0, @rev ;;"	\
				    : "=r" (__v)		\
				    : "r" (__x));		\
	    __v; }))
#    define UINT32_SWAP_LE_BE_IA64(val) \
       (__extension__						\
	 ({ register uint32 __v, __x = ((uint32) (val));	\
	    if (__builtin_constant_p (__x))			\
	      __v = UINT32_SWAP_LE_BE_CONSTANT (__x);		\
	    else						\
	     __asm__ __volatile__ ("shl %0 = %1, 32 ;;"		\
				   "mux1 %0 = %0, @rev ;;"	\
				    : "=r" (__v)		\
				    : "r" (__x));		\
	    __v; }))
#    define UINT64_SWAP_LE_BE_IA64(val) \
       (__extension__						\
	({ register uint64 __v, __x = ((uint64) (val));	\
	   if (__builtin_constant_p (__x))			\
	     __v = UINT64_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ __volatile__ ("mux1 %0 = %1, @rev ;;"	\
				   : "=r" (__v)			\
				   : "r" (__x));		\
	   __v; }))
#    define UINT16_SWAP_LE_BE(val) (UINT16_SWAP_LE_BE_IA64 (val))
#    define UINT32_SWAP_LE_BE(val) (UINT32_SWAP_LE_BE_IA64 (val))
#    define UINT64_SWAP_LE_BE(val) (UINT64_SWAP_LE_BE_IA64 (val))
#  elif defined (__x86_64__)
#    define UINT32_SWAP_LE_BE_X86_64(val) \
       (__extension__						\
	 ({ register uint32 __v, __x = ((uint32) (val));	\
	    if (__builtin_constant_p (__x))			\
	      __v = UINT32_SWAP_LE_BE_CONSTANT (__x);		\
	    else						\
	     __asm__ ("bswapl %0"				\
		      : "=r" (__v)				\
		      : "0" (__x));				\
	    __v; }))
#    define UINT64_SWAP_LE_BE_X86_64(val) \
       (__extension__						\
	({ register uint64 __v, __x = ((uint64) (val));	\
	   if (__builtin_constant_p (__x))			\
	     __v = UINT64_SWAP_LE_BE_CONSTANT (__x);		\
	   else							\
	     __asm__ ("bswapq %0"				\
		      : "=r" (__v)				\
		      : "0" (__x));				\
	   __v; }))
     /* gcc seems to figure out optimal code for this on its own */
#    define UINT16_SWAP_LE_BE(val) (UINT16_SWAP_LE_BE_CONSTANT (val))
#    define UINT32_SWAP_LE_BE(val) (UINT32_SWAP_LE_BE_X86_64 (val))
#    define UINT64_SWAP_LE_BE(val) (UINT64_SWAP_LE_BE_X86_64 (val))
#  else /* generic gcc */
#    define UINT16_SWAP_LE_BE(val) (UINT16_SWAP_LE_BE_CONSTANT (val))
#    define UINT32_SWAP_LE_BE(val) (UINT32_SWAP_LE_BE_CONSTANT (val))
#    define UINT64_SWAP_LE_BE(val) (UINT64_SWAP_LE_BE_CONSTANT (val))
#  endif
#else /* generic */
#  define UINT16_SWAP_LE_BE(val) (UINT16_SWAP_LE_BE_CONSTANT (val))
#  define UINT32_SWAP_LE_BE(val) (UINT32_SWAP_LE_BE_CONSTANT (val))
#  define UINT64_SWAP_LE_BE(val) (UINT64_SWAP_LE_BE_CONSTANT (val))
#endif /* generic */

/* The G*_TO_?E() macros are defined in glibconfig.h.
 * The transformation is symmetric, so the FROM just maps to the TO.
 */

#ifdef BO_LITTLE_ENDIAN
#define INT16_TO_LE(val) (val)
#define INT16_TO_BE(val) (UINT16_SWAP_LE_BE(val))
#define UINT16_TO_LE(val) (val)
#define UINT16_TO_BE(val) (UINT16_SWAP_LE_BE(val))
#define INT32_TO_LE(val) (val)
#define INT32_TO_BE(val) (UINT32_SWAP_LE_BE(val))
#define UINT32_TO_LE(val) (val)
#define UINT32_TO_BE(val) (UINT32_SWAP_LE_BE(val))
#define INT64_TO_LE(val) (val)
#define INT64_TO_BE(val) (UINT64_SWAP_LE_BE(val))
#define UINT64_TO_LE(val) (val)
#define UINT64_TO_BE(val) (UINT64_SWAP_LE_BE(val))
#else 
#define INT16_TO_BE(val) (val)
#define INT16_TO_LE(val) (UINT16_SWAP_LE_BE(val))
#define UINT16_TO_BE(val) (val)
#define UINT16_TO_LE(val) (UINT16_SWAP_LE_BE(val))
#define INT32_TO_BE(val) (val)
#define INT32_TO_LE(val) (UINT32_SWAP_LE_BE(val))
#define UINT32_TO_BE(val) (val)
#define UINT32_TO_LE(val) (UINT32_SWAP_LE_BE(val))
#define INT64_TO_BE(val) (val)
#define INT64_TO_LE(val) (UINT64_SWAP_LE_BE(val))
#define UINT64_TO_BE(val) (val)
#define UINT64_TO_LE(val) (UINT64_SWAP_LE_BE(val))
#endif


#endif
