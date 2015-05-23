/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include "Serialization.h"
#include "yasli/Enum.h"
#include "PropertyTreeModel.h"
#include "PropertyIArchive.h"
#include "PropertyRowBool.h"
#include "PropertyRowString.h"
#include "PropertyRowNumber.h"
#include "PropertyRowPointer.h"
#include "PropertyRowObject.h"
#include "Unicode.h"

using yasli::TypeID;

PropertyIArchive::PropertyIArchive(PropertyTreeModel* model, PropertyRow* root)
: Archive(INPUT | EDIT)
, model_(model)
, currentNode_(0)
, lastNode_(0)
, root_(root)
{
	stack_.push_back(Level());

	if (!root_)
		root_ = model_->root();
	else
		currentNode_ = root;
}

bool PropertyIArchive::operator()(yasli::StringInterface& value, const char* name, const char* label)
{
	if(openRow(name, label, "string")){
		if(PropertyRowString* row = static_cast<PropertyRowString*>(currentNode_))
 			value.set(fromWideChar(row->value().c_str()).c_str());
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(yasli::WStringInterface& value, const char* name, const char* label)
{
	if(openRow(name, label, "string")){
		if(PropertyRowString* row = static_cast<PropertyRowString*>(currentNode_)) {
			value.set(row->value().c_str());
		}
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(bool& value, const char* name, const char* label)
{
	if(openRow(name, label, "bool")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(char& value, const char* name, const char* label)
{
	if(openRow(name, label, "char")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

// Signed types
bool PropertyIArchive::operator()(signed char& value, const char* name, const char* label)
{
	if(openRow(name, label, "signed char")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(signed short& value, const char* name, const char* label)
{
	if(openRow(name, label, "short")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(signed int& value, const char* name, const char* label)
{
	if(openRow(name, label, "int")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(signed long& value, const char* name, const char* label)
{
	if(openRow(name, label, "long")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(long long& value, const char* name, const char* label)
{
	if(openRow(name, label, "long long")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

// Unsigned types
bool PropertyIArchive::operator()(unsigned char& value, const char* name, const char* label)
{
	if(openRow(name, label, "unsigned char")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(unsigned short& value, const char* name, const char* label)
{
	if(openRow(name, label, "unsigned short")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(unsigned int& value, const char* name, const char* label)
{
	if(openRow(name, label, "unsigned int")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(unsigned long& value, const char* name, const char* label)
{
	if(openRow(name, label, "unsigned long")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(unsigned long long& value, const char* name, const char* label)
{
	if(openRow(name, label, "unsigned long long")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(float& value, const char* name, const char* label)
{
	if(openRow(name, label, "float")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(double& value, const char* name, const char* label)
{
	if(openRow(name, label, "double")){
		currentNode_->assignToPrimitive(&value, sizeof(value));
		closeRow(name);
		return true;
	}
	else
		return false;
}

bool PropertyIArchive::operator()(yasli::ContainerInterface& ser, const char* name, const char* label)
{
	const char* typeName = ser.containerType().name();
	if(!openRow(name, label, typeName))
        return false;

    size_t size = 0;
	if(currentNode_->multiValue())
		size = ser.size();
	else{
		size = currentNode_->count();
		size = ser.resize(size);
	}

	stack_.push_back(Level());

	size_t index = 0;
    if(ser.size() > 0)
        while(index < size)
        {
            ser(*this, "", "<");
            ser.next();
			++index;
        }

	stack_.pop_back();

    closeRow(name);
	return true;
}

bool PropertyIArchive::operator()(const yasli::Serializer& ser, const char* name, const char* label)
{
	PropertyRow* nonLeaf = 0;
	if(openRow(name, label, ser.type().name())){
		if(currentNode_->isLeaf()) {
			if(!currentNode_->isRoot()){
				currentNode_->assignTo(ser);
				closeRow(name);
				return true;
			}
		}
		else
			nonLeaf = currentNode_;
    }
	else
		return false;

	stack_.push_back(Level());

    ser(*this);

	stack_.pop_back();

	if (nonLeaf)
		nonLeaf->closeNonLeaf(ser);
	closeRow(name);
	return true;
}


bool PropertyIArchive::operator()(yasli::PointerInterface& ser, const char* name, const char* label)
{
	const char* baseName = ser.baseType().name();

	if(openRow(name, label, baseName)){
		if (!currentNode_->isPointer()) {
			closeRow(name);
			return false;
		}

		YASLI_ASSERT(currentNode_);
		PropertyRowPointer* row = static_cast<PropertyRowPointer*>(currentNode_);
		if(!row){
			closeRow(name);
			return false;
		}
		row->assignTo(ser);
	}
	else
		return false;

	stack_.push_back(Level());

	if(ser.get() != 0)
		ser.serializer()( *this );

	stack_.pop_back();

	closeRow(name);
	return true;
}

bool PropertyIArchive::operator()(yasli::Object& obj, const char* name, const char* label)
{
	if(openRow(name, label, obj.type().name())){
		bool result = false;
		if (currentNode_->isObject()) {
			PropertyRowObject* rowObj = static_cast<PropertyRowObject*>(currentNode_);
			result = rowObj->assignTo(&obj);
		}
		closeRow(name);
		return result;
	}
	else
		return false;
}

bool PropertyIArchive::openBlock(const char* name, const char* label)
{
	if(openRow(name, label, "block")){
		return true;
	}
	else
		return false;
}

void PropertyIArchive::closeBlock()
{
	closeRow("block");
}

bool PropertyIArchive::openRow(const char* name, const char* label, const char* typeName)
{
	if(!name)
		return false;

	if(!currentNode_){
		lastNode_ = currentNode_ = model_->root();
		YASLI_ASSERT(currentNode_);
		if (currentNode_ && strcmp(currentNode_->typeName(), typeName) != 0)
			return false;
		return true;
	}

	YASLI_ESCAPE(currentNode_, return false);

	if(currentNode_->empty())
		return false;

	Level& level = stack_.back();

	PropertyRow* node = 0;
	if(currentNode_->isContainer()){
		if (level.rowIndex < int(currentNode_->count()))
			node = currentNode_->childByIndex(level.rowIndex);
		++level.rowIndex;
	}
	else {
		node = currentNode_->findFromIndex(&level.rowIndex, name, typeName, level.rowIndex);
		++level.rowIndex;
	}

	if(node){
		lastNode_ = node;
		if(node->isContainer() || !node->multiValue()){
			currentNode_ = node;
			if (currentNode_ && strcmp(currentNode_->typeName(), typeName) != 0)
				return false;
			return true;
		}
	}
	return false;
}

void PropertyIArchive::closeRow(const char* name)
{
	YASLI_ESCAPE(currentNode_, return);
	currentNode_ = currentNode_->parent();
}
