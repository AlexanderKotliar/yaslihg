#pragma once

#include "yasli/Assert.h"
#include "yasli/STL.h"
#include "yasli/Archive.h"
#include "yasli/PointersImpl.h"
#include "yasli/STLImpl.h"
#include "yasli/TextOArchive.h"

#include "yasli/StringList.h" 
using namespace yasli;

#define YCHECK(x) ASSERT(x); CHECK(x)

struct Member
{
	std::string name;
	float weight;

	Member()
	: weight(0.0f) {}

	void checkEquality(const Member& copy) const
	{
		YCHECK(name == copy.name);
		YCHECK(weight == copy.weight);
	}

	void change(int index)
	{
		name = "Changed name ";
		name += (index % 10) + '0';
		weight = float(index);
	}

	void serialize(Archive& ar)
	{
		ar(name, "name");
		ar(weight, "weight");
	}
};

class PolyBase : public RefCounter
{
public:
	PolyBase()
	{
		baseMember_ = "Regular base member";
	}

	virtual void change()
	{
		baseMember_ = "Changed base member";
	}

	virtual void serialize(Archive& ar)
	{		
		ar(baseMember_, "baseMember");
	}

	virtual void checkEquality(const PolyBase* copy) const
	{
		YCHECK(baseMember_ == copy->baseMember_);
	}
protected:
	std::string baseMember_;
};

class PolyDerivedA : public PolyBase
{
public:
	void serialize(Archive& ar)
	{		
		PolyBase::serialize(ar);
		ar(derivedMember_, "derivedMember");
	}

	void checkEquality(const PolyBase* copyBase) const
	{
		const PolyDerivedA* copy = dynamic_cast<const PolyDerivedA*>(copyBase);
		YCHECK(copy != 0);
		YCHECK(derivedMember_ == copy->derivedMember_);

		PolyBase::checkEquality(copyBase);
	}
protected:
	std::string derivedMember_;
};

class PolyDerivedB : public PolyBase
{
public:
	PolyDerivedB()
	: derivedMember_("B Derived")
	{
	}

	void serialize(Archive& ar)
	{		
		PolyBase::serialize(ar);
		ar(derivedMember_, "derivedMember");
	}

	void checkEquality(const PolyBase* copyBase) const
	{
		const PolyDerivedB* copy = dynamic_cast<const PolyDerivedB*>(copyBase);
		YCHECK(copy != 0);
		YCHECK(derivedMember_ == copy->derivedMember_);

		PolyBase::checkEquality(copyBase);
	}
protected:
	std::string derivedMember_;
};

struct NumericTypes
{
  NumericTypes()
  : bool_(false)
  , char_(0)
  , sChar_(0)
  , uChar_(0)
  , int_(0)
  , uInt_(0)
  , long_(0)
  , uLong_(0)
  , longLong_(0)
  , uLongLong_(0)
  , float_(0.0f)
  , double_(0.0)
  {
  }

  void change()
  {
    bool_ = true;
    char_ = -1;
    sChar_ = -2;
    uChar_ = 0xff - 3;
    int_ = -4;
    uInt_ = -5;
    long_ = -6l;
    uLong_ = -7ul;
    longLong_ = -8ll;
    uLongLong_ = -9ull;
    float_ = -10.0f;
    double_ = -11.0;
  }

  void serialize(Archive& ar)
  {
    ar(bool_, "bool");
    ar(char_, "char");
    ar(sChar_, "signed_char");
    ar(uChar_, "unsigned_char");
    ar(int_, "int");
    ar(uInt_, "unsigned_int");
    ar(long_, "long");
    ar(uLong_, "unsigned_long");
    ar(longLong_, "long_long");
    ar(uLongLong_, "unsigned_long_long");
    ar(float_, "float");
    ar(double_, "double");
  }

  void checkEquality(const NumericTypes& rhs) const
  {
    YCHECK(bool_ == rhs.bool_);
    YCHECK(char_ == rhs.char_);
    YCHECK(sChar_ == rhs.sChar_);
    YCHECK(uChar_ == rhs.uChar_);
    YCHECK(int_ == rhs.int_);
    YCHECK(uInt_ == rhs.uInt_);
    YCHECK(long_ == rhs.long_);
    YCHECK(uLong_ == rhs.uLong_);
    YCHECK(longLong_ == rhs.longLong_);
    YCHECK(uLongLong_ == rhs.uLongLong_);
    YCHECK(float_ == rhs.float_);
    YCHECK(double_ == rhs.double_);
  }

  bool bool_;

  char char_;
  signed char sChar_;
  unsigned char uChar_;

	int int_;
	unsigned int uInt_;

  long long_;
  unsigned long uLong_;

  long long longLong_;
  unsigned long long uLongLong_;

  float float_;
  double double_;
};

class ComplexClass{
public:
	ComplexClass()
	: index_(0)
	{
		name_ = "Foo";
		stringList_.push_back("Choice 1");
		stringList_.push_back("Choice 2");
		stringList_.push_back("Choice 3");
		stringList_.push_back("Choice 4");
		stringList_.push_back("Choice 5");
		stringList_.push_back("Choice 6");
		stringList_.push_back("Choice 7");
		stringList_.push_back("Choice 8");

		polyPtr_.set( new PolyDerivedA() );

		polyVector_.push_back(new PolyDerivedB);
		polyVector_.push_back(new PolyBase);

		members_.resize(13);
	}

	void change()
	{
		name_ = "Slightly changed name";
		index_ = 2;
		polyPtr_.set( new PolyDerivedB() );
		polyPtr_->change();

		for (size_t i = 0; i < members_.size(); ++i)
			members_[i].change(i);

		for (size_t i = 0; i < polyVector_.size(); ++i)
			polyVector_[i]->change();

		polyVector_.resize(4);
		polyVector_.push_back(new PolyBase());
		polyVector_[4]->change();

		const size_t arrayLen = sizeof(array_) / sizeof(array_[0]);
		for (size_t i = 0; i < arrayLen; ++i)
			array_[i].change(arrayLen - i);

    numericTypes_.change();
	}

	void serialize(Archive& ar)
	{
		ar(name_, "name");
		ar(polyPtr_, "polyPtr");
		ar(polyVector_, "polyVector");
		ar(members_, "members");
		if(ar.isInPlace())
		{
			ar(index_, "index");
		}
		else
		{
			StringListValue value(stringList_, stringList_[index_]);
			ar(value, "stringList");
			index_ = value.index();
			if(index_ == -1)
				index_ = 0;
		}
		ar(array_, "array");
		ar(numericTypes_, "numericTypes");
	}

	void checkEquality(const ComplexClass& copy) const
	{
		YCHECK(name_ == copy.name_);
		YCHECK(index_ == copy.index_);

		YCHECK(polyPtr_ != 0);
		YCHECK(copy.polyPtr_ != 0);
		polyPtr_->checkEquality(copy.polyPtr_);

		YCHECK(members_.size() == copy.members_.size());
		for (size_t i = 0; i < members_.size(); ++i)
		{
			members_[i].checkEquality(copy.members_[i]);
		}

		YCHECK(polyVector_.size() == copy.polyVector_.size());
		for (size_t i = 0; i < polyVector_.size(); ++i)
		{
			if(polyVector_[i] == 0)
			{
				YCHECK(copy.polyVector_[i] == 0);
				continue;
			}
			YCHECK(copy.polyVector_[i] != 0);
			polyVector_[i]->checkEquality(copy.polyVector_[i]);
		}

		const size_t arrayLen = sizeof(array_) / sizeof(array_[0]);
		for (size_t i = 0; i < arrayLen; ++i)
		{
			array_[i].checkEquality(copy.array_[i]);
		}

    numericTypes_.checkEquality(copy.numericTypes_);
	}
protected:
	std::string name_;
	typedef std::vector<Member> Members;
	Members members_;
	int index_;
  NumericTypes numericTypes_;

	StringListStatic stringList_;
	std::vector< SharedPtr<PolyBase> > polyVector_;
	SharedPtr<PolyBase> polyPtr_;

	Member array_[5];
};

