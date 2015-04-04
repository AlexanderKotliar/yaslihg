/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include <math.h>

#include "Factory.h"
#include "PropertyRowStringListValue.h"
#include "PropertyTreeModel.h"
#include "PropertyDrawContext.h"
#include "PropertyTree.h"
#include "ConstStringList.h"

#include "yasli/Archive.h"
#include "yasli/ClassFactory.h"
#include "IMenu.h"

using yasli::StringList;
using yasli::StringListValue;

REGISTER_PROPERTY_ROW(StringListValue, PropertyRowStringListValue)


InplaceWidget* PropertyRowStringListValue::createWidget(PropertyTree* tree)
{
	return new InplaceWidgetComboBox(this, this, (QPropertyTree*)tree);
}

// ---------------------------------------------------------------------------
REGISTER_PROPERTY_ROW(StringListStaticValue, PropertyRowStringListStaticValue)

InplaceWidget* PropertyRowStringListStaticValue::createWidget(PropertyTree* tree)
{
	return new InplaceWidgetComboBox(this, this, (QPropertyTree*)tree);
}

DECLARE_SEGMENT(PropertyRowStringList)

// vim:ts=4 sw=4:
