/* Copyright (c) 2020 Stuart Steven Calder
 * See accompanying LICENSE file for licensing information.
 */
#ifndef SHIM_PRINT_H
#define SHIM_PRINT_H

#include "errors.h"
#include "macros.h"
#include <stdint.h>
#include <stdio.h>

SHIM_BEGIN_DECLS

SHIM_API void 
shim_print_byte_buffer (uint8_t * SHIM_RESTRICT, size_t const);

SHIM_END_DECLS

#endif // ~ SHIM_PRINT_H
