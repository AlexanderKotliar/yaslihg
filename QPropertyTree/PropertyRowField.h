/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once
#include "PropertyRow.h"
class QIcon;

class PropertyRowField : public PropertyRow
{
public:
	WidgetPlacement widgetPlacement() const{ return WIDGET_VALUE; }
	int widgetSizeMin() const{ 
		if (userWidgetSize() >= 0)
			return userWidgetSize();
		else
			return 40;
	}

	virtual int buttonCount() const{ return 0; }
	virtual const QIcon& buttonIcon(const QPropertyTree* tree, int index) const{ return *(QIcon*)(0); }
	virtual bool usePathEllipsis() const { return false; }

	void redraw(const PropertyDrawContext& context);
};

