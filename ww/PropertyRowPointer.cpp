/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include "StdAfx.h"
#include <math.h>
#include <memory>

#include "ww/PropertyRowPointer.h"
#include "ww/PropertyTreeModel.h"
#include "ww/PropertyDrawContext.h"
#include "ww/PropertyTree.h"

#include "ww/ConstStringList.h"
#include "ww/PopupMenu.h"
#include "ww/Entry.h"
#include "ClassMenu.h"

#include "yasli/BitVector.h"
#include "yasli/Archive.h"
#include "yasli/BitVectorImpl.h"
#include "yasli/ClassFactory.h"
#include "yasli/Enum.h"
#include "ww/SafeCast.h"
#include "ww/PopupMenu.h"
#include "ww/Win32/Window32.h"
#include "ww/Win32/Drawing.h"
#include "ww/Win32/Rectangle.h"
#include "ww/Unicode.h"
#include "ww/Color.h"
#include "gdiplusUtils.h"

namespace ww{

void ClassMenuItemAdder::generateMenu(PopupMenuItem& createItem, const StringList& comboStrings)
{
	StringList::const_iterator it;
	int index = 0;
	FOR_EACH(comboStrings, it){
		StringList path;
		splitStringList(&path, it->c_str(), '\\');
		int level = 0;
		PopupMenuItem* item = &createItem;
		for(int level = 0; level < int(path.size()); ++level){
			const char* leaf = path[level].c_str();
			if(level == path.size() - 1){
				(*this)(*item, index++, leaf);
			}
			else{
				if(PopupMenuItem* subItem = item->find(leaf))
					item = subItem;
				else
					item = &item->add(leaf);
			}
		}
	}
}

// ---------------------------------------------------------------------------
YASLI_CLASS(PropertyRow, PropertyRowPointer, "SharedPtr");

PropertyRowPointer::PropertyRowPointer()
: PropertyRow("", "", "")
, factory_(0)
{
}

PropertyRowPointer::PropertyRowPointer(const char* name, const char* label, const PointerInterface &ptr)
: PropertyRow(name, label, ptr.baseType().name())
, baseType_(ptr.baseType())
, derivedType_(ptr.type())
, factory_(ptr.factory())
{
    serializer_ = ptr.serializer();
}

PropertyRowPointer::PropertyRowPointer(const char* name, const char* label, TypeID baseType, TypeID derivedType, ClassFactoryBase* factory)
: PropertyRow(name, label, baseType.name())
, baseType_(baseType)
, derivedType_(derivedType)
, factory_(factory)
{
}

bool PropertyRowPointer::assignTo(PointerInterface &ptr)
{
	if ( ptr.type() != derivedType_ )
	{
		ptr.create(derivedType_);
	}
    return true;
}


void PropertyRowPointer::onMenuCreateByIndex(int index, bool useDefaultValue, PropertyTree* tree)
{
    tree->model()->push(this);
	if(index < 0){ // NULL value
		derivedType_ = TypeID();
	    clear();
	}
	else{
		YASLI_ASSERT(typeName_);
		const PropertyDefaultTypeValue* defaultValue = tree->model()->defaultType(baseType_, index);
		if (defaultValue && defaultValue->root) {
			YASLI_ASSERT(defaultValue->root->refCount() == 1);
            if(useDefaultValue){
                clear();
				cloneChildren(this, defaultValue->root);
            }
			derivedType_ = defaultValue->type;
			tree->expandRow(this);
		}
        else{
            derivedType_ = TypeID();
            clear();
        }
	}
	tree->model()->rowChanged(this);
}


string PropertyRowPointer::valueAsString() const
{
    string result = derivedType_.name();
		if (factory_) {
			const TypeDescription* desc = factory_->descriptionByType(derivedType_);
			if (desc)
				result = desc->label();
		}
    if(digest()[0] != L'\0'){
        result += " ( ";
        result += fromWideChar(digest());
        result += " )";
    }
	return result;
}

wstring PropertyRowPointer::generateLabel() const
{
	if(multiValue())
		return L"...";

    wstring str;
	if(derivedType_){
		const char* textStart = derivedType_.name();
		if (factory_) {
			const TypeDescription* desc = factory_->descriptionByType(derivedType_);
			if (desc)
				textStart = desc->label();
		}
		const char* p = textStart + strlen(textStart);
		while(p > textStart){
			if(*(p - 1) == '\\')
				break;
			--p;
		}
		str = toWideChar(p);
		if(p != textStart){
			str += L" (";
			str += toWideChar(string(textStart, p - 1).c_str());
			str += L")";
		}
	}
	else
    {
        YASLI_ESCAPE(factory_ != 0, return L"NULL");
        str = toWideChar(factory_->nullLabel() ? factory_->nullLabel() : "[ null ]");
    }
    return str;
}

void PropertyRowPointer::redraw(const PropertyDrawContext& context)
{
	using namespace Gdiplus;
	using Gdiplus::Color;
	using Gdiplus::Rect;
	Rect widgetRect = gdiplusRect(context.widgetRect);

	Gdiplus::Rect rt(widgetRect.X, widgetRect.Y + 1, widgetRect.Width - 2, widgetRect.Height - 2);
	Color brushColor = gdiplusSysColor(COLOR_BTNFACE);
	LinearGradientBrush brush(Gdiplus::Rect(rt.X, rt.Y, rt.Width, rt.Height + 3), Color(255, 0, 0, 0), brushColor, LinearGradientModeVertical);

	Color colors[3] = { brushColor, brushColor, gdiplusSysColor(COLOR_3DSHADOW) };
	Gdiplus::REAL positions[3] = { 0.0f, 0.6f, 1.0f };
	brush.SetInterpolationColors(colors, positions, 3);

	fillRoundRectangle(context.graphics, &brush, rt, gdiplusSysColor(COLOR_3DSHADOW), 6);

	ww::Color textColor;
    textColor.setGDI(userReadOnly() ? GetSysColor(COLOR_3DSHADOW) : GetSysColor(COLOR_WINDOWTEXT));

	ww::Rect textRect(widgetRect.X + 4, widgetRect.Y, widgetRect.GetRight() - 5, widgetRect.GetBottom());
	wstring str = generateLabel();
	const wchar_t* text = str.c_str();;
	//context.drawValueText(false, text);
	Gdiplus::Font* font = derivedType_ == TypeID() ? propertyTreeDefaultFont() : propertyTreeDefaultBoldFont();
	context.tree->_drawRowValue(context.graphics, text, font, textRect, textColor, false, false);
}

struct ClassMenuItemAdderRowPointer : ClassMenuItemAdder{
	ClassMenuItemAdderRowPointer(PropertyRowPointer* row, PropertyTree* tree) : row_(row), tree_(tree) {}    
	void operator()(PopupMenuItem& root, int index, const char* text){
		root.add(text, index, !tree_->immediateUpdate(), tree_).connect(row_, &PropertyRowPointer::onMenuCreateByIndex);
	}
protected:
	PropertyRowPointer* row_;
	PropertyTree* tree_;
};


bool PropertyRowPointer::onActivate( PropertyTree* tree, bool force)
{
    if(userReadOnly())
        return false;
    PopupMenu menu;
    ClassMenuItemAdderRowPointer(this, tree).generateMenu(menu.root(), tree->model()->typeStringList(baseType()));
    menu.spawn(tree->_toScreen(Vect2(widgetPos_, pos_.y + ROW_DEFAULT_HEIGHT)), tree);
    return true;
}

bool PropertyRowPointer::onMouseDown(PropertyTree* tree, Vect2 point, bool& changed) 
{
    if(widgetRect().pointInside(point)){
        if(onActivate(tree, false))
            changed = true;
    }
    return false; 
}

bool PropertyRowPointer::onContextMenu(PopupMenuItem &menu, PropertyTree* tree)
{
	if(!menu.empty())
		menu.addSeparator();
    if(!userReadOnly()){
	    PopupMenuItem0& createItem = menu.add("Set");
	    ClassMenuItemAdderRowPointer(this, tree).generateMenu(createItem, tree->model()->typeStringList(baseType()));
    }

	return PropertyRow::onContextMenu(menu, tree);
}

void PropertyRowPointer::serializeValue(Archive& ar)
{
	if (factory_) {
		TypeIDWithFactory pair(derivedType_, factory_);
		ar(pair, "derivedType", "DerivedType");
		if (ar.isInput())
			derivedType_ = pair.type;
	}
}

int PropertyRowPointer::widgetSizeMin() const
{
    HDC dc = GetDC(Win32::getDefaultWindowHandle());
    Gdiplus::Graphics gr(dc);
    Gdiplus::Font* font = propertyTreeDefaultBoldFont();
    wstring wstr(generateLabel());
    Gdiplus::StringFormat format;
    Gdiplus::RectF bound;
    gr.MeasureString(wstr.c_str(), (int)wstr.size(), font, Gdiplus::RectF(0.0f, 0.0f, 0.0f, 0.0f), &format, &bound, 0);
    ReleaseDC(Win32::getDefaultWindowHandle(), dc);
    return bound.Width + 10;
}

}
// vim:ts=4 sw=4:
