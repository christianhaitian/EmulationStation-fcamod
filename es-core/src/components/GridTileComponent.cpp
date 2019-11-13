#include "GridTileComponent.h"

#include "resources/TextureResource.h"
#include "ThemeData.h"

#include <algorithm>

#include "animations/LambdaAnimation.h"
#include "ImageIO.h"

#ifdef _RPI_
#include "components/VideoPlayerComponent.h"
#endif
#include "components/VideoVlcComponent.h"
#include "utils/FileSystemUtil.h"

#include "Settings.h"
#include "ImageGridComponent.h"

#define VIDEODELAY	100

GridTileComponent::GridTileComponent(Window* window) : GuiComponent(window), mBackground(window), mLabel(window), mVideo(nullptr), mVideoPlaying(false), mShown(false)
{
	mSelectedZoomPercent = 1.0f;
	mAnimPosition = Vector3f(0, 0);
	mVideo = nullptr;
	mMarquee = nullptr;
	mFavorite = nullptr;
	mImageOverlay = nullptr;
	mIsDefaultImage = false;
	
	mLabelMerged = false;

	resetProperties();

	mImage = new ImageComponent(mWindow);
	mImage->setOrigin(0.5f, 0.5f);

	addChild(&mBackground);
	addChild(&(*mImage));
	addChild(&mLabel);

	setSelected(false);
	setVisible(true);
}

void GridTileComponent::resetProperties()
{
	mDefaultProperties.mSize = getDefaultTileSize();
	mDefaultProperties.mPadding = Vector2f(16.0f, 16.0f);

	mSelectedProperties.mSize = getSelectedTileSize();
	mSelectedProperties.mPadding = mDefaultProperties.mPadding;

	mDefaultProperties.mBackgroundImage = ":/frame.png";
	mDefaultProperties.mBackgroundCornerSize = Vector2f(16, 16);
	mDefaultProperties.mBackgroundCenterColor = 0xAAAAEEFF;
	mDefaultProperties.mBackgroundEdgeColor = 0xAAAAEEFF;

	mSelectedProperties.mBackgroundImage = mDefaultProperties.mBackgroundImage;
	mSelectedProperties.mBackgroundCornerSize = mDefaultProperties.mBackgroundCornerSize;
	mSelectedProperties.mBackgroundCenterColor = 0xFFFFFFFF;
	mSelectedProperties.mBackgroundEdgeColor = 0xFFFFFFFF;

	mDefaultProperties.Label = mSelectedProperties.Label = GridTextProperties();	
	mDefaultProperties.Image = mSelectedProperties.Image = GridImageProperties();	
	mDefaultProperties.Marquee = mSelectedProperties.Marquee = GridImageProperties();
	mDefaultProperties.Favorite = mSelectedProperties.Favorite = GridImageProperties();

	mDefaultProperties.Image.color = mDefaultProperties.Image.colorEnd = 0xFFFFFFDD;
}

void GridTileComponent::forceSize(Vector2f size, float selectedZoom)
{
	mDefaultProperties.mSize = size;
	mSelectedProperties.mSize = size * selectedZoom;
}

GridTileComponent::~GridTileComponent()
{
	if (mImage != nullptr)
		delete mImage;

	if (mImageOverlay != nullptr)
		delete mImageOverlay;

	if (mFavorite != nullptr)
		delete mFavorite;

	if (mMarquee != nullptr)
		delete mMarquee;

	if (mVideo != nullptr)
		delete mVideo;

	mFavorite = nullptr;
	mMarquee = nullptr;
	mImage = nullptr;
	mVideo = nullptr;
	mImageOverlay = nullptr;
}

std::shared_ptr<TextureResource> GridTileComponent::getTexture(bool marquee) 
{ 
	if (marquee && mMarquee  != nullptr)
		return mMarquee->getTexture();
	else if (!marquee && mImage != nullptr)
		return mImage->getTexture();

	return nullptr; 
}

void GridTileComponent::resize()
{
	auto currentProperties = getCurrentProperties();

	Vector2f size = currentProperties.mSize;
	if (mSize != size)
		setSize(size);

	bool isDefaultImage = mIsDefaultImage && (mCurrentPath == ":/folder.svg" || mCurrentPath == ":/cartridge.svg");

	float height = (int) (size.y() * currentProperties.Label.size.y());
	float labelHeight = height;

	if (!currentProperties.Label.Visible || mLabelMerged || currentProperties.Label.size.x() == 0)
		height = 0;

	float topPadding = currentProperties.mPadding.y();
	float bottomPadding = std::max(topPadding, height);
	float paddingX = currentProperties.mPadding.x();

	float imageWidth = size.x() - paddingX * 2.0;
	float imageHeight = size.y() - topPadding - bottomPadding;

	Vector2f imageSize(size.x(), (size.y() - height));

	// Image
	if (currentProperties.Image.Loaded)
	{
		currentProperties.Image.updateImageComponent(mImage, imageSize, false);

		if (mImage != nullptr && currentProperties.Image.sizeMode != "maxSize" && isDefaultImage)
			mImage->setMaxSize(imageWidth, imageHeight);
	}
	else if (mImage != nullptr)
	{
		// Retrocompatibility : imagegrid.image is not defined
		mImage->setPosition(imageSize.x() / 2.0f, imageSize.y() / 2.0f);
		mImage->setColorShift(currentProperties.Image.color);
		mImage->setMirroring(currentProperties.Image.reflexion);

		if (currentProperties.Image.sizeMode == "minSize" && !isDefaultImage)
			mImage->setMinSize(imageWidth, imageHeight);
		else if (currentProperties.Image.sizeMode == "size")
			mImage->setSize(imageWidth, imageHeight);
		else
			mImage->setMaxSize(imageWidth, imageHeight);	
	}

	// Text
	mLabel.setVisible(currentProperties.Label.Visible || mIsDefaultImage);

	if (currentProperties.Label.Visible)
	{
		currentProperties.Label.updateTextComponent(&mLabel, mSize);

		if (currentProperties.Label.pos.x() < 0 && mLabelMerged)
		{
			mLabel.setPosition(0, mSize.y() - labelHeight);
			mLabel.setSize(size.x(), labelHeight);
		}
		else if (mImage != nullptr && !mLabelMerged)
		{
			mLabel.setPosition(mImage->getPosition().x() - mImage->getSize().x() / 2, mImage->getSize().y());
			mLabel.setSize(mImage->getSize().x(), labelHeight);
		}
	}
	else if (mIsDefaultImage)
	{
		mLabel.setColor(0xFFFFFFFF);
		mLabel.setGlowColor(0x00000010);
		mLabel.setGlowSize(2);
		mLabel.setOpacity(255);
		mLabel.setPosition(mSize.x() * 0.1, mSize.y() * 0.2);
		mLabel.setSize(mSize.x() - mSize.x() * 0.2, mSize.y() - mSize.y() * 0.3);
	}

	// Other controls ( Favorite / Marquee / Overlay )
	if (currentProperties.Favorite.Loaded)
		currentProperties.Favorite.updateImageComponent(mFavorite, Vector2f(size.x(), (size.y() - height)), false);

	if (currentProperties.Marquee.Loaded)
		currentProperties.Marquee.updateImageComponent(mMarquee, Vector2f(size.x(), (size.y() - height)), true);

	if (currentProperties.ImageOverlay.Loaded)
		currentProperties.ImageOverlay.updateImageComponent(mImageOverlay, Vector2f(size.x(), (size.y() - height)), false);

	// Video
	if (mVideo != nullptr && mVideo->isPlaying())
	{
		mVideo->setPosition(size.x() / 2.0f, (size.y() - height) / 2.0f);

		if (currentProperties.Image.sizeMode == "minSize")
		{
			if (mImage != nullptr)
				mVideo->setRoundCorners(mImage->getRoundCorners());

			auto vs = mVideo->getVideoSize();
			if (vs == Vector2f(0, 0))
				vs = Vector2f(640, 480);

			mVideo->setSize(ImageIO::adjustExternPictureSizef(vs, Vector2f(imageWidth, imageHeight)));
		}
		else
			if (currentProperties.Image.sizeMode == "size")
				mVideo->setSize(imageWidth, size.y() - topPadding - bottomPadding);
			else
				mVideo->setMaxSize(imageWidth, size.y() - topPadding - bottomPadding);
	}

	// Background
	Vector3f bkposition = Vector3f(0, 0);
	Vector2f bkSize = size;

	if (mImage != NULL && currentProperties.mSelectionMode == "image" && mImage->getSize() != Vector2f(0, 0))
	{
		if (currentProperties.Image.sizeMode == "minSize")
			bkSize = Vector2f(size.x(), size.y() - bottomPadding + topPadding);
		else if (mAnimPosition == Vector3f(0, 0, 0))
		{
			bkposition = Vector3f(
				mImage->getPosition().x() - mImage->getSize().x() / 2 - mSelectedProperties.mPadding.x(),
				mImage->getPosition().y() - mImage->getSize().y() / 2 - mSelectedProperties.mPadding.y(), 0);

			bkSize = Vector2f(mImage->getSize().x() + 2 * mSelectedProperties.mPadding.x(), mImage->getSize().y() + 2 * mSelectedProperties.mPadding.y());
		}
		else
		{
			bkposition = Vector3f(
				mImage->getPosition().x() - mImage->getSize().x() / 2 - currentProperties.mPadding.x(),
				mImage->getPosition().y() - mImage->getSize().y() / 2 - currentProperties.mPadding.y(), 0);

			bkSize = Vector2f(mImage->getSize().x() + 2 * currentProperties.mPadding.x(), mImage->getSize().y() + 2 * currentProperties.mPadding.y());
		}
	}

	if (mSelectedZoomPercent != 1.0f && mAnimPosition.x() != 0 && mAnimPosition.y() != 0 && mSelected)
	{
		float x = mPosition.x() + bkposition.x();
		float y = mPosition.y() + bkposition.y();

		x = mAnimPosition.x() * (1.0 - mSelectedZoomPercent) + x * mSelectedZoomPercent;
		y = mAnimPosition.y() * (1.0 - mSelectedZoomPercent) + y * mSelectedZoomPercent;

		bkposition = Vector3f(x - mPosition.x(), y - mPosition.y(), 0);
	}	
	
	mBackground.setPosition(bkposition);
	mBackground.setSize(bkSize);

	mBackground.setCornerSize(currentProperties.mBackgroundCornerSize);
	mBackground.setCenterColor(currentProperties.mBackgroundCenterColor);
	mBackground.setEdgeColor(currentProperties.mBackgroundEdgeColor);
	mBackground.setImagePath(currentProperties.mBackgroundImage);

	if (mSelected && mAnimPosition == Vector3f(0, 0, 0) && mSelectedZoomPercent != 1.0)
		mBackground.setOpacity(mSelectedZoomPercent * 255);
	else
		mBackground.setOpacity(255);
}

void GridTileComponent::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	if (mVideo != nullptr && mVideo->isPlaying())
		resize();
}

void GridTileComponent::renderBackground(const Transform4x4f& parentTrans)
{
	if (!mVisible)
		return;

	Transform4x4f trans = getTransform() * parentTrans;
	mBackground.render(trans);
}

void GridTileComponent::renderContent(const Transform4x4f& parentTrans)
{
	if (!mVisible)
		return;

	Transform4x4f trans = getTransform() * parentTrans;

	Vector2f clipPos(trans.translation().x(), trans.translation().y());
	if (!Renderer::isVisibleOnScreen(clipPos.x(), clipPos.y(), mSize.x(), mSize.y()))
		return;

	auto currentProperties = getCurrentProperties();

	float padding = currentProperties.mPadding.x();
	float topPadding = currentProperties.mPadding.y();
	float bottomPadding = topPadding;

	if (currentProperties.Label.Visible && !mLabelMerged)
		bottomPadding = std::max((int)topPadding, (int)(mSize.y() * currentProperties.Label.size.y()));

	Vector2i pos((int)Math::round(trans.translation()[0] + padding), (int)Math::round(trans.translation()[1] + topPadding));
	Vector2i size((int)Math::round(mSize.x() - 2 * padding), (int)Math::round(mSize.y() - topPadding - bottomPadding));
	
	bool isDefaultImage = mIsDefaultImage && (mCurrentPath == ":/folder.svg" || mCurrentPath == ":/cartridge.svg");
	bool isMinSize = currentProperties.Image.sizeMode == "minSize" && !isDefaultImage;

	if (isMinSize)
		Renderer::pushClipRect(pos, size);

	if (mImage != NULL)
		mImage->render(trans);

	if (mSelected && !mVideoPath.empty() && mVideo != nullptr)
		mVideo->render(trans);

	if (!mLabelMerged && isMinSize)
		Renderer::popClipRect();

	if (mMarquee != nullptr && mMarquee->hasImage())
		mMarquee->render(trans);
	else if (currentProperties.Label.Visible && currentProperties.Label.size.y()>0)
		mLabel.render(trans);
	else if (!currentProperties.Label.Visible && mIsDefaultImage)
		mLabel.render(trans);

	if (mFavorite != nullptr && mFavorite->hasImage() && mFavorite->isVisible())
		mFavorite->render(trans);

	if (mImageOverlay != nullptr && mImageOverlay->hasImage() && mImageOverlay->isVisible())
		mImageOverlay->render(trans);

	if (mLabelMerged && isMinSize)
		Renderer::popClipRect();
}

void GridTileComponent::render(const Transform4x4f& parentTrans)
{
	if (!mVisible)
		return;

	renderBackground(parentTrans);
	renderContent(parentTrans);
}

void GridTileComponent::createMarquee()
{
	if (mMarquee != nullptr)
		return;

	mMarquee = new ImageComponent(mWindow);
	mMarquee->setOrigin(0.5f, 0.5f);
	mMarquee->setDefaultZIndex(35);
	addChild(mMarquee);
}

void GridTileComponent::createFavorite()
{
	if (mFavorite != nullptr)
		return;

	mFavorite = new ImageComponent(mWindow);
	mFavorite->setOrigin(0.5f, 0.5f);
	mFavorite->setDefaultZIndex(35);
	mFavorite->setVisible(false);
	
	addChild(mFavorite);
}

void GridTileComponent::createImageOverlay()
{
	if (mImageOverlay != nullptr)
		return;

	mImageOverlay = new ImageComponent(mWindow);
	mImageOverlay->setOrigin(0.5f, 0.5f);
	mImageOverlay->setDefaultZIndex(35);
	mImageOverlay->setVisible(false);

	addChild(mImageOverlay);
}

void GridTileComponent::createVideo()
{
	if (mVideo != nullptr)
		return;

	mVideo = new VideoVlcComponent(mWindow, "");

	// video
	mVideo->setOrigin(0.5f, 0.5f);
	mVideo->setStartDelay(VIDEODELAY);
	mVideo->setDefaultZIndex(30);
	addChild(mVideo);
}

void GridTileComponent::applyThemeToProperties(const ThemeData::ThemeElement* elem, GridTileProperties& properties)
{
	if (elem == nullptr)
		return;

	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	if (elem->has("size"))
		properties.mSize = elem->get<Vector2f>("size") * screen;

	if (elem->has("padding"))
		properties.mPadding = elem->get<Vector2f>("padding");

	if (elem->has("backgroundImage"))
		properties.mBackgroundImage = elem->get<std::string>("backgroundImage");

	if (elem->has("backgroundCornerSize"))
		properties.mBackgroundCornerSize = elem->get<Vector2f>("backgroundCornerSize");

	if (elem->has("backgroundColor"))
	{
		properties.mBackgroundCenterColor = elem->get<unsigned int>("backgroundColor");
		properties.mBackgroundEdgeColor = elem->get<unsigned int>("backgroundColor");
	}

	if (elem->has("backgroundCenterColor"))
		properties.mBackgroundCenterColor = elem->get<unsigned int>("backgroundCenterColor");

	if (elem->has("backgroundEdgeColor"))
		properties.mBackgroundEdgeColor = elem->get<unsigned int>("backgroundEdgeColor");

	if (elem && elem->has("selectionMode"))
		properties.mSelectionMode = elem->get<std::string>("selectionMode");		

	if (elem && elem->has("reflexion"))
		properties.Image.reflexion = elem->get<Vector2f>("reflexion");

	if (elem->has("imageColor"))
		properties.Image.color = properties.Image.colorEnd = elem->get<unsigned int>("imageColor");

	if (elem && elem->has("imageSizeMode"))
		properties.Image.sizeMode = elem->get<std::string>("imageSizeMode");
}

bool GridImageProperties::applyTheme(const ThemeData::ThemeElement* elem)
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
		sizeMode = "size";
		size = elem->get<Vector2f>("size");
	}
	else if (elem && elem->has("minSize"))
	{
		sizeMode = "minSize";
		size = elem->get<Vector2f>("minSize");
	}
	else if (elem && elem->has("maxSize"))
	{
		sizeMode = "maxSize";
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

bool GridTextProperties::applyTheme(const ThemeData::ThemeElement* elem)
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

	if (elem && elem->has("pos"))
		pos = elem->get<Vector2f>("pos");

	if (elem && elem->has("size"))
		size = elem->get<Vector2f>("size");

	if (elem && elem->has("color"))
		color = elem->get<unsigned int>("color");

	if (elem && elem->has("backgroundColor"))
		backColor = elem->get<unsigned int>("backgroundColor");

	if (elem && elem->has("glowColor"))
		glowColor = elem->get<unsigned int>("glowColor");

	if (elem && elem->has("glowSize"))
		glowSize = elem->get<float>("glowSize");

	if (elem && elem->has("fontSize"))
		fontSize = elem->get<float>("fontSize");

	if (elem && elem->has("fontPath"))
		fontPath = elem->get<std::string>("fontPath");

	return true;
}


void GridTileComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	resetProperties();

	const ThemeData::ThemeElement* grid = theme->getElement(view, "gamegrid", "imagegrid");
	if (grid && grid->has("showVideoAtDelay"))
	{
		createVideo();
		mVideo->applyTheme(theme, view, element, properties);
	}
	else if (mVideo != nullptr)
	{
		removeChild(mVideo);
		delete mVideo;
		mVideo = nullptr;
	}

	// Apply theme to the default gridtile
	const ThemeData::ThemeElement* elem = theme->getElement(view, "default", "gridtile");
	if (elem)
	{
		applyThemeToProperties(elem, mDefaultProperties);
		applyThemeToProperties(elem, mSelectedProperties);		
	}

	// Apply theme to the selected gridtile
	elem = theme->getElement(view, "selected", "gridtile");
	if (elem)
		applyThemeToProperties(elem, mSelectedProperties);
		

	// Apply theme to the <image name="gridtile.image"> element
	elem = theme->getElement(view, "gridtile.image", "image");
	if (elem)
	{
		mImage->applyTheme(theme, view, "gridtile.image", ThemeFlags::ALL ^ (ThemeFlags::PATH));

		mDefaultProperties.Image.applyTheme(elem);
		mSelectedProperties.Image.applyTheme(elem);

		// Apply theme to the <image name="gridtile.image:selected"> element
		elem = theme->getElement(view, "gridtile.image:selected", "image");
		if (elem)
			mSelectedProperties.Image.applyTheme(elem);
	}
	

	// Apply theme to the <image name="gridtile.marquee"> element
	elem = theme->getElement(view, "gridtile.marquee", "image");
	if (elem)
	{
		createMarquee();
		mMarquee->applyTheme(theme, view, "gridtile.marquee", ThemeFlags::ALL ^ (ThemeFlags::PATH));

		mDefaultProperties.Marquee.applyTheme(elem);
		mSelectedProperties.Marquee = mDefaultProperties.Marquee;

		// Apply theme to the <image name="gridtile.marquee:selected"> element
		elem = theme->getElement(view, "gridtile.marquee:selected", "image");
		if (elem)
			mSelectedProperties.Marquee.applyTheme(elem);
	}
	else if (mMarquee != nullptr)
	{
		removeChild(mMarquee);
		delete mMarquee;
		mMarquee = nullptr;
	}


	// Apply theme to the <image name="gridtile.marquee"> element
	elem = theme->getElement(view, "gridtile.favorite", "image");
	if (elem)
	{
		createFavorite();
		mFavorite->applyTheme(theme, view, "gridtile.favorite", ThemeFlags::ALL);

		mDefaultProperties.Favorite.sizeMode = "size";
		mDefaultProperties.Favorite.applyTheme(elem);
		mSelectedProperties.Favorite = mDefaultProperties.Favorite;

		// Apply theme to the <image name="gridtile.favorite:selected"> element
		elem = theme->getElement(view, "gridtile.favorite:selected", "image");
		if (elem)
			mSelectedProperties.Favorite.applyTheme(elem);
	}
	else if (mFavorite != nullptr)
	{
		removeChild(mFavorite);
		delete mFavorite;
		mFavorite = nullptr;
	}


	// Apply theme to the <image name="gridtile.overlay"> element
	elem = theme->getElement(view, "gridtile.overlay", "image");
	if (elem)
	{
		createImageOverlay();
		mImageOverlay->applyTheme(theme, view, "gridtile.overlay", ThemeFlags::ALL);

		mDefaultProperties.ImageOverlay.sizeMode = "size";
		mDefaultProperties.ImageOverlay.applyTheme(elem);
		mSelectedProperties.ImageOverlay = mDefaultProperties.ImageOverlay;

		// Apply theme to the <image name="gridtile.favorite:selected"> element
		elem = theme->getElement(view, "gridtile.overlay:selected", "image");
		if (elem)
			mSelectedProperties.ImageOverlay.applyTheme(elem);
	}
	else if (mImageOverlay != nullptr)
	{
		removeChild(mImageOverlay);
		delete mImageOverlay;
		mImageOverlay = nullptr;
	}


	// Apply theme to the <text name="gridtile"> element
	elem = theme->getElement(view, "gridtile", "text");
	if (elem == nullptr) // Apply theme to the <text name="gridtile.text"> element		
		elem = theme->getElement(view, "gridtile.text", "text");

	if (elem != NULL)
	{
		mLabel.applyTheme(theme, view, element, properties);

		mDefaultProperties.Label.applyTheme(elem);
		mSelectedProperties.Label = mDefaultProperties.Label;

		mLabelMerged = elem->has("pos");
		if (!mLabelMerged && elem->has("size"))
			mLabelMerged = mDefaultProperties.Label.size.x() == 0;

		// Apply theme to the <text name="gridtile:selected"> element
		elem = theme->getElement(view, "gridtile:selected", "text");
		if (elem == nullptr) // Apply theme to the <text name="gridtile.text:selected"> element
			elem = theme->getElement(view, "gridtile.text:selected", "text");

		if (elem)
			mSelectedProperties.Label.applyTheme(elem);
	}
}

// Made this a static function because the ImageGridComponent need to know the default tile size
// to calculate the grid dimension before it instantiate the GridTileComponents
Vector2f GridTileComponent::getDefaultTileSize()
{
	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	return screen * 0.22f;
}

Vector2f GridTileComponent::getSelectedTileSize() const
{
	return mDefaultProperties.mSize * 1.2f;
}

bool GridTileComponent::isSelected() const
{
	return mSelected;
}

void GridTileComponent::setImage(const std::string& path, bool isDefaultImage)
{
	mIsDefaultImage = isDefaultImage;
	if (mCurrentPath == path)
		return;

	mCurrentPath = path;

	if (mSelectedProperties.mSize.x() > mSize.x())
		mImage->setImage(path, false, MaxSizeInfo(mSelectedProperties.mSize, mSelectedProperties.Image.sizeMode != "maxSize"));
	else
		mImage->setImage(path, false, MaxSizeInfo(mSize, mSelectedProperties.Image.sizeMode != "maxSize"));

	resize();
}

void GridTileComponent::setMarquee(const std::string& path)
{
	if (mMarquee == nullptr)
		return;

	if (mCurrentMarquee == path)
		return;

	mCurrentMarquee = path;

	if (mSelectedProperties.mSize.x() > mSize.x())
		mMarquee->setImage(path, false, MaxSizeInfo(mSelectedProperties.mSize));
	else
		mMarquee->setImage(path, false, MaxSizeInfo(mSize));

	resize();
}

void GridTileComponent::setFavorite(bool favorite)
{
	if (mFavorite == nullptr)
		return;

	mFavorite->setVisible(favorite);
	resize();
}

void GridTileComponent::resetImages()
{
	setLabel("");
	setImage("");
	setMarquee("");
	stopVideo();
}

void GridTileComponent::setLabel(std::string name)
{
	if (mLabel.getText() == name)
		return;

	mLabel.setText(name);
	resize();
}

void GridTileComponent::setVideo(const std::string& path, float defaultDelay)
{
	if (mVideoPath == path)
		return;

	mVideoPath = path;

	if (mVideo != nullptr)
	{
		if (defaultDelay >= 0.0)
			mVideo->setStartDelay(defaultDelay);

		if (mVideoPath.empty())
			stopVideo();
	}

	resize();
}

void GridTileComponent::onShow()
{
	GuiComponent::onShow();
	mShown = true;
}

void GridTileComponent::onHide()
{
	GuiComponent::onHide();
	mShown = false;
}

void GridTileComponent::startVideo()
{
	if (mVideo != nullptr)
	{
		// Inform video component about size before staring in order to be able to use OptimizeVideo parameter
		if (mSelectedProperties.Image.sizeMode == "minSize")
			mVideo->setMinSize(mSelectedProperties.mSize);
		else
			mVideo->setResize(mSelectedProperties.mSize);

		mVideo->setVideo(mVideoPath);
	}
}

void GridTileComponent::stopVideo()
{
	if (mVideo != nullptr)
		mVideo->setVideo("");
}

void GridTileComponent::setSelected(bool selected, bool allowAnimation, Vector3f* pPosition, bool force)
{
	if (!mShown || !ALLOWANIMATIONS)
		allowAnimation = false;

	if (mSelected == selected && !force)
	{
		if (mSelected)
			startVideo();

		return;
	}

	mSelected = selected;

	if (!mSelected)
		stopVideo();

	if (selected)
	{
		if (pPosition == NULL || !allowAnimation)
		{
			cancelAnimation(3);

			this->setSelectedZoom(1);
			mAnimPosition = Vector3f(0, 0, 0);
			startVideo();

			resize();
		}
		else
		{
			if (pPosition == NULL)
				mAnimPosition = Vector3f(0, 0, 0);
			else
				mAnimPosition = Vector3f(pPosition->x(), pPosition->y(), pPosition->z());

			auto func = [this](float t)
			{
				t -= 1; // cubic ease out
				float pct = Math::lerp(0, 1, t*t*t + 1);

				this->setSelectedZoom(pct);
			};

			cancelAnimation(3);
			setAnimation(new LambdaAnimation(func, 250), 0, [this] {
				this->setSelectedZoom(1);
				mAnimPosition = Vector3f(0, 0, 0);
				startVideo();
			}, false, 3);
		}
	}
	else // if (!selected)
	{
		if (!allowAnimation)
		{
			cancelAnimation(3);
			this->setSelectedZoom(0);
			stopVideo();
			resize();
		}
		else
		{
			this->setSelectedZoom(1);
			stopVideo();

			auto func = [this](float t)
			{
				t -= 1; // cubic ease out
				float pct = Math::lerp(0, 1, t*t*t + 1);
				this->setSelectedZoom(1.0 - pct);
			};

			cancelAnimation(3);
			setAnimation(new LambdaAnimation(func, 250), 0, [this] {
				this->setSelectedZoom(0);
			}, false, 3);
		}
	}
}

void GridTileComponent::setSelectedZoom(float percent)
{
	if (mSelectedZoomPercent == percent)
		return;

	mSelectedZoomPercent = percent;
	resize();
}

void GridTileComponent::setVisible(bool visible)
{
	mVisible = visible;
}

Vector2f GridTileComponent::mixVectors(const Vector2f& def, const Vector2f& sel, float percent)
{
	if (def == sel)
		return def;
		
	float x = def.x() * (1.0 - percent) + sel.x() * percent;
	float y = def.y() * (1.0 - percent) + sel.y() * percent;
	return Vector2f(x, y);	
}

unsigned int GridTileComponent::mixUnsigned(const unsigned int def, const unsigned int sel, float percent)
{
	if (def == sel)
		return def;

	return def * (1.0 - percent) + sel * percent;
}

float GridTileComponent::mixFloat(const float def, const float sel, float percent)
{
	if (def == sel)
		return def;

	return def * (1.0 - percent) + sel * percent;
}


GridTileProperties GridTileComponent::getCurrentProperties()
{
	using namespace Renderer;

	GridTileProperties mMixedProperties;

	if (mSelectedZoomPercent == 0.0f || mSelectedZoomPercent == 1.0f)
		return mSelected ? mSelectedProperties : mDefaultProperties;

	mMixedProperties = mSelected ? mSelectedProperties : mDefaultProperties;

	mMixedProperties.mSize				= mixVectors(mDefaultProperties.mSize, mSelectedProperties.mSize, mSelectedZoomPercent);
	mMixedProperties.mPadding			= mixVectors(mDefaultProperties.mPadding, mSelectedProperties.mPadding, mSelectedZoomPercent);

	if (mDefaultProperties.Label.Loaded)
	{
		mMixedProperties.Label.pos = mixVectors(mDefaultProperties.Label.pos, mSelectedProperties.Label.pos, mSelectedZoomPercent);
		mMixedProperties.Label.size = mixVectors(mDefaultProperties.Label.size, mSelectedProperties.Label.size, mSelectedZoomPercent);		
		mMixedProperties.Label.color = mixColors(mDefaultProperties.Label.color, mSelectedProperties.Label.color, mSelectedZoomPercent);
		mMixedProperties.Label.backColor = mixColors(mDefaultProperties.Label.backColor, mSelectedProperties.Label.backColor, mSelectedZoomPercent);
		mMixedProperties.Label.glowColor = mixColors(mDefaultProperties.Label.glowColor, mSelectedProperties.Label.glowColor, mSelectedZoomPercent);
		mMixedProperties.Label.glowSize = mixFloat(mDefaultProperties.Label.glowSize, mSelectedProperties.Label.glowSize, mSelectedZoomPercent);
		mMixedProperties.Label.fontSize = mixFloat(mDefaultProperties.Label.fontSize, mSelectedProperties.Label.fontSize, mSelectedZoomPercent);
	}

	if (mDefaultProperties.Image.Loaded)
	{
		mMixedProperties.Image.pos = mixVectors(mDefaultProperties.Image.pos, mSelectedProperties.Image.pos, mSelectedZoomPercent);
		mMixedProperties.Image.size = mixVectors(mDefaultProperties.Image.size, mSelectedProperties.Image.size, mSelectedZoomPercent);
		mMixedProperties.Image.origin = mixVectors(mDefaultProperties.Image.origin, mSelectedProperties.Image.origin, mSelectedZoomPercent);
		mMixedProperties.Image.color = mixColors(mDefaultProperties.Image.color, mSelectedProperties.Image.color, mSelectedZoomPercent);
		mMixedProperties.Image.colorEnd = mixColors(mDefaultProperties.Image.colorEnd, mSelectedProperties.Image.colorEnd, mSelectedZoomPercent);
		mMixedProperties.Image.reflexion = mixVectors(mDefaultProperties.Image.reflexion, mSelectedProperties.Image.reflexion, mSelectedZoomPercent);
	}

	if (mDefaultProperties.Marquee.Loaded)
	{
		mMixedProperties.Marquee.pos		= mixVectors(mDefaultProperties.Marquee.pos, mSelectedProperties.Marquee.pos, mSelectedZoomPercent);
		mMixedProperties.Marquee.size		= mixVectors(mDefaultProperties.Marquee.size, mSelectedProperties.Marquee.size, mSelectedZoomPercent);
		mMixedProperties.Marquee.origin		= mixVectors(mDefaultProperties.Marquee.origin, mSelectedProperties.Marquee.origin, mSelectedZoomPercent);
		mMixedProperties.Marquee.color		= mixColors(mDefaultProperties.Marquee.color, mSelectedProperties.Marquee.color, mSelectedZoomPercent);
		mMixedProperties.Marquee.colorEnd	= mixColors(mDefaultProperties.Marquee.colorEnd, mSelectedProperties.Marquee.colorEnd, mSelectedZoomPercent);
		mMixedProperties.Marquee.reflexion	= mixVectors(mDefaultProperties.Marquee.reflexion, mSelectedProperties.Marquee.reflexion, mSelectedZoomPercent);
	}

	if (mDefaultProperties.Favorite.Loaded)
	{
		mMixedProperties.Favorite.pos = mixVectors(mDefaultProperties.Favorite.pos, mSelectedProperties.Favorite.pos, mSelectedZoomPercent);
		mMixedProperties.Favorite.size = mixVectors(mDefaultProperties.Favorite.size, mSelectedProperties.Favorite.size, mSelectedZoomPercent);
		mMixedProperties.Favorite.origin = mixVectors(mDefaultProperties.Favorite.origin, mSelectedProperties.Favorite.origin, mSelectedZoomPercent);
		mMixedProperties.Favorite.color = mixColors(mDefaultProperties.Favorite.color, mSelectedProperties.Favorite.color, mSelectedZoomPercent);
		mMixedProperties.Favorite.colorEnd = mixColors(mDefaultProperties.Favorite.colorEnd, mSelectedProperties.Favorite.colorEnd, mSelectedZoomPercent);
		mMixedProperties.Favorite.reflexion = mixVectors(mDefaultProperties.Favorite.reflexion, mSelectedProperties.Favorite.reflexion, mSelectedZoomPercent);
	}

	if (mDefaultProperties.ImageOverlay.Loaded)
	{
		mMixedProperties.ImageOverlay.pos = mixVectors(mDefaultProperties.ImageOverlay.pos, mSelectedProperties.ImageOverlay.pos, mSelectedZoomPercent);
		mMixedProperties.ImageOverlay.size = mixVectors(mDefaultProperties.ImageOverlay.size, mSelectedProperties.ImageOverlay.size, mSelectedZoomPercent);
		mMixedProperties.ImageOverlay.origin = mixVectors(mDefaultProperties.ImageOverlay.origin, mSelectedProperties.ImageOverlay.origin, mSelectedZoomPercent);
		mMixedProperties.ImageOverlay.color = mixColors(mDefaultProperties.ImageOverlay.color, mSelectedProperties.ImageOverlay.color, mSelectedZoomPercent);
		mMixedProperties.ImageOverlay.colorEnd = mixColors(mDefaultProperties.ImageOverlay.colorEnd, mSelectedProperties.ImageOverlay.colorEnd, mSelectedZoomPercent);
		mMixedProperties.ImageOverlay.reflexion = mixVectors(mDefaultProperties.ImageOverlay.reflexion, mSelectedProperties.ImageOverlay.reflexion, mSelectedZoomPercent);
	}
	
	return mMixedProperties;
}

Vector3f GridTileComponent::getBackgroundPosition()
{
	return Vector3f(mBackground.getPosition().x() + mPosition.x(), mBackground.getPosition().y() + mPosition.y(), 0);
}