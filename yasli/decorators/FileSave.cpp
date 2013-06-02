#include "yasli/decorators/FileSave.h"
#include "yasli/STL.h"
#include "yasli/Archive.h"
#include "yasli/STLImpl.h"

namespace yasli {

void FileSave::serialize(Archive& ar)
{
	ar(path, "path");
	ar(filter, "filter");
	ar(relativeToFolder, "folder");
}

}

bool serialize(yasli::Archive& ar, yasli::FileSave& value, const char* name, const char* label)
{
	if (ar.isEdit())
		return ar(yasli::Serializer(value), name, label);
	else
		return ar(value.path, name, label);
}
