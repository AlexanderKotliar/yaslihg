#pragma once

#include <vector>
#include <list>

#include "yasli/Serializer.h"

class Archive;

template<class T, class Alloc>
bool serialize(Archive& ar, std::vector<T, Alloc>& container, const char* name, const char* label);

template<class T, class Alloc>
bool serialize(Archive& ar, std::list<T, Alloc>& container, const char* name, const char* label);
