/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once
#include "PropertyRow.h"

class PropertyRowContainer;
struct ContainerMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:

	QPropertyTree* tree;
	PropertyRowContainer* container;
	PropertyRow* element;
	int pointerIndex;

	ContainerMenuHandler(QPropertyTree* tree, PropertyRowContainer* container);

public slots:
	void onMenuAppendElement();
	void onMenuAppendPointerByIndex();
	void onMenuRemoveAll();
	void onMenuChildInsertBefore();
	void onMenuChildRemove();
};

class PropertyRowContainer : public PropertyRow
{
public:
	enum { Custom = false };
	PropertyRowContainer(const char* name, const char* nameAlt, const char* typeName, const char* elementTypeName, bool fixedSizeContainer);
	bool isContainer() const{ return true; }
	bool onActivate( QPropertyTree* tree, bool force);
	bool onContextMenu(QMenu& item, QPropertyTree* tree);
	void redraw(const PropertyDrawContext& context);
	bool onKeyDown(QPropertyTree* tree, const QKeyEvent* key) override;

	PropertyRow* clone() const{
		return cloneChildren(new PropertyRowContainer(name_, label_, typeName_, elementTypeName_, userReadOnly_), this);
	}
	void digestReset(const QPropertyTree* tree);
	bool isStatic() const{ return false; }
	bool isSelectable() const{ return userWidgetSize() == 0 ? false : true; }

	PropertyRow* defaultRow(PropertyTreeModel* model);
	const PropertyRow* defaultRow(const PropertyTreeModel* model) const;
	void serializeValue(yasli::Archive& ar);

	const char* elementTypeName() const{ return elementTypeName_; }
	yasli::string valueAsString() const;
	// C-array is an example of fixed size container
	bool isFixedSize() const{ return fixedSize_; }
	WidgetPlacement widgetPlacement() const{ return WIDGET_AFTER_NAME; }
	int widgetSizeMin() const{ return userWidgetSize() >=0 ? userWidgetSize() : 36; }

protected:
	void generateMenu(QMenu& menu, QPropertyTree* tree);

	bool fixedSize_;
	const char* elementTypeName_;
	wchar_t buttonLabel_[8];
};