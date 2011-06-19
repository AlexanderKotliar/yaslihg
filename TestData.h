#ifndef __TEST_DATA_H_INCLUDED__
#define __TEST_DATA_H_INCLUDED__

#include <string>
#include <vector>

//#include "xMath\xMath.h"

#include "ww/PropertyEditor.h"
#include "yasli/STL.h"
#include "yasli/Pointers.h"
#include "yasli/BitVector.h"
#include "yasli/Archive.h"
#include "yasli/BitVectorImpl.h"
#include "yasli/STLImpl.h"
#include "yasli/PointersImpl.h"
#include "yasli/TypesFactory.h"
#include "ww/Decorators.h"
#include "ww/SliderDecorator.h"
#include "XMath/Colors.h"
#include "ww/FileSelector.h"
#include "ww/KeyPress.h"

using namespace yasli;
using ww::NotDecorator;
using ww::HLineDecorator;
using ww::RadioDecorator;
using ww::FileSelector;
using ww::ButtonDecorator;

enum USELESS_ENUM {
    FIRST_VALUE,
    SECOND_VALUE,
    THIRD_VALUE,
    LAST_VALUE
};

enum USELESS_FLAGS {
	FIRST_FLAG  = 1 << 1,
	SECOND_FLAG = 1 << 2,
	THIRD_FLAG  = 1 << 3
};

class TestBase : public RefCounter{
public:
	std::string name;
	std::string base_member;
    float       base_float;
	bool		base_flag;
	bool		bool1;
	bool		bool2;
	bool		boolRight;

	TestBase () {
		name = "name";
        base_member = "New string";
        base_float = 3.1415926f;
		base_flag = false;
    }
	virtual ~TestBase () {}

	virtual void serialize (Archive& ar) {
		ar(name, "name", "^!!");
		ar(NotDecorator(base_flag), "base_flag", "^^");
		ar(bool1, "bool1", "^^L");
		ar(bool2, "bool2", "^^");
		ar(base_float, "base_float", "^Base Float");
		ar(boolRight, "boolRight", "^R");
		ar(base_member, "base_member", "^Base string");
		ar(base_member, "base_member", 0);
	}
};

class TestDerivedA : public TestBase
{
public:
	TestDerivedA (){
		derived_a_member = "";
	}
	virtual ~TestDerivedA () {}

	std::string derived_a_member;

	virtual void serialize (Archive& ar) {
		ar(derived_a_member, "derived_a_member", "&Строка в производном классе");
		__super::serialize (ar);
	}
};

class TestDerivedB : public TestBase
{
public:
	std::vector<SharedPtr <TestBase> > test_poly;

	std::string one_string;
	std::string second_string;

	virtual void serialize (Archive& ar) {
		__super::serialize (ar);
		
		ar(one_string, "one_string", "&One String");
		ar(second_string, "second_string", "&Second String");
	}
};

enum SwitcherType{
	SWITCHER_AND,
	SWITCHER_OR
};

class TestSwitcher : public TestBase{
public:
	TestSwitcher()
	: type(SWITCHER_AND)
	{
	}
	void serialize(Archive& ar){
		ar(type, "type", "^");
		__super::serialize(ar);
		ar(children, "children", "^[>100>]Children");
		ar(base, "base", "^");
	}
	SwitcherType type;
	std::vector<SharedPtr <TestBase> > children;
	TestBase base;
	
};

enum EnumType {
	ENUM_STRINGS,
	ENUM_INTEGERS,
	ENUM_FLOATS
};

struct S {
	int a, b;
	void serialize(Archive& ar){
		ar(a, "a", "a");
		ar(b, "b", "b");
	}
};


struct S0 {
	int a, b;
	S s;
	void serialize(Archive& ar){
		ar(a, "a", "a");
		ar(b, "b", "b");
		ar(s, "s", "s");
	}
};

struct S1 {
	int key;
	S0 s0;
	void serialize(Archive& ar){
		ar(key, "key", "^key");
		ar(s0, "s0", "^");
	}
};

struct TestData
{
	TestData () {
		enableSerialization_ = true;
		type = ENUM_FLOATS;
		labels.push_back ("First String");
		labels.push_back ("Second String");
		labels.push_back ("Last One");
		single_child = 0;
		comboList_ = StringListValue(labels, 0);
		float_value = 5.0000001e-002f;
	}

	StringListValue comboList_;
	bool mega_boolean;
	std::vector< SharedPtr<TestBase> > child;
	SharedPtr<TestBase> single_child;
    SharedPtr<TestBase> smart_child;
	typedef std::vector< SharedPtr<TestBase> > TestBases;
	TestBases poly_vector;
    std::string name;
    std::wstring description;
	Vect2i position;
	Vect2i size;
	BitVector<USELESS_FLAGS> flags;
    float fvalue;
	Color4c colors[4];
	std::string filename;
	S1 s1;
	S0 s0;

	std::list<TestData> childs;
	std::list<TestBase> childsBase;
	StringList labels;

	bool enableSerialization_;
	char   char_value;
    short  short_value;
    int    int_value;
    long   long_value;
    float  float_value;
    double double_value;

	EnumType type;
	ww::KeyPress hotkey;
	std::vector<Vect2f> vects;
	TestBase base;
	std::vector<Color4c> Colors;
	std::vector<int> ints;

	void serialize_(Archive& ar) {
		//ar(Colors, "colors", "Colors");
		//ar(ints, "ints", "Ints");
		//ar(enableSerialization_, "enableSerialization", "Show Simple Types");
		//ar(double_value, "double_value", "С плавающей запятой, двойной точности");
		//ar(Vect2f(0,0), "vect2", "Vect2");
		//ar(single_child, "single_child", "Condition");
		ar(s1, "s1", "s1");
		ar(s0, "s0", "s0");
	}

	void serialize(Archive& ar) {
		ar(s1, "s1", "s1");
		if(ar.openBlock("customEditors", "Custom Editors")){
			static FileSelector::Options options("*.ta", false, "");
			bool order = mega_boolean;
			ar(FileSelector(filename, options), "filename", "<");
			ar(colors, "colors", "Colors");
            ar(comboList_, "comboList", "^StringListValue");
			ar.closeBlock();
		}

		ar(HLineDecorator(), "hline1", 0);
		ar(vects, "vects", "[<[^>100>]]Vects");
		ar(single_child, "single_child", ">100>Condition");
		ar(HLineDecorator(), "hline2", "<");
		ButtonDecorator button("Open in PropertyEditor");
		ar(button, "button", "<");
		if(button)
			ww::edit(Serializer(*this), "testStateNested");

		ar(hotkey, "hotkey", "Hotkey");
		ar(base, "base", "Base");
		ar(enableSerialization_, "enableSerialization", "Show Simple Types");
		if(enableSerialization_){
			ar(flags, "flags", "Bit Flags");
			ar(type, "type", "Enum");

			std::list<TestData> childsTmp = childs;
			ar(childsTmp, "childsTmp", "~Temp Elements");
			ar(childs, "childs", "Children Elements");

			TestBases::iterator it;
			for(it = poly_vector.begin(); it != poly_vector.end(); ++it){
				if(TestBase* base = *it){
					ASSERT(base->refCount() == 1);
				}
			}

			ar(poly_vector, "poly_vector", "Vector with Polymorphic Items");
			ar(childsBase, "childsBase", "Simple Vector");
			ar(ww::SliderDecoratorf(fvalue, -15.0f, 15.0f, 0.5f), "fvalue", "Float Value");
			ar(mega_boolean, "mega_boolean", 0);
			ar(position, "position", "Position");
			ar(size, "size", "Size");

			std::vector<Color4f> comboList;
			comboList.push_back(Color4f(1.0f, 0.0f, 0.0f));
			comboList.push_back(Color4f(0.0f, 1.0f, 0.0f));
			comboList.push_back(Color4f(0.0f, 0.0f, 1.0f));

			if(ar.isInput() || !ar.isEdit() || type == ENUM_STRINGS) {
                ar(name, "name", "Name");
                ar(description, "description", "Description (wstring)");
            }

			if(ar.isInput() || !ar.isEdit() || type == ENUM_INTEGERS) {
				ar(char_value, "char_value", "Символ");
				ar(short_value, "short_value", "Короткое целое");
				ar(int_value, "int_value", "Целое");
				ar(long_value, "long_value", "!Длинное целое");
			}

			if(ar.isInput() || !ar.isEdit() || type == ENUM_FLOATS) {
                ar(float_value, "float_value", "С плавающей запятой");
                ar(double_value, "double_value", "С плавающей запятой, двойной точности");
            }
		}
	}
};

#endif
