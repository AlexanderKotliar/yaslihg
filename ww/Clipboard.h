#pragma once

#include "ww/ConstStringList.h"
namespace yasli{
class Serializer;
}

namespace ww{
struct PasteFunc{
    virtual bool operator()(const char* mem, size_t size) = 0;
};
class Widget;
class PropertyRow;
class PropertyTreeModel;
class Clipboard{
public:
	Clipboard(Widget* owner, ConstStringList* constStrings = 0, PropertyTreeModel* model = 0, int filter = 0);
	~Clipboard();

	bool empty();
	bool copy(PropertyRow* row);
	bool copy(Serializer& se);
	bool paste(PropertyRow* row, bool onlyCheck = false);
	bool paste(Serializer& se);
	bool canBePastedOn(const char* destinationType);
	int smartPaste(Serializer& se);
protected:
	bool pasteFunc(PasteFunc& func);

	Widget* widget_;
	PropertyTreeModel* model_;
	ConstStringList* constStrings_;
	ConstStringList ownConstStrings_;
	unsigned int clipboardFormat_;
    int filter_;
};

};

