#pragma once
#include "common/String.h"
#include <optional>

namespace Platform
{
	void SendIntent(ByteString action, std::optional<ByteString> data, std::optional<ByteString> extra, std::optional<ByteString> mimeType);
	std::optional<ByteString> CallActivityStringFunc(const char *funcName);
}
