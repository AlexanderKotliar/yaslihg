/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */


#pragma once

// Disable C++ RTTI use (e.g. typeid())
#ifndef YASLI_NO_RTTI
#define YASLI_NO_RTTI 1
#endif

// Disable extra struct-level for polymorphic pointer serialization
#ifndef YASLI_NO_EXTRA_BLOCK_FOR_POINTERS
#define YASLI_NO_EXTRA_BLOCK_FOR_POINTERS 0
#endif

// Default filter for Archive: 0 - strict mode, -1 - silent mode.
#ifndef YASLI_DEFAULT_FILTER
#define YASLI_DEFAULT_FILTER 0
#endif

// Toggles between russian and english serialization labels 
#ifndef XMATH_IN_ENGLISH
#define XMATH_IN_ENGLISH 0
#endif

// Serialize std::pair: 1 - ("first", "second") or 0 - ("key", "value")
#ifndef YASLI_STD_PAIR_FIRST_SECOND
#define YASLI_STD_PAIR_FIRST_SECOND 0
#endif


#ifdef _DEBUG
// BinArchives use short hash of name to compact and speed up. Collision on particular level of hierarchy could cause to wrong result.
//#define YASLI_BIN_ARCHIVE_CHECK_HASH_COLLISION 

// Node with empty name hasn't size information. So this block will not be suited for search. Using non-empty names in such a block is dangerous.
#define YASLI_BIN_ARCHIVE_CHECK_EMPTY_NAME_MIX
#endif

#include <stdint.h>
namespace yasli
{
	// See note at CastInteger in Archive.h for details.
	typedef int8_t i8;
	typedef int16_t i16;
	typedef int32_t i32;
	typedef int64_t i64;
	typedef uint8_t u8;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;
};
