#pragma once

#include "ww/_WidgetWithWindow.h"

class HLineImpl;

namespace ww{

	class WW_API HLine : public _WidgetWithWindow{
	public:
		HLine(int border = 0);
		bool _focusable() const{ return false; }
	protected:
		// ���������� �������
		HLineImpl* window() const{ return reinterpret_cast<HLineImpl*>(_window()); }
	};

}

