#pragma once

#include "yasli/Archive.h"

namespace yasli{

struct Button
{
	Button(const char* _text = 0)
	: pressed(false)
	, text(_text) {}

	operator bool() const{
		return pressed;
	}
	void serialize(yasli::Archive& ar) {}

	bool pressed;
	const char* text;
};

inline bool serialize(Archive& ar, Button& button, const char* name, const char* label)
{
	if (ar.isEdit())
		return ar(Serializer(button), name, label);
	else
	{
		button.pressed = false;
		return false;
	}
}

}