#include "Platform.h"

namespace Platform
{
void OpenURI(ByteString uri)
{
	fprintf(stderr, "cannot open URI: not implemented\n");
}

void ShareText(ByteString text)
{
	fprintf(stderr, "cannot share text: not implemented\n");
}

void ShareFile(ByteString path)
{
	fprintf(stderr, "cannot share file: not implemented\n");
}

bool CanUpdate()
{
	return false;
}

void SetupCrt()
{
}
}
