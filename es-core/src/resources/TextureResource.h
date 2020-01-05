#pragma once
#ifndef ES_CORE_RESOURCES_TEXTURE_RESOURCE_H
#define ES_CORE_RESOURCES_TEXTURE_RESOURCE_H

#include "math/Vector2i.h"
#include "math/Vector2f.h"
#include "resources/ResourceManager.h"
#include "resources/TextureDataManager.h"
#include <set>
#include <string>

class TextureData;

class MaxSizeInfo
{
public:	
	MaxSizeInfo() : mSize(Vector2f(0, 0)), mExternalZoom(false) {}

	MaxSizeInfo(float x, float y) : mSize(Vector2f(x, y)), mExternalZoom(false), mExternalZoomKnown(false) { }
	MaxSizeInfo(Vector2f size) : mSize(size), mExternalZoom(false), mExternalZoomKnown(false) { }

	MaxSizeInfo(float x, float y, bool externalZoom) : mSize(Vector2f(x, y)), mExternalZoom(externalZoom), mExternalZoomKnown(true){ }
	MaxSizeInfo(Vector2f size, bool externalZoom) : mSize(size), mExternalZoom(externalZoom), mExternalZoomKnown(true) { }

	bool empty() { return mSize.x() <= 1 && mSize.y() <= 1; }

	float x() { return mSize.x(); }
	float y() { return mSize.y(); }

	bool externalZoom() 
	{ 
		return mExternalZoom; 
	}

	bool isExternalZoomKnown()
	{
		return mExternalZoomKnown;
	}

private:
	Vector2f mSize;
	bool	 mExternalZoom;
	bool	 mExternalZoomKnown;
};

// An OpenGL texture.
// Automatically recreates the texture with renderer deinit/reinit.
class TextureResource : public IReloadable
{
protected:
	TextureResource(const std::string& path, bool tile, bool linear, bool dynamic, bool allowAsync, MaxSizeInfo maxSize);

public:
	static std::shared_ptr<TextureResource> get(const std::string& path, bool tile = false, bool linear = false, bool forceLoad = false, bool dynamic = true, bool asReloadable = true, MaxSizeInfo maxSize = MaxSizeInfo());
	static void cancelAsync(std::shared_ptr<TextureResource> texture);

	void initFromPixels(const unsigned char* dataRGBA, size_t width, size_t height);
	void initFromExternalPixels(unsigned char* dataRGBA, size_t width, size_t height);
	virtual void initFromMemory(const char* file, size_t length);

	// For scalable source images in textures we want to set the resolution to rasterize at
	void rasterizeAt(float width, float height);
	Vector2f getSourceImageSize() const;

	virtual ~TextureResource();

	bool isLoaded() const;
	bool isTiled() const;

	const Vector2i getSize() const;
	bool bind();

	static size_t getTotalMemUsage(); // returns an approximation of total VRAM used by textures (in bytes)
	static size_t getTotalTextureSize(); // returns the number of bytes that would be used if all textures were in memory
	static void resetCache();

public:
	virtual bool unload();
	virtual void reload();

	void onTextureLoaded(std::shared_ptr<TextureData> tex);

private:
	// mTextureData is used for textures that are not loaded from a file - these ones
	// are permanently allocated and cannot be loaded and unloaded based on resources
	std::shared_ptr<TextureData>		mTextureData;

	// The texture data manager manages loading and unloading of filesystem based textures
	static TextureDataManager		sTextureDataManager;

	Vector2i					mSize;
	Vector2f					mSourceSize;
	bool							mForceLoad;

	typedef std::tuple<std::string, bool, bool> TextureKeyType;
	static std::map< TextureKeyType, std::weak_ptr<TextureResource> > sTextureMap; // map of textures, used to prevent duplicate textures
	static std::map< TextureKeyType, std::shared_ptr<TextureResource> > sPermanentTextureMap; // map of textures, used to prevent duplicate textures // FCAWEAK
	static std::set<TextureResource*> 	sAllTextures;	// Set of all textures, used for memory management

#if _DEBUG
	std::string	mPath;
#endif
};

#endif // ES_CORE_RESOURCES_TEXTURE_RESOURCE_H
