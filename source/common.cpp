#include "common.h"

char *readWholeFile(const char *filename, size_t *outSize)
{
	FILE *f = fopen(filename, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		long size = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *contents = (char *)malloc(size + 1);
		if (contents != NULL)
		{
			fread(contents, 1, size, f);
			contents[size] = 0;
		}
		else
			size = 0;

		if (outSize)
			*outSize = (size_t)size;

		fclose(f);
		return contents;
	}
	else
	{
		if (outSize)
			*outSize = 0;
		return NULL;
	}
}

size_t hashBytes(const void *bytes, size_t numBytes)
{
	// FNV 1a: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

	uint64_t hash = 14695981039346656037llu;
	const uint8_t *dat = (const uint8_t *)bytes;

	for (size_t i = 0; i < numBytes; ++i)
	{
		hash ^= dat[i];
		hash *= 1099511628211llu;
	}

	return hash;
}