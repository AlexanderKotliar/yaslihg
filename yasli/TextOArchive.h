#pragma once

#include <fstream>
#include "yasli/Archive.h"
#include "yasli/Pointers.h"

class MemoryWriter;

class YASLI_API TextOArchive : public Archive{
public:
    // header = 0 - default header, use "" to omit
    TextOArchive(int textWidth = 80, const char* header = 0);
    ~TextOArchive();

    void open(const char* fileName);
    void close();

    const char* c_str() const;    
    size_t size() const;    

    // from Archive:
    bool operator()(bool& value, const char* name = "", const char* label = 0);
    bool operator()(std::string& value, const char* name = "", const char* label = 0);
    bool operator()(float& value, const char* name = "", const char* label = 0);
    bool operator()(double& value, const char* name = "", const char* label = 0);
    bool operator()(int& value, const char* name = "", const char* label = 0);
    bool operator()(unsigned int& value, const char* name = "", const char* label = 0);
    bool operator()(__int64& value, const char* name = "", const char* label = 0);

    bool operator()(char& value, const char* name = "", const char* label = 0);
    bool operator()(signed char& value, const char* name = "", const char* label = 0);
    bool operator()(unsigned char& value, const char* name = "", const char* label = 0);

    bool operator()(const Serializer& ser, const char* name = "", const char* label = 0);
    bool operator()(const ContainerSerializationInterface& ser, const char* name = "", const char* label = 0);
    // ^^^
	
    bool operator()(StringListStaticValue& value, const char* name = "", const char* label = 0);
    
	using Archive::operator();
private:
    void openBracket();
    void closeBracket();
    void openContainerBracket();
    void closeContainerBracket();
    void placeName(const char* name);
    void placeIndent();

    bool joinLinesIfPossible();

    struct Level{
        Level(bool _isContainer, std::size_t position, int column)
        : isContainer(_isContainer)
        , startPosition(position)
        , indentCount(-column)
        {}
        bool isContainer;
        std::size_t startPosition;
        int indentCount;
    };

    typedef std::vector<Level> Stack;
    Stack stack_;
    SharedPtr<MemoryWriter> buffer_;
    const char* header_;
    int textWidth_;
	std::string fileName_;
	bool open_;
};
