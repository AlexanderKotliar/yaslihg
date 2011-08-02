/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once

#include "ww/API.h"
#include "ww/Win32/Types.h"
#include "ww/Vect2.h"

namespace ww{

class WW_API ImageStore : public RefCounter{
public:
	ImageStore(int cx, int cy, const char* resourceName = 0, unsigned int maskColor = 0);
	~ImageStore();
	void addFromResource(const char* bitmapID, unsigned int color);
	void addFromFile(const char* fileName, unsigned int color);
	Vect2 slideSize() const { return size_; }

	void _draw(int index, HDC destDC, int x, int y, bool disabled = false);
protected:
	void _createFromBitmap(HBITMAP bitmap, unsigned int color);

	HBITMAP bitmap_;
	HBITMAP bitmapGray_;
	HIMAGELIST imageList_;
	HIMAGELIST imageListGray_;
	Vect2 size_;
};

}

