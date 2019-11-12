#pragma once
#ifndef ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H
#define ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H

#include "NinePatchComponent.h"
#include "ImageComponent.h"
#include "TextComponent.h"
#include "ThemeData.h"

class VideoComponent;

struct ImageProperties
{
public:
	ImageProperties()
	{
		Loaded = false;
		Visible  = false;

		reflexion = Vector2f::Zero();
		pos = Vector2f(0.5f, 0.5f);
		size = Vector2f(1.0f, 1.0f);
		origin = Vector2f(0.5f, 0.5f);
		color = colorEnd = 0xFFFFFFFF;		
		sizeMode = 2;
	}

	void updateImageComponent(ImageComponent* image, Vector2f parentSize, bool disableSize = false)
	{
		if (image == nullptr)
			return;

		image->setPosition(pos.x() * parentSize.x(), pos.y() * parentSize.y());
		
		if (!disableSize && sizeMode == 0)
			image->setSize(size.x() * parentSize.x(), size.y() * parentSize.y());
		else if (sizeMode == 1)
			image->setMinSize(size.x() * parentSize.x(), size.y() * parentSize.y());
		else
			image->setMaxSize(size.x() * parentSize.x(), size.y() * parentSize.y());

		image->setOrigin(origin);
		image->setColorShift(color);
		image->setColorShiftEnd(colorEnd);
		image->setMirroring(reflexion);
	}

	bool loadElement(const ThemeData::ThemeElement* elem)
	{
		if (!elem)
		{
			Visible = false;
			return false;
		}

		Loaded = true;
		Visible = true;

		if (elem && elem->has("visible"))
			Visible = elem->get<bool>("visible");

		if (elem && elem->has("origin"))
			origin = elem->get<Vector2f>("origin");

		if (elem && elem->has("pos"))
			pos = elem->get<Vector2f>("pos");

		if (elem && elem->has("size"))
		{
			sizeMode = 0;
			size = elem->get<Vector2f>("size");
		}
		else if (elem && elem->has("minSize"))
		{
			sizeMode = 1;
			size = elem->get<Vector2f>("minSize");
		}
		else if (elem && elem->has("maxSize"))
		{
			sizeMode = 2;
			size = elem->get<Vector2f>("maxSize");
		}

		if (elem && elem->has("color"))
			color = colorEnd = elem->get<unsigned int>("color");

		if (elem && elem->has("colorEnd"))
			colorEnd = elem->get<unsigned int>("colorEnd");

		if (elem && elem->has("reflexion"))
			reflexion = elem->get<Vector2f>("reflexion");

		return true;
	}

	bool Loaded;
	bool Visible;

	Vector2f pos;
	Vector2f size;

	Vector2f origin;

	Vector2f reflexion;
	
	unsigned int color;
	unsigned int colorEnd;

	int sizeMode;
};

struct GridTileProperties
{
	Vector2f mSize;
	Vector2f mPadding;
	unsigned int mImageColor;
	std::string mBackgroundImage;
	Vector2f mBackgroundCornerSize;
	unsigned int mBackgroundCenterColor;
	unsigned int mBackgroundEdgeColor;

	std::string mImageSizeMode;
	std::string mSelectionMode;

	Vector2f mLabelPos;
	Vector2f mLabelSize;

	unsigned int mLabelColor;
	unsigned int mLabelBackColor;

	unsigned int mLabelGlowColor;
	unsigned int mLabelGlowSize;

	std::string		mFontPath;
	unsigned int	mFontSize;

	Vector2f		mMirror;

	ImageProperties Image;
	ImageProperties Marquee;
	ImageProperties Favorite;
};

class GridTileComponent : public GuiComponent
{
public:
	GridTileComponent(Window* window);
	~GridTileComponent();

	void render(const Transform4x4f& parentTrans) override;

	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties);

	// Made this a static function because the ImageGridComponent need to know the default tile max size
	// to calculate the grid dimension before it instantiate the GridTileComponents
	static Vector2f getDefaultTileSize();
	Vector2f getSelectedTileSize() const;
	bool isSelected() const;

	void resetImages();

	void setLabel(std::string name);
	void setVideo(const std::string& path, float defaultDelay = -1.0);

	void setImage(const std::string& path, bool isDefaultImage = false);
	void setMarquee(const std::string& path);
	
	void setFavorite(bool favorite);
	bool hasFavoriteMedia() { return mFavorite != nullptr; }

	void setSelected(bool selected, bool allowAnimation = true, Vector3f* pPosition = NULL, bool force = false);
	void setVisible(bool visible);

	void forceSize(Vector2f size, float selectedZoom = 1.0);

	void renderBackground(const Transform4x4f& parentTrans);
	void renderContent(const Transform4x4f& parentTrans);

	bool shouldSplitRendering() { return isAnimationPlaying(3); };

	Vector3f getBackgroundPosition();

	virtual void onShow();
	virtual void onHide();
	virtual void update(int deltaTime);

	std::shared_ptr<TextureResource> getTexture(bool marquee = false);

private:
	void	resetProperties();
	void	createVideo();
	void	createMarquee();
	void	createFavorite();
	void	startVideo();
	void	stopVideo();

	void resize();
	
	inline static unsigned int mixUnsigned(const unsigned int def, const unsigned int sel, float percent);
	static Vector2f mixVectors(const Vector2f& def, const Vector2f& sel, float percent);

	GridTileProperties getCurrentProperties();

	TextComponent mLabel;

	bool mLabelVisible;
	bool mLabelMerged;

	NinePatchComponent mBackground;

	GridTileProperties mDefaultProperties;
	GridTileProperties mSelectedProperties;

	std::string mCurrentMarquee;
	std::string mCurrentPath;
	std::string mVideoPath;

	void setSelectedZoom(float percent);

	float mSelectedZoomPercent;
	bool mSelected;
	bool mVisible;

	bool mIsDefaultImage;

	Vector3f mAnimPosition;

	VideoComponent* mVideo;
	ImageComponent* mImage;
	ImageComponent* mMarquee;
	ImageComponent* mFavorite;

	bool mVideoPlaying;
	bool mShown;
};

#endif // ES_CORE_COMPONENTS_GRID_TILE_COMPONENT_H
