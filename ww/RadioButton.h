/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once

#include "ww/_WidgetWithWindow.h"
#include <string>

namespace ww{

using std::string;
class RadioButtonImpl;
class RadioButtonGroup;

class WW_API RadioButton : public _WidgetWithWindow{
public:
	RadioButton(RadioButton* groupWith = 0, const char* text = "RadioButton", int border = 0);

	const char* text() const { return text_.c_str(); }
	void setText(const char* text);
	void setStatus(bool status);
	bool status() const { return status_; }

	virtual void onChanged();
	signal0& signalChanged() { return signalChanged_; }

	void serialize(Archive& ar);
	RadioButtonGroup* group() const;

protected:
	friend class RadioButtonImpl;
	RadioButtonImpl* window() const;
	// ���������� �������
	signal0 signalChanged_;
	string text_;
	bool status_;
};

}

