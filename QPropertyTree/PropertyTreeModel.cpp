/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include "PropertyTreeModel.h"
#include "QPropertyTree.h"
#include "Serialization.h"
#include "yasli/ClassFactory.h"

PropertyTreeModel::PropertyTreeModel()
: expandLevels_(0)
, undoEnabled_(true)
, fullUndo_(false)
{
	clear();
}

PropertyTreeModel::~PropertyTreeModel()
{
	clearObjectReferences();
	rootObject_ = Object();
	root_ = 0;
	defaultTypes_.clear();
	defaultTypesPoly_.clear();
}

void PropertyTreeModel::clearObjectReferences()
{
	ModelObjectReferences::iterator it = objectReferences_.begin();
	for (; it != objectReferences_.end(); ++it) 
		it->second.row->setModel(0);
	objectReferences_.clear();
}


TreePath PropertyTreeModel::pathFromRow(PropertyRow* row)
{
	TreePath result;
	if(row)
		while(row->parent()){
            int childIndex = row->parent()->childIndex(row);
            YASLI_ESCAPE(childIndex >= 0, return TreePath());
			result.insert(result.begin(), childIndex);
			row = row->parent();
		}
		return result;
}

void PropertyTreeModel::selectRow(PropertyRow* row, bool select, bool exclusive)
{
	if(exclusive)
		deselectAll();

	row->setSelected(select);

	Selection::iterator it = std::find(selection_.begin(), selection_.end(), pathFromRow(row));
	if(select){
		if(it == selection_.end())
			selection_.push_back(pathFromRow(row));
		setFocusedRow(row);
	}
	else if(it != selection_.end()){
		PropertyRow* it_row = rowFromPath(*it);
		YASLI_ASSERT(it_row->refCount() > 0 && it_row->refCount() < 0xFFFF);
		selection_.erase(it);
	}
}

void PropertyTreeModel::deselectAll()
{
	Selection::iterator it;
	for(it = selection_.begin(); it != selection_.end(); ++it){
		PropertyRow* row = rowFromPath(*it);
		row->setSelected(false);
	}
	selection_.clear();
}

PropertyRow* PropertyTreeModel::rowFromPath(const TreePath& path)
{
	PropertyRow* row = root();
	if (!root())
		return 0;
	TreePath::const_iterator it;
	for(it = path.begin(); it != path.end(); ++it){
		int index = it->index;
		if(index < int(row->count()) && index >= 0){
			PropertyRow* nextRow = row->childByIndex(index);
			if(!nextRow)
				return row;
			else
				row = nextRow;
		}
		else
			return row;
	}
	return row;
}

void PropertyTreeModel::setSelection(const Selection& selection)
{
	deselectAll();
	Selection::const_iterator it;
	for(it = selection.begin(); it != selection.end(); ++it){
		const TreePath& path = *it;
		PropertyRow* row = rowFromPath(path);
		if(row)
			selectRow(row, true, false);
	}
}

void PropertyTreeModel::clear()
{
	if(root_)
		root_->clear();
	root_ = 0;
	setRoot(new PropertyRow("", "root", ""));
	selection_.clear();
}

void PropertyTreeModel::onUpdated(const PropertyRows& rows)
{
    signalUpdated(rows);
}

void PropertyTreeModel::applyOperator(PropertyTreeOperator* op, bool createRedo)
{
    YASLI_ESCAPE(op, return);
    PropertyRow *dest = rowFromPath(op->path_);
    YASLI_ESCAPE(dest && "Unable to apply operator!", return);
    if(op->type_ == PropertyTreeOperator::NONE)
        return;
    YASLI_ESCAPE(op->row_, return);
    if(dest->parent())
        dest->parent()->replaceAndPreserveState(dest, op->row_, false);
    else{
        op->row_->assignRowProperties(root_);
        root_ = op->row_;
    }
    PropertyRow* newRow = op->row_;
    op->row_ = 0;
    rowChanged(newRow);
    // TODO: redo
}

void PropertyTreeModel::undo()
{
    YASLI_ESCAPE(!undoOperators_.empty(), return);
    applyOperator(&undoOperators_.back(), true);
    undoOperators_.pop_back();
}

PropertyTreeModel::UpdateLock PropertyTreeModel::lockUpdate()
{
	if(updateLock_)
		return updateLock_;
	else {
		UpdateLock lock = new PropertyTreeModel::LockedUpdate(this);;
		updateLock_ = lock;
		lock->release();
		return lock;
	}
}

void PropertyTreeModel::dismissUpdate()
{
	if(updateLock_)
		updateLock_->dismissUpdate();
}

void PropertyTreeModel::requestUpdate(const PropertyRows& rows)
{
	if(updateLock_)
		updateLock_->requestUpdate(rows);
	else
		onUpdated(rows);
}

struct RowObtainer {
	RowObtainer(std::vector<char>& states) : states_(states) {}
	ScanResult operator()(PropertyRow* row)
	{
		states_.push_back(row->expanded() ? 1 : 0);
		return row->expanded() ? SCAN_CHILDREN_SIBLINGS : SCAN_SIBLINGS;
	}
protected:
	std::vector<char>& states_;
};

struct RowExpander {
	RowExpander(const std::vector<char>& states) : states_(states), index_(0) {}
	ScanResult operator()(PropertyRow* row, QPropertyTree* tree, int index)
	{
		if(size_t(index_) >= states_.size())
			return SCAN_FINISHED;

		if(states_[index_++]){
			if(row->canBeToggled(tree))
				row->_setExpanded(true);
			return SCAN_CHILDREN_SIBLINGS;
		}
		else{
			row->_setExpanded(false);
			return SCAN_SIBLINGS;
		}
	}
protected:
	int index_;
	const std::vector<char>& states_;
};

void PropertyTreeModel::serialize(Archive& ar, QPropertyTree* tree)
{
	if(ar.filter(SERIALIZE_STATE)){
		ar(focusedRow_, "focusedRow", 0);		
		ar(selection_, "selection", 0);

		if (root()) {
		std::vector<char> expanded;
        if(ar.isOutput()) {
            RowObtainer op(expanded);
            root()->scanChildren(op);
        }
		ar(expanded, "expanded", 0);
			if(ar.isInput()){
			Selection sel = selection_;
            setSelection(sel);
            RowExpander op(expanded);
            root()->scanChildren(op, tree);
		}
	}
	}
}

void PropertyTreeModel::pushUndo(const PropertyTreeOperator& op)
{
    PropertyTreeOperator oper = op;
    bool handled = false;
    signalPushUndo(&oper, &handled);
    if(!handled && oper.row_ != 0)
        undoOperators_.push_back(oper);
}

void PropertyTreeModel::push(PropertyRow* row)
{
    YASLI_ESCAPE(row, return);
    if(fullUndo_){
        if(undoEnabled_){
            PropertyRow* clonedRow = root()->clone();
            clonedRow->assignRowState(*root(), true);
            pushUndo(PropertyTreeOperator(TreePath(), clonedRow));
        }
        else{
            pushUndo(PropertyTreeOperator(TreePath(), 0));
        }
    }
    else{
        if(undoEnabled_){
            PropertyRow* clonedRow = row->clone();
            clonedRow->assignRowState(*row, true);
            pushUndo(PropertyTreeOperator(pathFromRow(row), clonedRow));
        }
        else{
            pushUndo(PropertyTreeOperator(pathFromRow(row), 0));
        }
    }
}

void PropertyTreeModel::rowChanged(PropertyRow* row)
{
	YASLI_ESCAPE(row, return);

	PropertyRow* parentObj = row;
	while (parentObj->parent() && !parentObj->isObject())
		parentObj = parentObj->parent();

	row->setMultiValue(false);
	while (row->parent()) {
		row->setLabelChanged();
		row = row->parent();
	}

	PropertyRows rows;
	rows.push_back(parentObj);
	requestUpdate(rows);
}

bool PropertyTreeModel::defaultTypeRegistered(const char* typeName) const
{
	return defaultTypes_.find(typeName) != defaultTypes_.end();
}

void PropertyTreeModel::addDefaultType(PropertyRow* row, const char* typeName)
{
	YASLI_ESCAPE(typeName != 0, return);
	defaultTypes_[typeName] = row;
}

PropertyRow* PropertyTreeModel::defaultType(const char* typeName) const
{
	DefaultTypes::const_iterator it = defaultTypes_.find(typeName);
	YASLI_ESCAPE(it != defaultTypes_.end(), return 0);
	return it->second;
}

void PropertyTreeModel::addDefaultType(const TypeID& type, const PropertyDefaultTypeValue& value)
{
	YASLI_ASSERT(type != TypeID());

	BaseClass& base = defaultTypesPoly_[type];
	for (DerivedTypes::iterator it = base.types.begin(); it != base.types.end(); ++it){
		if (it->type == value.type) {
			YASLI_ASSERT(it->root == 0);
			*it = value;
			return;
		}
	}

	base.types.push_back(value);
	base.strings.push_back(value.label.c_str());
}

const PropertyDefaultTypeValue* PropertyTreeModel::defaultType(const TypeID& baseType, int derivedIndex) const
{
	DefaultTypesPoly::const_iterator it = defaultTypesPoly_.find(baseType);
	YASLI_ESCAPE(it != defaultTypesPoly_.end(), return 0);
	const BaseClass& base = it->second;
	YASLI_ESCAPE(size_t(derivedIndex) < base.types.size(), return 0);
	return &base.types[derivedIndex];
}

bool PropertyTreeModel::defaultTypeRegistered(const TypeID& baseType, const TypeID& derivedType) const
{
	DefaultTypesPoly::const_iterator it = defaultTypesPoly_.find(baseType);

	if (it == defaultTypesPoly_.end())
		return false;

	const BaseClass& base = it->second;
	DerivedTypes::const_iterator dit;
	for (dit = base.types.begin(); dit != base.types.end(); ++dit){
		if (dit->type == derivedType)
			return true;
	}
	return false;
}

const yasli::StringList& PropertyTreeModel::typeStringList(const yasli::TypeID& baseType) const
{
	DefaultTypesPoly::const_iterator it = defaultTypesPoly_.find(baseType);

	static yasli::StringList empty;
	YASLI_ESCAPE(it != defaultTypesPoly_.end(), return empty);
	const BaseClass& base = it->second;
	return base.strings;
}

void PropertyTreeModel::registerObjectRow(PropertyRowObject* row)
{
	ModelObjectReferences::iterator it;
	void* address = row->object().address();
	YASLI_ESCAPE(address != 0, return);
	it = objectReferences_.find(address);
	if (it == objectReferences_.end())
	{
		ModelObjectReference ref;
		ref.row = row;
		ref.needUpdate = true;
		objectReferences_.insert(std::make_pair(row->object().address(), ref));
	}
	else
		it->second.row = row;
}

void PropertyTreeModel::unregisterObjectRow(PropertyRowObject* row)
{
	ModelObjectReferences::iterator it;
	it = objectReferences_.find(row->object().address());
	if(it == objectReferences_.end())
		return;
	if (it->second.row != row)
		return;
	row->setModel(0);
	objectReferences_.erase(it);
}

void PropertyTreeModel::setRootObject(const Object& obj)
{
	if (rootObject_.address() != obj.address()) {
		rootObject_ = obj;
		root_ = 0;

		clearObjectReferences();

		PropertyRowObject* root = new PropertyRowObject("", "", obj, this);

		ModelObjectReference ref;
		root_ = root;
		ref.row = root;
		ref.needUpdate = true;
		objectReferences_[obj.address()] = ref;
	}
	else {
		ModelObjectReferences::iterator it = objectReferences_.find(obj.address());
		if (it != objectReferences_.end())
			it->second.needUpdate = true;
	}

	rootObject_ = obj;
}

// ----------------------------------------------------------------------------------

void TreePathLeaf::serialize(Archive& ar)
{
	ar(index, "", 0);
}

bool serialize(yasli::Archive& ar, TreeSelection& value, const char* name, const char* label)
{
	return ar(static_cast<std::vector<TreePath>&>(value), name, label);
}
