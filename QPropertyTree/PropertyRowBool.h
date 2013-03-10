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
#include "PropertyRow.h"
#include "Unicode.h"

class PropertyRowBool : public PropertyRow
{
public:
	enum { Custom = false };
	PropertyRowBool();
	PropertyRowBool(const char* name, const char* nameAlt, const char* typeName);
	bool assignTo(void* val, size_t size);
	void setValue(bool value) { value_ = value; }

	void redraw(const PropertyDrawContext& context);
	bool isLeaf() const{ return true; }
	bool isStatic() const{ return false; }

	bool onActivate(QPropertyTree* tree, bool force);
	void digestReset(const QPropertyTree* tree);
	yasli::wstring valueAsWString() const{ return value_ ? L"true" : L"false"; }
    yasli::string valueAsString() const{ return value_ ? "true" : "false"; }
    yasli::wstring digestValue() const { return value_ ? toWideChar(labelUndecorated()) : yasli::wstring(); }
	WidgetPlacement widgetPlacement() const{ return WIDGET_ICON; }
	PropertyRow* clone() const{
		PropertyRowBool* result = new PropertyRowBool(name_, label_, typeName_);
		result->value_ = value_;
		return cloneChildren(result, this);
	}
    void serializeValue(yasli::Archive& ar);
	int widgetSizeMin() const{ return ICON_SIZE; }
protected:
	bool value_;
};

