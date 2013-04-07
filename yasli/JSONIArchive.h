/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once

#include "Pointers.h"
#include "yasli/Archive.h"
#include "Token.h"
#include <memory>

namespace yasli{

class MemoryReader;

class JSONIArchive : public Archive{
public:
	JSONIArchive();
	~JSONIArchive();

	bool load(const char* filename);
	bool open(const char* buffer, size_t length, bool free = false);

	// virtuals:
	bool operator()(bool& value, const char* name = "", const char* label = 0);
	bool operator()(StringInterface& value, const char* name = "", const char* label = 0);
	bool operator()(WStringInterface& value, const char* name = "", const char* label = 0);
	bool operator()(float& value, const char* name = "", const char* label = 0);
	bool operator()(double& value, const char* name = "", const char* label = 0);
	bool operator()(short& value, const char* name = "", const char* label = 0);
	bool operator()(unsigned short& value, const char* name = "", const char* label = 0);
	bool operator()(int& value, const char* name = "", const char* label = 0);
	bool operator()(unsigned int& value, const char* name = "", const char* label = 0);
	bool operator()(long long& value, const char* name = "", const char* label = 0);
	bool operator()(unsigned long long& value, const char* name = "", const char* label = 0);

	bool operator()(signed char& value, const char* name = "", const char* label = 0);
	bool operator()(unsigned char& value, const char* name = "", const char* label = 0);
	bool operator()(char& value, const char* name = "", const char* label = 0);

	bool operator()(const Serializer& ser, const char* name = "", const char* label = 0);
	bool operator()(ContainerInterface& ser, const char* name = "", const char* label = 0);
	bool operator()(KeyValueInterface& ser, const char* name = "", const char* label = 0);
	bool operator()(PointerInterface& ser, const char* name = "", const char* label = 0);

	using Archive::operator();
private:
	bool findName(const char* name, Token* outName = 0);
	bool openBracket();
	bool closeBracket();

	bool openContainerBracket();
	bool closeContainerBracket();

	void checkValueToken();
	bool checkStringValueToken();
	void readToken();
	void putToken();
	int line(const char* position) const; 
	bool isName(Token token) const;

	void expect(char token);
	void skipBlock();

	struct Level{
		const char* start;
		const char* firstToken;
		bool isContainer;
		bool isKeyValue;
		Level() : isContainer(false), isKeyValue(false) {}
	};
	typedef std::vector<Level> Stack;
	Stack stack_;

	std::auto_ptr<MemoryReader> reader_;
	Token token_;
	std::string filename_;
};

}
