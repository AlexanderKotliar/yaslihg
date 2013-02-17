/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once
#include "yasli/Strings.h"
#include <stdio.h>

yasli::string fromWideChar(const wchar_t* wideCharString);
yasli::wstring toWideChar(const char* multiByteString);
yasli::wstring fromANSIToWide(const char* ansiString);
yasli::string toANSIFromWide(const wchar_t* wstr);
