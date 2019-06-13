#include "ImageIO.h"

#include "Log.h"
#include <FreeImage.h>
#include <string.h>

std::vector<unsigned char> ImageIO::loadFromMemoryRGBA32(const unsigned char * data, const size_t size, size_t & width, size_t & height)
{
	std::vector<unsigned char> rawData;
	width = 0;
	height = 0;
	FIMEMORY * fiMemory = FreeImage_OpenMemory((BYTE *)data, (DWORD)size);
	if (fiMemory != nullptr) 
	{
		//detect the filetype from data
		FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeFromMemory(fiMemory);
		if (format != FIF_UNKNOWN && FreeImage_FIFSupportsReading(format))
		{
			//file type is supported. load image
			FIBITMAP * fiBitmap = FreeImage_LoadFromMemory(format, fiMemory);
			if (fiBitmap != nullptr)
			{
				//loaded. convert to 32bit if necessary
				if (FreeImage_GetBPP(fiBitmap) != 32)
				{
					FIBITMAP * fiConverted = FreeImage_ConvertTo32Bits(fiBitmap);
					if (fiConverted != nullptr)
					{
						//free original bitmap data
						FreeImage_Unload(fiBitmap);
						fiBitmap = fiConverted;
					}
				}
				if (fiBitmap != nullptr)
				{					
					width = FreeImage_GetWidth(fiBitmap);
					height = FreeImage_GetHeight(fiBitmap);

					//loop through scanlines and add all pixel data to the return vector
					//this is necessary, because width*height*bpp might not be == pitch

					unsigned char * tempData = new unsigned char[width * height * 4];
					unsigned char * bytes = FreeImage_GetBits(fiBitmap);

				//	memcpy(tempData, bytes, width * height * 4);
/*
					for (size_t i = 0; i < height; i++)
					{
						const BYTE * scanLine = FreeImage_GetScanLine(fiBitmap, (int)i);
						memcpy(tempData + (i * width * 4), scanLine, width * 4);
					}*/
					//convert from BGRA to RGBA
					for(size_t i = 0; i < width*height; i++)
					{
						RGBQUAD bgra = ((RGBQUAD *)bytes)[i];
						RGBQUAD rgba;
						rgba.rgbBlue = bgra.rgbRed;
						rgba.rgbGreen = bgra.rgbGreen;
						rgba.rgbRed = bgra.rgbBlue;
						rgba.rgbReserved = bgra.rgbReserved;
						((RGBQUAD *)tempData)[i] = rgba;
					}
					
					rawData = std::vector<unsigned char>(tempData, tempData + width * height * 4);
					//free bitmap data
					FreeImage_Unload(fiBitmap);
					delete[] tempData;
				}
			}
			else
			{
				LOG(LogError) << "Error - Failed to load image from memory!";
			}
		}
		else
		{
			LOG(LogError) << "Error - File type " << (format == FIF_UNKNOWN ? "unknown" : "unsupported") << "!";
		}
		//free FIMEMORY again
		FreeImage_CloseMemory(fiMemory);
	}
	return rawData;
}

#include "math/Vector2i.h"

//public static Rectangle GetPictureRect(Size imageSize, Rectangle rcPhoto, bool outerZooming = false, bool sourceRect = false)

Vector2i ImageIO::adjustPictureSize(Vector2i imageSize, Vector2i maxSize)
{
	int cxDIB = imageSize.x();
	int cyDIB = imageSize.y();
	int iMaxX = maxSize.x();
	int iMaxY = maxSize.y();

	double xCoef = (double)iMaxX / (double)cxDIB;
	double yCoef = (double)iMaxY / (double)cyDIB;
	
	cyDIB = (int)((double)cyDIB * std::fmax(xCoef, yCoef));
	cxDIB = (int)((double)cxDIB * std::fmax(xCoef, yCoef));

	if (cxDIB > iMaxX)
	{
		cyDIB = (int)((double)cyDIB * (double)iMaxX / (double)cxDIB);
		cxDIB = iMaxX;
	}

	if (cyDIB > iMaxY)
	{
		cxDIB = (int)((double)cxDIB * (double)iMaxY / (double)cyDIB);
		cyDIB = iMaxY;
	}

	return Vector2i(cxDIB, cyDIB);
}


unsigned char* ImageIO::loadFromMemoryRGBA32Ex(const unsigned char * data, const size_t size, size_t & width, size_t & height, int maxWidth, int maxHeight)
{
	width = 0;
	height = 0;

	FIMEMORY * fiMemory = FreeImage_OpenMemory((BYTE *)data, (DWORD)size);
	if (fiMemory != nullptr)
	{
		//detect the filetype from data
		FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeFromMemory(fiMemory);
		if (format != FIF_UNKNOWN && FreeImage_FIFSupportsReading(format))
		{
			//file type is supported. load image
			FIBITMAP * fiBitmap = FreeImage_LoadFromMemory(format, fiMemory);
			if (fiBitmap != nullptr)
			{
				//loaded. convert to 32bit if necessary
				if (FreeImage_GetBPP(fiBitmap) != 32)
				{
					FIBITMAP * fiConverted = FreeImage_ConvertTo32Bits(fiBitmap);
					if (fiConverted != nullptr)
					{
						//free original bitmap data
						FreeImage_Unload(fiBitmap);
						fiBitmap = fiConverted;
					}
				}
				if (fiBitmap != nullptr)
				{
					width = FreeImage_GetWidth(fiBitmap);
					height = FreeImage_GetHeight(fiBitmap);
					
					if (maxWidth > 0 && maxHeight > 0 && (width > maxWidth || height > maxHeight))
					{
						Vector2i sz = adjustPictureSize(Vector2i(width, height), Vector2i(maxWidth, maxHeight));
						if (sz.x() != width || sz.y() != height)
						{
							FIBITMAP* imageRescaled = FreeImage_Rescale(fiBitmap, sz.x(), sz.y(), FILTER_BOX);
							FreeImage_Unload(fiBitmap);
							fiBitmap = imageRescaled;

							width = FreeImage_GetWidth(fiBitmap);
							height = FreeImage_GetHeight(fiBitmap);
						}
					}
					
					//loop through scanlines and add all pixel data to the return vector
					//this is necessary, because width*height*bpp might not be == pitch

					unsigned char* tempData = new unsigned char[width * height * 4];

					int w = (int)width;

					for (int y = (int)height; --y >= 0; )
					{
						unsigned int* argb = (unsigned int*)FreeImage_GetScanLine(fiBitmap, y);
						unsigned int* abgr = (unsigned int*)(tempData + (y * width * 4));
						for (int x = w; --x >= 0;)
						{
							unsigned int c = argb[x];
							abgr[x] = (c & 0xFF00FF00) | ((c & 0xFF) << 16) | ((c >> 16) & 0xFF);
						}
					}
				
					FreeImage_Unload(fiBitmap);
					FreeImage_CloseMemory(fiMemory);

					return tempData;
				}
			}
			else
			{
				LOG(LogError) << "Error - Failed to load image from memory!";
			}
		}
		else
		{
			LOG(LogError) << "Error - File type " << (format == FIF_UNKNOWN ? "unknown" : "unsupported") << "!";
		}
		//free FIMEMORY again
		FreeImage_CloseMemory(fiMemory);
	}

	return NULL;
}

void ImageIO::flipPixelsVert(unsigned char* imagePx, const size_t& width, const size_t& height)
{
	unsigned int temp;
	unsigned int* arr = (unsigned int*)imagePx;
	for(size_t y = 0; y < height / 2; y++)
	{
		for(size_t x = 0; x < width; x++)
		{
			temp = arr[x + (y * width)];
			arr[x + (y * width)] = arr[x + (height * width) - ((y + 1) * width)];
			arr[x + (height * width) - ((y + 1) * width)] = temp;
		}
	}
}
