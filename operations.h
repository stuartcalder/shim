/* Copyright (c) 2020 Stuart Steven Calder
 * See accompanying LICENSE file for licensing information.
 */
#ifndef SHIM_OPERATIONS_H
#define SHIM_OPERATIONS_H

#include "errors.h"
#include "macros.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if    defined (SHIM_OS_UNIXLIKE)
#	include <unistd.h>
#	if    defined (__OpenBSD__)
#		include <endian.h>
#	elif  defined (__FreeBSD__) || defined (__Dragonfly__)
#		include <sys/endian.h>
#	elif  defined (__NetBSD__)
#		include <machine/bswap.h>
#		include <sys/types.h>
#		include <string.h>
#		include <sys/param.h>
#		if    (__NetBSD_Version__ < 1000000000)
#			include "files.h"
#		endif /* ~ if __NetBSD_Version__ < 1000000000 */
#	elif  defined (__gnu_linux__)
#		include <byteswap.h>
#		include <sys/random.h>
#	elif  defined (SHIM_OS_MAC)
#		define SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS
#		if   !defined (__STDC_WANT_LIB_EXT1__) || (__STDC_WANT_LIB_EXT1__ != 1)
#			error "The macro __STDC_WANT_LIB_EXT1__ must be #defined to 1 for access to memset_s."
#		endif
#		include "files.h"
#		include <string.h>
#	else
#		error "Unsupported unix-like OS."
#	endif /* ~ if defined (unixlike_os's) ... */
#elif  defined (SHIM_OS_WINDOWS)
#	include <windows.h>
#	include <ntstatus.h>
#	include <bcrypt.h>
#	include <stdlib.h>
#else
#	error "Unsupported operating system."
#endif

#define SHIM_ROT_IMPL_UNSIGNED_MASK_TYPE(bits) \
	uint##bits##_t
#define SHIM_ROT_IMPL_UNSIGNED_MASK(bits) \
	((SHIM_ROT_IMPL_UNSIGNED_MASK_TYPE(bits))(sizeof(SHIM_ROT_IMPL_UNSIGNED_MASK_TYPE(bits)) * CHAR_BIT) - 1)
#define SHIM_ROT_IMPL_MASKED_COUNT(bits, count) \
	((SHIM_ROT_IMPL_UNSIGNED_MASK(bits)) & count)

#define SHIM_ROT_LEFT(value, count, bits) \
	((value << SHIM_ROT_IMPL_MASKED_COUNT(bits, count)) | \
		(value >> \
			((-SHIM_ROT_IMPL_MASKED_COUNT(bits, count)) & SHIM_ROT_IMPL_UNSIGNED_MASK(bits)) \
		) \
	)
#define SHIM_ROT_RIGHT(value, count, bits) \
	((value >> SHIM_ROT_IMPL_MASKED_COUNT(bits, count)) | \
		(value << \
			((-SHIM_ROT_IMPL_MASKED_COUNT(bits, count)) & SHIM_ROT_IMPL_UNSIGNED_MASK(bits)) \
		) \
	)

#define SHIM_BIT_CAST_OP(ptr, type_t, tmp_name, statement) \
	do { \
		type_t tmp_name; \
		memcpy( &tmp_name, ptr, sizeof(tmp_name) ); \
		statement; \
		memcpy( ptr, &tmp_name, sizeof(tmp_name) ); \
	} while( 0 )

SHIM_BEGIN_DECLS

SHIM_API void
shim_xor_16 (void * SHIM_RESTRICT, void const * SHIM_RESTRICT);

SHIM_API void 
shim_xor_32 (void * SHIM_RESTRICT, void const * SHIM_RESTRICT);

SHIM_API void 
shim_xor_64 (void * SHIM_RESTRICT, void const * SHIM_RESTRICT);

SHIM_API void 
shim_xor_128 (void * SHIM_RESTRICT, void const * SHIM_RESTRICT);

SHIM_API void *
shim_enforce_malloc (size_t bytes);

SHIM_API void *
shim_enforce_calloc (size_t num_elements, size_t element_size);

SHIM_API void *
shim_enforce_realloc (void * SHIM_RESTRICT ptr, size_t size);

#ifdef SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY
#	define API_ static inline
#else
#	define API_ SHIM_API
#endif

API_ void
shim_obtain_os_entropy (uint8_t * SHIM_RESTRICT, size_t);

#undef API_

static inline void
shim_secure_zero (void * SHIM_RESTRICT, size_t);

SHIM_API ssize_t
shim_ctime_memdiff (void const * SHIM_RESTRICT,
		    void const * SHIM_RESTRICT,
		    ssize_t const);

SHIM_API bool
shim_iszero (void const * SHIM_RESTRICT, ssize_t const);

SHIM_API bool
shim_ctime_iszero (void const * SHIM_RESTRICT, ssize_t const);

#ifdef SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS
#	define SWAP_API_ SHIM_API
#else
#	define SWAP_API_ static inline
#endif

SWAP_API_ uint16_t
shim_swap_16 (uint16_t);

SWAP_API_ uint32_t
shim_swap_32 (uint32_t);

SWAP_API_ uint64_t
shim_swap_64 (uint64_t);

#undef SWAP_API_

SHIM_END_DECLS

/* shim_obtain_os_entropy Implementation */
#if    defined (SHIM_OS_MAC) || \
      (defined (__NetBSD__) && (__NetBSD_Version__ < 1000000000)) || \
       defined (__Dragonfly__)
#	ifdef SHIM_OS_MAC
#		define SHIM_OPERATIONS_DEV_RANDOM "/dev/random"
#	else
#		define SHIM_OPERATIONS_DEV_RANDOM "/dev/urandom"
#	endif /* ~ ifdef SHIM_OS_MAC */
#	define SHIM_OPERATIONS_OBTAIN_OS_ENTROPY_IMPL(ptr_v, size_v) \
		{ \
			Shim_File_t dev_random = shim_enforce_open_filepath( SHIM_OPERATIONS_DEV_RANDOM, true ); \
			if( read( dev_random, ptr_v, size_v ) != ((ssize_t)size_v) ) \
				shim_errx("Error: Failed to read from " SHIM_OPERATIONS_DEV_RANDOM "\n"); \
			shim_enforce_close_file( dev_random ); \
		}
#elif  defined (__gnu_linux__)
#	define SHIM_OPERATIONS_GETRANDOM_MAX 256
#	define SHIM_OPERATIONS_GETRANDOM_ERROR_IMPL(max) "Error: getrandom(%p, " #max ") failed!\n"
#	define SHIM_OPERATIONS_GETRANDOM_ERROR(max) SHIM_OPERATIONS_GETRANDOM_ERROR_IMPL (max)
#	define SHIM_OPERATIONS_OBTAIN_OS_ENTROPY_IMPL(u8_ptr_v, size_v) \
	{ \
		while( size_v > SHIM_OPERATIONS_GETRANDOM_MAX ) { \
			if( getrandom( u8_ptr_v, SHIM_OPERATIONS_GETRANDOM_MAX, 0 ) != ((ssize_t)SHIM_OPERATIONS_GETRANDOM_MAX) ) \
				shim_errx("Error: getrandom(%p, %zu) failed!\n", ((void *)u8_ptr_v), ((size_t)SHIM_OPERATIONS_GETRANDOM_MAX)); \
			size_v   -= SHIM_OPERATIONS_GETRANDOM_MAX; \
			u8_ptr_v += SHIM_OPERATIONS_GETRANDOM_MAX; \
		} \
		if( getrandom( u8_ptr_v, size_v, 0 ) != ((ssize_t)size_v) ) \
			shim_errx("Error: getrandom(%p, %zu) failed!\n", (void *)u8_ptr_v, size_v); \
	}
#elif  defined (SHIM_OS_UNIXLIKE)
#	define SHIM_OPERATIONS_GETENTROPY_MAX 256
#	define SHIM_OPERATIONS_OBTAIN_OS_ENTROPY_IMPL(u8_ptr_v, size_v) \
		{ \
			while( size_v > SHIM_OPERATIONS_GETENTROPY_MAX ) { \
				if( getentropy( u8_ptr_v, SHIM_OPERATIONS_GETENTROPY_MAX ) ) \
					shim_errx("Error: Failed to getentropy()\n"); \
				size_v   -= SHIM_OPERATIONS_GETENTROPY_MAX; \
				u8_ptr_v += SHIM_OPERATIONS_GETENTROPY_MAX; \
			} \
			if( getentropy( u8_ptr_v, size_v ) ) \
				shim_errx("Error: Failed to getentropy()\n"); \
		}
#elif  defined (SHIM_OS_WINDOWS)
#	define SHIM_OPERATIONS_OBTAIN_OS_ENTROPY_IMPL(ptr_v, size_v) \
		{ \
			BCRYPT_ALG_HANDLE cng_h; \
			if( BCryptOpenAlgorithmProvider( &cng_h, L"RNG", NULL, 0 ) != STATUS_SUCCESS ) \
				shim_errx("Error: BCryptOpenAlgorithmProvider() failed\n"); \
			if( BCryptGenRandom( cng_h, ptr_v, size_v, 0 ) != STATUS_SUCCESS ) \
				shim_errx("Error: BCryptGenRandom() failed\n"); \
			if( BCryptCloseAlgorithmProvider( cng_h, 0 ) != STATUS_SUCCESS ) \
				shim_errx("Error: BCryptCloseAlgorithmProvider() failed\n"); \
		}
#else
#	error "Unsupported OS."
#endif /* ~ ifdef (os_0) elif (os_1) ... */

/* Swap Functions Implementation */
#define SHIM_OPERATIONS_SWAP_F(size, u) \
	SHIM_OPERATIONS_SWAP_F_IMPL (size, u)

#ifdef SHIM_OS_MAC
#	define SHIM_OPERATIONS_NO_NATIVE_SWAP_FUNCTIONS
#endif /* ~ ifdef SHIM_OS_MAC */

#if    defined (__OpenBSD__)
#	define SHIM_OPERATIONS_SWAP_F_IMPL(size, u)	swap##size( u )
#elif  defined (__FreeBSD__) || \
       defined (__NetBSD__)  || \
       defined (__Dragonfly__)
#	define SHIM_OPERATIONS_SWAP_F_IMPL(size, u)	bswap##size( u )
#elif  defined (__gnu_linux__)
#	define SHIM_OPERATIONS_SWAP_F_IMPL(size, u)	bswap_##size( u )
#elif  defined (SHIM_OS_WINDOWS)
#	define SHIM_OPERATIONS_SWAP_F_IMPL(size, u)	_byteswap_##size( u )
#elif !defined (SHIM_OPERATIONS_NO_NATIVE_SWAP_FUNCTIONS)
#	error "Unsupported OS."
#endif

#if    defined (SHIM_OS_UNIXLIKE)
#	define SHIM_OPERATIONS_SWAP_SIZE(unixlike, windows) unixlike
#elif  defined (SHIM_OS_WINDOWS)
#	define SHIM_OPERATIONS_SWAP_SIZE(unixlike, windows) windows
#elif !defined (SHIM_OPERATIONS_NO_NATIVE_SWAP_FUNCTIONS)
#	error "Unsupported OS."
#endif

#ifdef SHIM_OPERATIONS_NO_NATIVE_SWAP_FUNCTIONS
#	define SHIM_OPERATIONS_SWAP_16_IMPL(u16_var) \
		{ \
			return (u16_var >> 8) | (u16_var << 8); \
		}
#	define SHIM_OPERATIONS_SWAP_32_IMPL(u32_var) \
		{ \
			return ( (u32_var >> (3 * 8)) | \
				((u32_var >> (    8)) & UINT32_C (0x0000ff00)) | \
				((u32_var << (    8)) & UINT32_C (0x00ff0000)) | \
				 (u32_var << (3 * 8)) ); \
		}
#	define SHIM_OPERATIONS_SWAP_64_IMPL(u64_var) \
		{ \
			return ( (u64_var >> (7 * 8)) | \
				((u64_var >> (5 * 8)) & UINT64_C (0x000000000000ff00)) | \
				((u64_var >> (3 * 8)) & UINT64_C (0x0000000000ff0000)) | \
				((u64_var >> (    8)) & UINT64_C (0x00000000ff000000)) | \
				((u64_var << (    8)) & UINT64_C (0x000000ff00000000)) | \
				((u64_var << (3 * 8)) & UINT64_C (0x0000ff0000000000)) | \
				((u64_var << (5 * 8)) & UINT64_C (0x00ff000000000000)) | \
				 (u64_var << (7 * 8)) ); \
		}
#else
#	define SHIM_OPERATIONS_SWAP_16_IMPL(u16_var) \
		{ \
			return SHIM_OPERATIONS_SWAP_F (SHIM_OPERATIONS_SWAP_SIZE (16, ushort), u16_var); \
		}
#	define SHIM_OPERATIONS_SWAP_32_IMPL(u32_var) \
		{ \
			return SHIM_OPERATIONS_SWAP_F (SHIM_OPERATIONS_SWAP_SIZE (32, ulong), u32_var); \
		}
#	define SHIM_OPERATIONS_SWAP_64_IMPL(u64_var) \
		{ \
			return SHIM_OPERATIONS_SWAP_F (SHIM_OPERATIONS_SWAP_SIZE (64, uint64), u64_var); \
		}
#endif

#ifdef SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY
void
shim_obtain_os_entropy (uint8_t * SHIM_RESTRICT buffer, size_t num_bytes)
	SHIM_OPERATIONS_OBTAIN_OS_ENTROPY_IMPL (buffer, num_bytes)
#endif /* ~ ifdef SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY */

#ifndef SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS
uint16_t
shim_swap_16 (uint16_t u16)
	SHIM_OPERATIONS_SWAP_16_IMPL (u16)
uint32_t
shim_swap_32 (uint32_t u32)
	SHIM_OPERATIONS_SWAP_32_IMPL (u32)
uint64_t
shim_swap_64 (uint64_t u64)
	SHIM_OPERATIONS_SWAP_64_IMPL (u64)
#endif /* ~ ifndef SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS */

void
shim_secure_zero (void * SHIM_RESTRICT buffer, size_t num_bytes) {
#if    defined(SHIM_OS_MAC)
	memset_s(buffer, num_bytes, 0, num_bytes);
#elif  defined(__NetBSD__)
	explicit_memset(buffer, 0, num_bytes);
#elif  defined(SHIM_OS_UNIXLIKE)
	explicit_bzero(buffer, num_bytes);
#elif  defined(SHIM_OS_WINDOWS)
	SecureZeroMemory(buffer, num_bytes);
#else
#	error "Unsupported operating system."
#endif /* ~ if defined (os's) ... */
} /* ~ shim_secure_zero(buffer, size) */

#endif /* ~ ifndef SHIM_OPERATIONS_H */
