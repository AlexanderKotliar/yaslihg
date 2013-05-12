#pragma once
#include "yasli/decorators/BitFlags.h"
#include "yasli/Archive.h"

namespace yasli {

void BitFlagsWrapper::serialize(yasli::Archive& ar)
{
	const yasli::EnumDescription& desc = *description;
	int count = desc.count();
	if (ar.isInput()) {
		//*variable = 0;
		int previousValue = *variable;
		for (size_t i = 0; i < count; ++i) {
			int flagValue = desc.valueByIndex(i);
			bool flag = (previousValue & flagValue) == flagValue;
			bool previousFlag = flag;
			ar(flag, desc.nameByIndex(i), desc.labelByIndex(i));
			if (flag != previousFlag)
			{
				if (flag)
					*variable |= flagValue;
				else
					*variable &= ~flagValue;
			}
		}
	}
	else {
		for (size_t i = 0; i < count; ++i) {
			int flagValue = desc.valueByIndex(i);
			bool flag = (*variable & flagValue) == flagValue;
			ar(flag, desc.nameByIndex(i), desc.labelByIndex(i));
		}
	}
}

}