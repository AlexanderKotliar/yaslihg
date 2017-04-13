/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once

#include "yasli/Config.h"
#include "yasli/Archive.h"
#include "yasli/Pointers.h"

class PropertyRow;
class PropertyRowStruct;
class PropertyTreeModel;
class ValidatorBlock;
using yasli::SharedPtr;

class PropertyOArchive final : public yasli::Archive{
public:
	PropertyOArchive(PropertyTreeModel* model, PropertyRowStruct* rootNode, ValidatorBlock* validator);
	~PropertyOArchive();
	
	void finalize();

	void setOutlineMode(bool outlineMode);

	bool operator()(yasli::StringInterface& value, const char* name, const char* label) override;
	bool operator()(yasli::WStringInterface& value, const char* name, const char* label) override;
	bool operator()(bool& value, const char* name, const char* label) override;
	bool operator()(char& value, const char* name, const char* label) override;

	bool operator()(yasli::i8& value, const char* name, const char* label) override;
	bool operator()(yasli::i16& value, const char* name, const char* label) override;
	bool operator()(yasli::i32& value, const char* name, const char* label) override;
	bool operator()(yasli::i64& value, const char* name, const char* label) override;

	bool operator()(yasli::u8& value, const char* name, const char* label) override;
	bool operator()(yasli::u16& value, const char* name, const char* label) override;
	bool operator()(yasli::u32& value, const char* name, const char* label) override;
	bool operator()(yasli::u64& value, const char* name, const char* label) override;

	bool operator()(float& value, const char* name, const char* label) override;
	bool operator()(double& value, const char* name, const char* label) override;

	bool operator()(const yasli::Serializer& ser, const char* name, const char* label) override;
	bool operator()(yasli::PointerInterface& ptr, const char *name, const char *label) override;
	bool operator()(yasli::ContainerInterface& ser, const char *name, const char *label) override;
	bool operator()(yasli::CallbackInterface& callback, const char* name, const char* label) override;
	bool operator()(yasli::Object& obj, const char *name, const char *label) override;
	using yasli::Archive::operator();

	bool openBlock(const char* name, const char* label) override;
	void closeBlock() override;
	void validatorMessage(bool error, const void* handle, const yasli::TypeID& type, const char* message) override;
	void documentLastField(const char* docString) override;

protected:
	PropertyOArchive(PropertyTreeModel* model, bool forDefaultType);

private:
	struct Level {
		int rowIndex;
		int realIndex;
		Level(int realIndex = 0) : rowIndex(0), realIndex(realIndex) {}
	};
	std::vector<Level> stack_;

	template<class RowType, class ValueType>
	PropertyRow* updateRowPrimitive(const char* name, const char* label, const char* typeName, const ValueType& value, const void* handle, const yasli::TypeID& typeId);

	template<class RowType, class ValueType>
	RowType* updateRow(const char* name, const char* label, const char* typeName, const ValueType& value, bool isBlock = false);

	void enterNode(PropertyRowStruct* row, bool isBlock = false); // sets currentNode
	void closeStruct(const char* name, bool isBlock = false);
	PropertyRow* defaultValueRootNode();

	bool updateMode_;
	bool defaultValueCreationMode_;
	PropertyTreeModel* model_;
	ValidatorBlock* validator_;
	SharedPtr<PropertyRowStruct> currentNode_;
	SharedPtr<PropertyRow> lastNode_;

	// for defaultArchive
	SharedPtr<PropertyRowStruct> rootNode_;
	yasli::string typeName_;
	const char* derivedTypeName_;
	yasli::string derivedTypeNameAlt_;
	bool outlineMode_;
};

// vim:ts=4 sw=4:
