#pragma once

#include <fstream>
#include "yasli/Archive.h"
#include "utils/Pointers.h"

class MemoryWriter;

class YASLI_API TextOArchive : public Archive{
public:
    // header = 0 - default header, use "" to omit
    TextOArchive(int textWidth = 80, const char* header = 0);
    ~TextOArchive();

    void open(const char* fileName);
    void close();

    const char* c_str() const;

    

    // from Archive:
    bool operator()(bool& value, const char* name = "");
    bool operator()(std::string& value, const char* name = "");
    bool operator()(float& value, const char* name = "");
    bool operator()(int& value, const char* name = "");
    bool operator()(StringListStaticValue& value, const char* name = "");

    bool operator()(char& value, const char* name = "");
    bool operator()(signed char& value, const char* name = "");
    bool operator()(unsigned char& value, const char* name = "");

    bool operator()(const Serializer& ser, const char* name = "");
    bool operator()(const ContainerSerializer& ser, const char* name = "");
    // ^^^
    
    template<class T>
    bool operator()(T& value, const char* name = ""){
        return Archive::operator()(value, name);
    }
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
