#pragma once

#include <windows.h>
#include "XMath/Recti.h"
#include "XMath/Colors.h"

namespace Win32{

struct Rect : ::RECT{
	Rect()
	{}
	Rect(POINT leftTop, POINT rightBottom)
	{
		left = leftTop.x;
		top = leftTop.y;
		right = rightBottom.x;
		bottom = rightBottom.y;
	}
	Rect(Vect2i leftTop, Vect2i rightBottom)
	{
		left = leftTop.x;
		top = leftTop.y;
		right = rightBottom.x;
		bottom = rightBottom.y;
	}
	Rect(int left, int top, int right, int bottom)
	{
		this->left = left;
		this->top = top;
		this->right = right;
		this->bottom = bottom;
	}
	explicit Rect(const Recti& rect)
	{
		left = rect.left();
		top = rect.top();
		right = rect.right();
		bottom = rect.bottom();
	}
	Rect(const RECT& rect)
	{
		left = rect.left;
		top = rect.top;
		right = rect.right;
		bottom = rect.bottom;
	}

	int width() const{ return right - left; }
	int height() const{ return bottom - top; }

	bool pointIn(Vect2i point) const{
		return point.x >= left && point.x < right &&
			   point.y >= top && point.y < bottom;
	}
	Recti recti(){
		return Recti(left, top, right, bottom);
	}
};


}

