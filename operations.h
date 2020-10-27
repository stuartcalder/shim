/* Copyright (c) 2020 Stuart Steven Calder
 * See accompanying LICENSE file for licensing information.
 */
#ifndef SHIM_OPERATIONS_H
#define SHIM_OPERATIONS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include "macros.h"
#include "errors.h"

#if    defined (SHIM_OS_UNIXLIKE)
#	include <unistd.h>
#	if    defined (__OpenBSD__)
#		include <endian.h>
#	elif  defined (__FreeBSD__) || defined (__Dragonfly__)
#		include <sys/endian.h>
#		ifdef __Dragonfly__
#			define SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY
#		endif
#	elif  defined (__NetBSD__)
#		include <sys/types.h>
#		include <machine/bswap.h>
#		include <string.h>
#		include <sys/param.h>
#		if    (__NetBSD_Version__ < 1000000000)
#			include "files.h"
#			define SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY
#		endif
#	elif  defined (__gnu_linux__)
#		include <byteswap.h>
#	elif  defined (SHIM_OS_OSX)
#		define SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS
#		define SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY
#		if   !defined (__STDC_WANT_LIB_EXT1__) || (__STDC_WANT_LIB_EXT1__ != 1)
#			error "The macro __STDC_WANT_LIB_EXT1__ must be #defined to 1 for access to memset_s."
#		endif
#		include "files.h"
#		include <string.h>
#	else
#		error "Unsupported unix-like OS."
#	endif
#elif  defined (SHIM_OS_WINDOWS)
#	include <windows.h>
#	include <ntstatus.h>
#	include <bcrypt.h>
#	include <stdlib.h>
#else
#	error "Unsupported operating system."
#endif

#define SHIM_BITS_TO_BYTES(bits)	(bits / CHAR_BIT)
#define SHIM_BYTES_TO_BITS(bytes)	(bytes * CHAR_BIT)

#define SHIM_ROT_UNSIGNED_MASK_T_(bits) \
	uint##bits##_t

#define SHIM_ROT_UNSIGNED_MASK_(bits) \
	((SHIM_ROT_UNSIGNED_MASK_T_(bits))(SHIM_BYTES_TO_BITS(sizeof(SHIM_ROT_UNSIGNED_MASK_T_(bits))) - 1))
#define SHIM_ROT_MASKED_COUNT_(bits, count) \
	((SHIM_ROT_UNSIGNED_MASK_(bits)) & count)

#define SHIM_ROT_LEFT(value, count, bits) \
	((value << SHIM_ROT_MASKED_COUNT_(bits,count)) | \
	 (value >> \
	  ((-SHIM_ROT_MASKED_COUNT_(bits,count)) & SHIM_ROT_UNSIGNED_MASK_(bits)) \
	  ))

#define SHIM_ROT_RIGHT(value, count, bits) \
	((value >> SHIM_ROT_MASKED_COUNT_(bits,count)) | \
	 (value << \
	  ((-SHIM_ROT_MASKED_COUNT_(bits,count)) & SHIM_ROT_UNSIGNED_MASK_(bits)) \
	  ))

#define SHIM_BIT_CAST_OP(ptr, type_t, tmp_name, statement) \
	SHIM_MACRO_SHIELD \
		type_t tmp_name; \
		memcpy( &tmp_name, ptr, sizeof(tmp_name) ); \
		statement; \
		memcpy( ptr, &tmp_name, sizeof(tmp_name) ); \
	SHIM_MACRO_SHIELD_EXIT

SHIM_BEGIN_DECLS

#ifdef SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS
#	define SWAP_DECL_ /*nil*/
#else
#	define SWAP_DECL_ static inline
#endif
#ifdef SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY
#	define OS_ENT_DECL_ static inline
#else
#	define OS_ENT_DECL_ /*nil*/
#endif

void
shim_xor_16 (void * SHIM_RESTRICT, void const * SHIM_RESTRICT);

void
shim_xor_32 (void * SHIM_RESTRICT, void const * SHIM_RESTRICT);

void
shim_xor_64 (void * SHIM_RESTRICT, void const * SHIM_RESTRICT);

void
shim_xor_128 (void * SHIM_RESTRICT, void const * SHIM_RESTRICT);

void *
shim_checked_malloc (size_t size);

OS_ENT_DECL_ void
shim_obtain_os_entropy (uint8_t * SHIM_RESTRICT, size_t);

static inline void
shim_secure_zero (void * SHIM_RESTRICT, size_t);

int
shim_ctime_memcmp (void const * SHIM_RESTRICT,
		   void const * SHIM_RESTRICT,
		   size_t const);

SWAP_DECL_ uint16_t
shim_swap_16 (uint16_t);

SWAP_DECL_ uint32_t
shim_swap_32 (uint32_t);

SWAP_DECL_ uint64_t
shim_swap_64 (uint64_t);

#undef SWAP_DECL_
#undef OS_ENT_DECL_

SHIM_END_DECLS

#ifdef SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY
void
shim_obtain_os_entropy (uint8_t * SHIM_RESTRICT buffer, size_t num_bytes) {
#if    defined (SHIM_OS_OSX) || \
      (defined (__NetBSD__) && (__NetBSD_Version__ < 1000000000)) || \
       defined (__Dragonfly__)
#	if    defined (SHIM_OS_OSX)
#		define DEV_RANDOM_ "/dev/random"
#	else
#		define DEV_RANDOM_ "/dev/urandom"
#	endif
	Shim_File_t random_dev = shim_open_existing_filepath( DEV_RANDOM_, true );
	if( read( random_dev, buffer, num_bytes ) != ((ssize_t)num_bytes) )
		SHIM_ERRX ("Error: Failed to read from " DEV_RANDOM_ "\n");
	shim_close_file( random_dev );
#	undef DEV_RANDOM_
#elif  	defined (SHIM_OS_UNIXLIKE)
#	define MAX_ 256
	while( num_bytes > MAX_ ) {
		if( getentropy( buffer, MAX_ ) != 0 )
			SHIM_ERRX ("Error: Failed to getentropy()\n");
		num_bytes -= MAX_;
		buffer    += MAX_;
	}
	if( getentropy( buffer, num_bytes ) != 0 )
		SHIM_ERRX ("Error: Failed to getentropy()\n");
#	undef MAX_
#elif  defined (SHIM_OS_WINDOWS)
	BCRYPT_ALG_HANDLE cng_provider_handle;
	if( BCryptOpenAlgorithmProvider( &cng_provider_handle, L"RNG", NULL, 0 ) != STATUS_SUCCESS )
		SHIM_ERRX ("Error: BCryptOpenAlgorithmProvider() failed\n");
	if( BCryptGenRandom( cng_provider_handle, buffer, num_bytes, 0 ) != STATUS_SUCCESS )
		SHIM_ERRX ("Error: BCryptGenRandom() failed\n");
	if( BCryptCloseAlgorithmProvider( cng_provider_handle, 0 ) != STATUS_SUCCESS )
		SHIM_ERRX ("Error: BCryptCloseAlgorithmProvider() failed\n");
#	else
#		error "Unsupported operating system."
#	endif
} // ~ shim_obtain_os_entropy(...)
#endif /* SHIM_OPERATIONS_INLINE_OBTAIN_OS_ENTROPY */

void
shim_secure_zero (void * SHIM_RESTRICT buffer, size_t num_bytes) {
#if    defined (SHIM_OS_OSX)
	(void)memset_s( buffer, num_bytes, 0, num_bytes );
#elif  defined (__NetBSD__)
	(void)explicit_memset( buffer, 0, num_bytes );
#elif  defined (SHIM_OS_UNIXLIKE)
	explicit_bzero( buffer, num_bytes );
#elif  defined (SHIM_OS_WINDOWS)
	SecureZeroMemory( buffer, num_bytes );
#else
#	error "Unsupported operating system."
#endif
} // ~ shim_secure_zero(...)

#ifndef SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS

#	define SWAP_F_(size, u) \
		SWAP_F_IMPL_ (size, u)
#	if    defined (__OpenBSD__)
#		define SWAP_F_IMPL_(size,u)	swap##size( u )
#	elif  defined (__FreeBSD__) || defined (__NetBSD__) || defined (__Dragonfly__)
#		define SWAP_F_IMPL_(size,u)	bswap##size( u )
#	elif  defined (__gnu_linux__)
#		define SWAP_F_IMPL_(size,u)	bswap_##size( u )
#	elif  defined (SHIM_OS_WINDOWS)
#		define SWAP_F_IMPL_(size,u)	_byteswap_##size( u )
#	elif !defined (SHIM_OS_OSX)
#		error "Unsupported operating system."
#	endif

#	if    defined (SHIM_OS_UNIXLIKE) && !defined (SHIM_OS_OSX)
#		define SIZE_(unixlike, windows)	unixlike
#	elif  defined (SHIM_OS_WINDOWS)
#		define SIZE_(unixlike, windows) windows
#	elif !defined (SHIM_OS_OSX)
#		error "Unsupported operating system."
#	endif

uint16_t
shim_swap_16 (uint16_t val) {
#	ifdef SHIM_OS_OSX
	return (val >> 8) | (val << 8);
#	else
	return SWAP_F_ (SIZE_ (16,ushort),val);
#	endif
}

uint32_t
shim_swap_32 (uint32_t val) {
#	ifdef SHIM_OS_OSX
	return (
		 (val >> (3*8)) |
		((val >>    8 ) & UINT32_C (0x0000ff00)) |
		((val <<    8 ) & UINT32_C (0x00ff0000)) |
		 (val << (3*8))
	       );
#	else
	return SWAP_F_ (SIZE_ (32,ulong),val);
#	endif
}

uint64_t
shim_swap_64 (uint64_t val) {
#	ifdef SHIM_OS_OSX
	return (
		 (val >> (7*8))                                  |
		((val >> (5*8)) & UINT64_C (0x000000000000ff00)) |
		((val >> (3*8)) & UINT64_C (0x0000000000ff0000)) |
		((val >>    8 ) & UINT64_C (0x00000000ff000000)) |
		((val <<    8 ) & UINT64_C (0x000000ff00000000)) |
		((val << (3*8)) & UINT64_C (0x0000ff0000000000)) |
		((val << (5*8)) & UINT64_C (0x00ff000000000000)) |
		 (val << (7*8))
	       );
#	else
	return SWAP_F_ (SIZE_ (64,uint64),val);
#	endif
}


#	undef SIZE_
#	undef SWAP_F_IMPL_
#	undef SWAP_F_
#endif /* SHIM_OPERATIONS_NO_INLINE_SWAP_FUNCTIONS */

#endif // ~ SHIM_OPERATIONS_H
