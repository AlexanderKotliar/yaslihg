#pragma once

#include "ww/Splitter.h"

namespace ww{

class SplitterImpl;
class WW_API VSplitter : public Splitter{
public:
	VSplitter(int splitterSpacing = 1, int border = 0);
	~VSplitter();

protected:
	VSplitter(int splitterSpacing, int border, SplitterImpl* impl);
	bool vertical() { return true; }

	int boxLength() const;
	Vect2 elementSize(Widget* widget) const;
	Rect rectByPosition(int start, int end);
	void setSplitterMinimalSize(const Vect2& size);
	int positionFromPoint(const Vect2 point) const;
};

}

