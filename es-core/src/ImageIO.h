#pragma once
#ifndef ES_CORE_IMAGE_IO
#define ES_CORE_IMAGE_IO

#include <stdlib.h>
#include <vector>

#include "math/Vector2i.h"

class ImageIO
{
public:
	static std::vector<unsigned char> loadFromMemoryRGBA32(const unsigned char * data, const size_t size, size_t & width, size_t & height);
	static unsigned char* loadFromMemoryRGBA32Ex(const unsigned char * data, const size_t size, size_t & width, size_t & height, int maxWidth=0, int maxHeight=0);
	
	static void flipPixelsVert(unsigned char* imagePx, const size_t& width, const size_t& height);
	static Vector2i adjustPictureSize(Vector2i imageSize, Vector2i maxSize);
};

#endif // ES_CORE_IMAGE_IO
