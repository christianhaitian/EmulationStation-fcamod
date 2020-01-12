#pragma once
#ifndef ES_CORE_IMAGE_IO
#define ES_CORE_IMAGE_IO

#include <stdlib.h>
#include <vector>

#include "math/Vector2i.h"
#include "math/Vector2f.h"

class ImageIO
{
public:
	static std::vector<unsigned char> loadFromMemoryRGBA32(const unsigned char * data, const size_t size, size_t & width, size_t & height);
	static unsigned char* loadFromMemoryRGBA32Ex(const unsigned char * data, const size_t size, size_t & width, size_t & height, int maxWidth, int maxHeight, bool externZoom, Vector2i& baseSize, Vector2i& packedSize);
	
	static bool getImageSize(const char *fn, unsigned int *x, unsigned int *y);

	static void flipPixelsVert(unsigned char* imagePx, const size_t& width, const size_t& height);
	static Vector2i adjustPictureSize(Vector2i imageSize, Vector2i maxSize, bool externSize = false);
	static Vector2f adjustExternPictureSizef(Vector2f imageSize, Vector2f maxSize);
	static Vector2f adjustPictureSizef(Vector2f imageSize, Vector2f maxSize);

	static void loadImageCache();
	static void saveImageCache();

	static void updateImageCache(const std::string fn, int sz, int x, int y);
};

#endif // ES_CORE_IMAGE_IO
