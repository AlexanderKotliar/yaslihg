/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once

#include "PropertyRowImpl.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"

#include <QtGui/QLineEdit>

class PropertyRowString : public PropertyRowImpl<yasli::wstring, PropertyRowString>
{
public:
	enum { Custom = false };
	PropertyRowString(const char* name = "", const char* nameAlt = "", const wchar_t* value = L"");
	PropertyRowString(const char* name, const char* nameAlt, const char* value);
	PropertyRowString(void* object, size_t size, const char* name, const char* nameAlt, const char* typeName); // понадобился из за PropertyRowImpl
	bool assignTo(yasli::string& str);
	bool assignTo(yasli::wstring& str);
	using PropertyRowImpl<yasli::wstring, PropertyRowString>::assignTo;
	PropertyRowWidget* createWidget(QPropertyTree* tree);
	yasli::string valueAsString() const;
	yasli::wstring valueAsWString() const { return value_; }
};

class PropertyRowWidgetString : public PropertyRowWidget
{
	Q_OBJECT
public:
  PropertyRowWidgetString(PropertyRowString* row, QPropertyTree* tree)
	: PropertyRowWidget(row, tree)
	, entry_(new QLineEdit())
	, tree_(tree)
	{
		initialValue_ = QString::fromUtf16((ushort*)row->value().c_str());
		entry_->setText(initialValue_);
		entry_->selectAll();
		connect(entry_.data(), SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));
	}
	~PropertyRowWidgetString()
	{
		entry_->hide();
		entry_->setParent(0);
		entry_.take()->deleteLater();
	}

	void commit(){
		//if(entry_)
		//	entry_->commit();
	}
	QWidget* actualWidget() { return entry_.data(); }

	public slots:
	void onEditingFinished(){
		PropertyRowString* row = static_cast<PropertyRowString*>(this->row());
		if(initialValue_ != entry_->text() || row_->multiValue()){
			model()->push(row);
			row->setValue((wchar_t*)entry_->text().data());
			model()->rowChanged(row);
		}
		else
			tree_->_cancelWidget();
	}
protected:
    QPropertyTree* tree_;
	QScopedPointer<QLineEdit> entry_;
    QString initialValue_;
};