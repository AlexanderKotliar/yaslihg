#pragma once

#include "ww/Container.h"

namespace Win32{
    class Window;
};

namespace ww{

class SplitterImpl;

class WW_API Splitter: public Container{
public:
    Splitter(int splitterSize, int border, SplitterImpl* impl);
	~Splitter();
	static const int SPLITTER_WIDTH = 6;

    /// ������� ��� �������� �������
    void clear();
    /// �������� ���������� ������ (���� ��� ���������)
    void resize(int newSize);
    /// �������� ����� ��������� beforeIndex (-1 = �������� � �����) 
    void add(Widget* widget, float position = 0.5f, bool fold = true, int beforeIndex = -1);
    /// ������� ������� �� �������
    void remove(int index, bool inFavourOfPrevious);
	/// �������� �������
	void replace(Widget* oldWidget, Widget* newWidget);
	/// ���������� ��������� ���������
	void setSplitterPosition(float position, int splitterIndex = 0);
    void setFlat(bool flat){ flat_ = flat; }

	int splitterSpacing() const{ return splitterSpacing_; }
	float widgetPosition(Widget* widget) const;
	Widget* widgetByPosition(float position);
	Widget* widgetByScreenPoint(Vect2i point);
	Widget* widgetByIndex(int index);
	virtual bool vertical() = 0;

    // virtuals:
	void visitChildren(WidgetVisitor& visitor) const;
	void serialize(Archive& ar);
    // ^^

	void _updateVisibility();
	void _setParent(Container* container);
	void _setPosition(const Rect& position);
	void _arrangeChildren();
	void _relayoutParents();

	int splittersCount();
	Rect getSplitterRect(int splitterIndex);
protected:
	friend SplitterImpl;

	Win32::Window32* _window() const{ return window_; }

	void setPanePosition(int index, int poisitionInPixels);
	virtual int boxLength() const = 0;
	virtual Vect2i elementSize(Widget* widget) const = 0;
	virtual void setSplitterMinimalSize(const Vect2i& size) = 0;
	virtual Rect rectByPosition(int start, int end) = 0;
	virtual int positionFromPoint(const Vect2i point) const = 0;

	int splitterWidth() const;


	struct Element{
		Element()
		: position(0.0f)
		, snappedPosition(0.0f)
		, fold(false)
		{
		}
		void serialize(Archive& ar);
		SharedPtr<ww::Widget> widget;
		Rect splitterRect;
		float position;
		float snappedPosition;
		bool fold;
	};

	typedef std::list<Element> Elements;
	Elements elements_;
    Win32::Window32* window_;
    bool flat_;
	int splitterSpacing_;
};

}

