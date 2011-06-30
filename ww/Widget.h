#pragma once

#include "ww/API.h"
#include "ww/MouseButton.h"
#include "ww/sigslot.h"
#include "ww/Vect2.h"
#include "ww/Rect.h"

#include "yasli/Pointers.h"

namespace Win32{
	class Window32;
};	

namespace ww{

enum{
	SERIALIZE_DESIGN = 1 << 0,
	SERIALIZE_STATE  = 1 << 1
};


class HotkeyContext;
class Container;
class Tooltip;

enum FocusDirection{
	FOCUS_FIRST,
	FOCUS_LAST,

	FOCUS_PREVIOUS,
	FOCUS_NEXT,

	FOCUS_UP,
	FOCUS_DOWN,
	FOCUS_LEFT,
	FOCUS_RIGHT
};

class Widget;

struct WidgetVisitor{
	virtual void operator()(Widget& widget) = 0;
};

class WW_API Widget : public sigslot::has_slots, public PolyRefCounter{
public:
	Widget();
	virtual ~Widget();
	virtual bool isVisible() const = 0;

	void setVisibility(bool visible);
	virtual void show();
	virtual void showAll();
	virtual void hide();

	virtual void visitChildren(WidgetVisitor& visitor) const{};

	/// �� �������: enabled/active
	virtual void setSensitive(bool sensitive){ sensitive_ = sensitive; }
	/// �� �������: enabled/active
	bool sensitive() const{ return sensitive_; }

	/// ������� ������ ��������, � ��������
	virtual void setBorder(int border);
	int border() const{ return border_; }
	/// ���������������� ����������� ������ � ��������
	Vect2 requestSize() const{ return requestSize_; }
	void setRequestSize(int w, int h);
	virtual void setRequestSize(const Vect2 size);

	virtual void passFocus(FocusDirection direction);
	void setFocus();

	/// ������ � ������������� ����������
	Container* parent() const{ return parent_; }

	sigslot::signal0& signalDelete() { return signalDelete_; };

	virtual void serialize(Archive& ar);

	// �����: ��� ������, ������������ � underscore('_') �� ������ ����������
	// �������������� ���������� � � ������ ��������������� � ����������� �� 
	// Widget � Container ������� ������ ���������� � ������ protected ��� private

	/// ����������� ������� ������������ ���������
    virtual void _setParent(Container* container);

	const Rect& _position() const{ return position_; }
	virtual void _setPosition(const Rect& position);
	void _relayoutParents(bool propagate);
	virtual void _relayoutParents();
	virtual void _queueRelayout();

	const Vect2 _minimalSize() const;
	void _setMinimalSize(int x, int y);
	virtual void _setMinimalSize(const Vect2& size);

	virtual Win32::Window32* _window() const{ return 0; }
    
	virtual Widget* _focusedWidget();
	virtual void _setFocusedWidget(Widget* widget);

	virtual Widget* _nextWidget(Widget* last, FocusDirection direction) const;
	Widget* _nextFocusableWidget(Widget* last, FocusDirection direction) const;
	virtual void _setFocus();
	virtual bool _focusable() const{ return sensitive() && _visible() && _visibleInLayout(); }

	virtual bool canBeDefault() const { return false; }
	virtual void setDefaultFrame(bool enable) {}

	virtual void _updateVisibility();
	virtual void _setVisibleInLayout(bool visibleInLayout);
	bool _visibleInLayout() const{ return visibleInLayout_; }
	bool _visible() const{ return visible_; }

	virtual HotkeyContext* _hotkeyContext();
	
	void setToolTip(Tooltip* toolTip) { toolTip_ = toolTip; }
	Tooltip* toolTip() const { return toolTip_; }

protected:
	/// ���������� �� ����������
	sigslot::signal0 signalDelete_;

	/// ��������� �������� � ������������ ����������, � �������� (�� ������ � ������������ � ����)
	Rect position_;
	/// ������������ ���������, ��������� ���������
    Container* parent_;
	/// ����������-��������� ������
	Vect2 minimalSize_;
	/// ����������� ������ ������������� �������������
	Vect2 requestSize_;
	/// ����� ������ ��������, � ��������
	int border_;

	// * ��������� � Container?
	/// ����� ������ ��������, � ��������
	int focusedChildIndex_;
	// * ^^^

	/// ����� �� ����������� ��������� ����� ������������
	bool relayoutQueued_;
	/// �-�� enabled
	bool sensitive_;
	/// ����� �� ������� (� ������ ���� ����� ��� ������������ �������)
	bool visible_;
	/// ��������� ���������� ����� ������� �������� (�������� Splitter �����������)
	bool visibleInLayout_;

	Tooltip* toolTip_;

	static Widget* focusedWidget_;
};

Win32::Window32* _findWindow(const Widget* widget);

};

