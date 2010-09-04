#pragma once

#include <windows.h>

namespace Win32{

class MemoryDC{
public:
	MemoryDC(HDC dc);
	~MemoryDC();

	operator HDC() const{ return dc_; }
protected:
	RECT rect_;
	HDC destinationDC_;
	HDC dc_;
	HBITMAP bitmap_;
	HBITMAP oldBitmap_;
};

}

