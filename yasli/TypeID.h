/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once

#include <typeinfo>
#include "yasli/Config.h"
#include "yasli/Assert.h"
#include "yasli/Strings.h"
#include <string.h>

#if !YASLI_NO_RTTI
#ifndef _MSC_VER
using std::type_info;
#endif
#endif

namespace yasli{

class Archive;
struct TypeInfo;

class TypeID{
	friend class TypesFactory;
public:
	TypeID()
	: typeInfo_(0)
#if YASLI_NO_RTTI
	, runtimeID_(0)
#endif
	{}

	TypeID(const TypeID& original)
	: typeInfo_(original.typeInfo_)
#if !YASLI_NO_RTTI
	, name_(original.name_)
#else
	, runtimeID_(original.runtimeID_)
#endif
	{
	}

#if !YASLI_NO_RTTI
	explicit TypeID(const type_info& typeInfo)
	: typeInfo_(&typeInfo)
	{
	}
#endif

	operator bool() const{
		return *this != TypeID();
	}

	template<class T>
	static TypeID get();
	std::size_t sizeOf() const;
	const char* name() const;
	bool registered() const;

	bool operator==(const TypeID& rhs) const{
		if(typeInfo_ && rhs.typeInfo_)
			return typeInfo_ == rhs.typeInfo_;
		else
		{
			const char* name1 = name();
			const char* name2 = rhs.name();
			return strcmp(name1, name2) == 0;
		}
	}
	bool operator!=(const TypeID& rhs) const{
#if YASLI_NO_RTTI
		return runtimeID_ != rhs.runtimeID_;
#else
		return typeInfo_ != rhs.typeInfo_;
#endif
	}
	bool operator<(const TypeID& rhs) const{
#if YASLI_NO_RTTI
		return runtimeID_ < rhs.runtimeID_;
#else
		if(typeInfo_ && rhs.typeInfo_)
			return typeInfo_->before(*rhs.typeInfo_) > 0;
		else if(!typeInfo_)
			return rhs.typeInfo_!= 0;
		else if(!rhs.typeInfo_)
			return false;
		return false;
#endif
	}

#if YASLI_NO_RTTI
	const TypeInfo* typeInfo() const{ return typeInfo_; }
#else
	const type_info* typeInfo() const{ return typeInfo_; }
#endif
private:
#if YASLI_NO_RTTI 
	unsigned int runtimeID_;
	TypeInfo* typeInfo_;
    friend class bTypeInfo;
#else
	const type_info* typeInfo_;
	string name_;
#endif
	friend class TypeDescription;
    friend struct TypeInfo;
};

#if YASLI_NO_RTTI
struct TypeInfo
{
	TypeID id;
	size_t size;
	char name[128];

	template<size_t nameLen>
	static void extractTypeName(char (&name)[nameLen], const char* funcName)
	{
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 4
        // static yasli::TypeID yasli::TypeID::get() [with T = ActualTypeName]
        const char* s = strstr(funcName, "[with T = ");
        if (s)
            s += 9;
        const char* send = strrchr(funcName, ']');
#else
        // static yasli::TypeID yasli::TypeID::get<ActualTypeName>()
        const char* s = strchr(funcName, '<');
		const char* send = strrchr(funcName, '>');
#endif
		YASLI_ASSERT(s != 0  && send != 0);
		if (s != send)
			++s;
		if(strncmp(s, "class ", 6) == 0)
			s += 6;
		else if(strncmp(s, "struct ", 7) == 0)
			s += 7;

		char* d = name;
		const char* dend = name + sizeof(name) - 1;
		while (d != dend && s != send)
		{
			*d = *s;
			++s;
			++d;
		}
		*d = '\0';
        fprintf(stderr, "%s\n", name);
		YASLI_ASSERT(s == send && "Type name doesn't fit into the buffer");
	}

	static unsigned int generateTypeID()
	{
		static unsigned int counter = 0;
		return ++counter;
	}

	TypeInfo(size_t size, const char* templatedFunctionName)
	: size(size)
	{
		id.runtimeID_ = generateTypeID();
		extractTypeName(name, templatedFunctionName);
		id.typeInfo_ = this;
	}
};
#endif

template<class T>
TypeID TypeID::get()
{
#if YASLI_NO_RTTI
# ifdef _MSC_VER
  static TypeInfo typeInfo(sizeof(T), __FUNCSIG__);
# else
  static TypeInfo typeInfo(sizeof(T), __PRETTY_FUNCTION__);
# endif
  return typeInfo.id;
#else
  static TypeID result(typeid(T));
  return result;
#endif
}

template<class T>
T* createDerivedClass(TypeID typeID);

}

