#include "GridTileComponent.h"

#include "resources/TextureResource.h"
#include "ThemeData.h"
#include "Renderer.h"

#include <algorithm>

#include "animations/LambdaAnimation.h"


#ifdef _RPI_
#include "components/VideoPlayerComponent.h"
#endif
#include "components/VideoVlcComponent.h"
#include "utils/FileSystemUtil.h"
#ifdef _RPI_
#include "Settings.h"
#endif

#define VIDEODELAY	100

GridTileComponent::GridTileComponent(Window* window) : GuiComponent(window), mBackground(window), mLabel(window), mVideo(nullptr), mVideoPlaying(false)
{	
	mSelectedZoomPercent = 1.0f;
	mAnimPosition = Vector3f(0, 0);

	mLabelMerged = false;

	mDefaultProperties.mSize = getDefaultTileSize();
	mDefaultProperties.mPadding = Vector2f(16.0f, 16.0f);
	mDefaultProperties.mImageColor = 0xFFFFFFDD; // 0xAAAAAABB;
	mDefaultProperties.mBackgroundImage = ":/frame.png";
	mDefaultProperties.mBackgroundCornerSize = Vector2f(16 ,16);
	mDefaultProperties.mBackgroundCenterColor = 0xAAAAEEFF;
	mDefaultProperties.mBackgroundEdgeColor = 0xAAAAEEFF;

	mSelectedProperties.mSize = getSelectedTileSize();
	mSelectedProperties.mPadding = mDefaultProperties.mPadding;
	mSelectedProperties.mImageColor = 0xFFFFFFFF;
	mSelectedProperties.mBackgroundImage = mDefaultProperties.mBackgroundImage;
	mSelectedProperties.mBackgroundCornerSize = mDefaultProperties.mBackgroundCornerSize;
	mSelectedProperties.mBackgroundCenterColor = 0xFFFFFFFF;
	mSelectedProperties.mBackgroundEdgeColor = 0xFFFFFFFF;

	mDefaultProperties.mLabelSize = Vector2f(1.0, 0.30);
	mDefaultProperties.mLabelColor = 0xFFFFFFFF;
	mDefaultProperties.mLabelBackColor = 0;
	
	mSelectedProperties.mLabelSize = Vector2f(1.0, 0.30);
	mSelectedProperties.mLabelColor = 0xFFFFFFFF;
	mSelectedProperties.mLabelBackColor = 0;

	mImage = std::make_shared<ImageComponent>(mWindow);
	mImage->setOrigin(0.5f, 0.5f);

	addChild(&mBackground);
	addChild(&(*mImage));
	addChild(&mLabel);
		
	// Create the correct type of video window
#ifdef _RPI_
	if (Settings::getInstance()->getBool("VideoOmxPlayer"))
		mVideo = new VideoPlayerComponent(window, "");
	else
#endif
		mVideo = new VideoVlcComponent(window, getTitlePath());

	// video
	mVideo->setOrigin(0.5f, 0.5f);
	mVideo->setStartDelay(VIDEODELAY);
	mVideo->setDefaultZIndex(30);
	addChild(mVideo);
	
	setSelected(false);
	setVisible(true);
}

void GridTileComponent::forceSize(Vector2f size, float selectedZoom)
{
	mDefaultProperties.mSize = size;
	mSelectedProperties.mSize = size * selectedZoom;
}

GridTileComponent::~GridTileComponent()
{
	if (mVideo != nullptr)
		delete mVideo;

	mVideo = nullptr;
}

void GridTileComponent::resize()
{
	const GridTileProperties& currentProperties = getCurrentProperties();

	Vector2f size = currentProperties.mSize;
	setSize(size);

	float height = (int) (size.y() * currentProperties.mLabelSize.y());
	float labelHeight = height;

	mLabel.setColor(currentProperties.mLabelColor);
	mLabel.setBackgroundColor(currentProperties.mLabelBackColor);

	if (mLabelMerged)
	{
		mLabel.setPosition(currentProperties.mPadding.x(), mSize.y() - height - currentProperties.mPadding.y());
		mLabel.setSize(size.x() - 2 * currentProperties.mPadding.x(), height);
	}
	else
	{		
 		mLabel.setPosition(0, mSize.y() - height);
		mLabel.setSize(size.x(), height);
	}

	if (!mLabelVisible || mLabelMerged)
		height = 0;

	if (currentProperties.mLabelSize.x() == 0)
		height = 0;

	float topPadding = currentProperties.mPadding.y();
	float bottomPadding = std::max(topPadding, height);
	float paddingX = currentProperties.mPadding.x();
	
	if (mSelectedZoomPercent != 1.0f)
	{
		if (mAnimPosition.x() != 0 && mAnimPosition.y() != 0 && mSelected)
		{
			int x = mAnimPosition.x() - mPosition.x();
			int y = mAnimPosition.y() - mPosition.y();

			x = x * (1.0 - mSelectedZoomPercent);
			y = y * (1.0 - mSelectedZoomPercent);

			mBackground.setPosition(x, y);
		}
		else 
			mBackground.setPosition(0, 0);
	}
	else
		mBackground.setPosition(0, 0);

	float imageWidth = size.x() - paddingX * 2.0;
	float imageHeight = size.y() - topPadding - bottomPadding;

	if (mImage != nullptr)
	{
		mImage->setOrigin(0.5f, 0.5f);
		mImage->setPosition(size.x() / 2.0f, (size.y() - height) / 2.0f);
		
		if (currentProperties.mImageSizeMode == "minSize")
			mImage->setMinSize(imageWidth, imageHeight);
		else if (currentProperties.mImageSizeMode == "size")
			mImage->setSize(imageWidth, imageHeight);
		else
			mImage->setMaxSize(imageWidth, imageHeight);

		if (mLabelMerged)
		{
			mLabel.setPosition(mImage->getPosition().x() - mImage->getSize().x() / 2, currentProperties.mPadding.y() + mImage->getSize().y() - labelHeight);
			mLabel.setSize(mImage->getSize().x(), labelHeight);
		}
		else if (currentProperties.mPadding.x() == 0)
		{
			mLabel.setPosition(mImage->getPosition().x() - mImage->getSize().x() / 2, mImage->getSize().y());
			mLabel.setSize(mImage->getSize().x(), labelHeight);
		}
	}

	if (mVideo != nullptr)
	{
		mVideo->setOrigin(0.5f, 0.5f);
		mVideo->setPosition(size.x() / 2.0f, (size.y() - height) / 2.0f);

		if (currentProperties.mImageSizeMode == "minSize")
		{
			auto vs = mVideo->getVideoSize();

			double prop = vs == Vector2f(0,0) ? 640.0 / 480.0 : vs.x() / vs.y();
			double imgprop = imageWidth / imageHeight;

			if (prop < imgprop)
			{
				double h = imageWidth / prop; // suppose video is 4:3

				if (h < imageHeight)
					h = imageHeight;

				mVideo->setSize(imageWidth, h);
			}
			else
			{
				double w = imageHeight / prop; // suppose video is 4:3

				if (w < imageWidth)
					w = imageWidth;

				mVideo->setSize(w, imageHeight);
			}
		}
		else 
		if (currentProperties.mImageSizeMode == "size")
			mVideo->setSize(imageWidth, size.y() - topPadding - bottomPadding );
		else
			mVideo->setMaxSize(imageWidth, size.y() - topPadding - bottomPadding);
	}

	if (!mLabelMerged && currentProperties.mImageSizeMode == "minSize")
		mBackground.setSize(size.x(), size.y() - bottomPadding + topPadding);
	else
		mBackground.setSize(size);

	mBackground.setCornerSize(currentProperties.mBackgroundCornerSize);
}

void GridTileComponent::renderBackground(const Transform4x4f& parentTrans)
{
	if (!mVisible)
		return;

	Transform4x4f trans = getTransform() * parentTrans;

	Vector2f clipPos(trans.translation().x(), trans.translation().y());
	if (!Renderer::isVisibleOnScreen(clipPos.x(), clipPos.y(), mSize.x(), mSize.y()))
		return;

	if (mBackground.getCornerSize().x() == 0)
	{
		Renderer::setMatrix(trans);
		Renderer::drawRect(mBackground.getPosition().x(), mBackground.getPosition().y(), mBackground.getSize().x() - 1, mBackground.getSize().y() - 1, mBackground.getCenterColor());
		Renderer::setMatrix(parentTrans);
	}
	else
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

	float padding = getCurrentProperties().mPadding.x();
	float topPadding = getCurrentProperties().mPadding.y();
	float bottomPadding = topPadding;

	if (mLabelVisible && !mLabelMerged)
		bottomPadding = std::max((int)topPadding, (int)(mSize.y() * getCurrentProperties().mLabelSize.y()));

	Vector2i pos((int)Math::round(trans.translation()[0] + padding), (int)Math::round(trans.translation()[1] + topPadding));
	Vector2i size((int)Math::round(mSize.x() - 2 * padding), (int)Math::round(mSize.y() - topPadding - bottomPadding));
	
	if (getCurrentProperties().mImageSizeMode == "minSize")
		Renderer::pushClipRect(pos, size);

	if (mImage != NULL)
		mImage->render(trans);

	if (mSelected && !mVideoPath.empty() && mVideo != nullptr)
		mVideo->render(trans);

	if (!mLabelMerged && getCurrentProperties().mImageSizeMode == "minSize")
		Renderer::popClipRect();

	if (mLabelVisible && getCurrentProperties().mLabelSize.y()>0)
		mLabel.render(trans);

	if (mLabelMerged && getCurrentProperties().mImageSizeMode == "minSize")
		Renderer::popClipRect();
}

void GridTileComponent::render(const Transform4x4f& parentTrans)
{
	if (!mVisible)
		return;

	renderBackground(parentTrans);
	renderContent(parentTrans);
}

// Update all the tile properties to the new status (selected or default)
void GridTileComponent::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	const GridTileProperties& currentProperties = getCurrentProperties();

	mBackground.setImagePath(currentProperties.mBackgroundImage);

	if (mImage != NULL)
		mImage->setColorShift(currentProperties.mImageColor);

	mBackground.setCenterColor(currentProperties.mBackgroundCenterColor);
	mBackground.setEdgeColor(currentProperties.mBackgroundEdgeColor);

	resize();
}

void GridTileComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	// Apply theme to the default gridtile
	const ThemeData::ThemeElement* elem = theme->getElement(view, "default", "gridtile");
	if (elem)
	{		
		if (elem->has("size"))
			mDefaultProperties.mSize = elem->get<Vector2f>("size") * screen;

		if (elem->has("padding"))
			mDefaultProperties.mPadding = elem->get<Vector2f>("padding");

		if (elem->has("imageColor"))
			mDefaultProperties.mImageColor = elem->get<unsigned int>("imageColor");

		if (elem->has("backgroundImage"))
			mDefaultProperties.mBackgroundImage = elem->get<std::string>("backgroundImage");

		if (elem->has("backgroundCornerSize"))
			mDefaultProperties.mBackgroundCornerSize = elem->get<Vector2f>("backgroundCornerSize");

		if (elem->has("backgroundColor"))
		{
			mDefaultProperties.mBackgroundCenterColor = elem->get<unsigned int>("backgroundColor");
			mDefaultProperties.mBackgroundEdgeColor = elem->get<unsigned int>("backgroundColor");
		}

		if (elem->has("backgroundCenterColor"))
			mDefaultProperties.mBackgroundCenterColor = elem->get<unsigned int>("backgroundCenterColor");

		if (elem->has("backgroundEdgeColor"))
			mDefaultProperties.mBackgroundEdgeColor = elem->get<unsigned int>("backgroundEdgeColor");

		if (elem && elem->has("imageSizeMode"))
			mDefaultProperties.mImageSizeMode = elem->get<std::string>("imageSizeMode");
	}

	// Apply theme to the selected gridtile
	// NOTE that some of the default gridtile properties influence on the selected gridtile properties
	// See THEMES.md for more informations
	elem = theme->getElement(view, "selected", "gridtile");

	mSelectedProperties.mSize = elem && elem->has("size") ?
								elem->get<Vector2f>("size") * screen :
								mDefaultProperties.mSize;
								//getSelectedTileSize();

	mSelectedProperties.mPadding = elem && elem->has("padding") ?
								   elem->get<Vector2f>("padding") :
								   mDefaultProperties.mPadding;

	if (elem && elem->has("imageColor"))
		mSelectedProperties.mImageColor = elem->get<unsigned int>("imageColor");

	mSelectedProperties.mBackgroundImage = elem && elem->has("backgroundImage") ?
										   elem->get<std::string>("backgroundImage") :
										   mDefaultProperties.mBackgroundImage;

	mSelectedProperties.mBackgroundCornerSize = elem && elem->has("backgroundCornerSize") ?
												elem->get<Vector2f>("backgroundCornerSize") :
												mDefaultProperties.mBackgroundCornerSize;

	if (elem && elem->has("backgroundColor"))
	{
		mSelectedProperties.mBackgroundCenterColor = elem->get<unsigned int>("backgroundColor");
		mSelectedProperties.mBackgroundEdgeColor = elem->get<unsigned int>("backgroundColor");
	}

	if (elem && elem->has("backgroundCenterColor"))
		mSelectedProperties.mBackgroundCenterColor = elem->get<unsigned int>("backgroundCenterColor");

	if (elem && elem->has("backgroundEdgeColor"))
		mSelectedProperties.mBackgroundEdgeColor = elem->get<unsigned int>("backgroundEdgeColor");

	mSelectedProperties.mImageSizeMode = elem && elem->has("imageSizeMode") ?
		elem->get<std::string>("imageSizeMode") :
		mDefaultProperties.mImageSizeMode;

	elem = theme->getElement(view, "gridtile", "text");
	if (elem != NULL)
	{
		if (elem && elem->has("size"))
		{
			mDefaultProperties.mLabelSize = elem->get<Vector2f>("size");
			mSelectedProperties.mLabelSize = mDefaultProperties.mLabelSize;
			mLabelMerged = mDefaultProperties.mLabelSize.x() == 0;
		}

		if (elem && elem->has("color"))
		{
			mDefaultProperties.mLabelColor = elem->get<unsigned int>("color");
			mSelectedProperties.mLabelColor = mDefaultProperties.mLabelColor;			
		}

		if (elem && elem->has("backgroundColor"))
		{
			mDefaultProperties.mLabelBackColor = elem->get<unsigned int>("backgroundColor");
			mSelectedProperties.mLabelBackColor = mDefaultProperties.mLabelBackColor;			
		}

		mLabelVisible = true;
		mLabel.applyTheme(theme, view, element, properties);

		elem = theme->getElement(view, "gridtile_selected", "text");
		if (elem != NULL)
		{
			if (elem && elem->has("size"))
				mSelectedProperties.mLabelSize = elem->get<Vector2f>("size");

			if (elem && elem->has("color"))
				mSelectedProperties.mLabelColor = elem->get<unsigned int>("color");

			if (elem && elem->has("backgroundColor"))
				mSelectedProperties.mLabelBackColor = elem->get<unsigned int>("backgroundColor");
		}
	}
	else
		mLabelVisible = false;
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

void GridTileComponent::setImage(const std::string& path)
{
	if (mCurrentPath == path)
		return;
	
	mCurrentPath = path;		
	mImage->setImage(path, false, mSize);
	resize();	
}

void GridTileComponent::reset()
{
	setLabel("");
	setVideo("");
	setImage("");
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
			mVideo->setVideo("");
	}
}

void GridTileComponent::setImage(const std::shared_ptr<TextureResource>& texture, std::string name)
{
	mImage->setImage(texture);
	mLabel.setText(name);
	
	// Resize now to prevent flickering images when scrolling
	resize();
}

void GridTileComponent::setSelected(bool selected, bool allowAnimation, Vector3f* pPosition)
{
	if (mSelected == selected)
	{
		if (mSelected && mVideo != nullptr)
			mVideo->setVideo(mVideoPath);

		return;
	}

	mSelected = selected;
	
	if (!mSelected && mVideo != nullptr)
		mVideo->setVideo("");	

	if (selected)
	{
		if (pPosition == NULL || !allowAnimation)
		{
			cancelAnimation(3);

			this->setSelectedZoom(1);
			mAnimPosition = Vector3f(0, 0, 0);

			if (mVideo != NULL)
				mVideo->setVideo(mVideoPath);
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

				if (mVideo != NULL)
					mVideo->setVideo(mVideoPath);

			}, false, 3);
		}
	}
	else // if (!selected)
	{
		if (!allowAnimation)
		{
			this->setSelectedZoom(0);

			if (mVideo != NULL)
				mVideo->setVideo("");
		}
		else
		{
			auto func = [this](float t)
			{
				t -= 1; // cubic ease out
				float pct = Math::lerp(0, 1, t*t*t + 1);
				this->setSelectedZoom(1.0 - pct);
			};

			cancelAnimation(3);
			setAnimation(new LambdaAnimation(func, 250), 0, [this] {
				this->setSelectedZoom(0);

				if (mVideo != NULL)
					mVideo->setVideo("");

			}, false, 3);
		}
	}
}

void GridTileComponent::setSelectedZoom(float percent)
{
	mSelectedZoomPercent = percent;
	resize();
}

void GridTileComponent::setVisible(bool visible)
{
	mVisible = visible;
}

unsigned int mixColors(unsigned int first, unsigned int second, float percent)
{
	unsigned char alpha0 = (first >> 24) & 0xFF;
	unsigned char blue0 = (first >> 16) & 0xFF;
	unsigned char green0 = (first >> 8) & 0xFF;
	unsigned char red0 = first & 0xFF;

	unsigned char alpha1 = (second >> 24) & 0xFF;
	unsigned char blue1 = (second >> 16) & 0xFF;
	unsigned char green1 = (second >> 8) & 0xFF;
	unsigned char red1 = second & 0xFF;

	unsigned char alpha = (unsigned char)(alpha0 * (1.0 - percent) + alpha1 * percent);
	unsigned char blue = (unsigned char)(blue0 * (1.0 - percent) + blue1 * percent);
	unsigned char green = (unsigned char)(green0 * (1.0 - percent) + green1 * percent);
	unsigned char red = (unsigned char)(red0 * (1.0 - percent) + red1 * percent);

	return (alpha << 24) | (blue << 16) | (green << 8) | red;
}

const GridTileProperties& GridTileComponent::getCurrentProperties() 
{
	if (mSelectedZoomPercent != 1.0f)
	{
		auto def = mSelected ? mSelectedProperties : mDefaultProperties;

		mMixedProperties = mSelected ? mSelectedProperties : mDefaultProperties;

		if (mDefaultProperties.mSize != mSelectedProperties.mSize)
		{
			float x = mDefaultProperties.mSize.x() * (1.0 - mSelectedZoomPercent) + mSelectedProperties.mSize.x() * mSelectedZoomPercent;
			float y = mDefaultProperties.mSize.y() * (1.0 - mSelectedZoomPercent) + mSelectedProperties.mSize.y() * mSelectedZoomPercent;
			mMixedProperties.mSize = Vector2f(x, y);
		}

		if (mDefaultProperties.mPadding != mSelectedProperties.mPadding)
		{
			float x = mDefaultProperties.mPadding.x() * (1.0 - mSelectedZoomPercent) + mSelectedProperties.mPadding.x() * mSelectedZoomPercent;
			float y = mDefaultProperties.mPadding.y() * (1.0 - mSelectedZoomPercent) + mSelectedProperties.mPadding.y() * mSelectedZoomPercent;
			mMixedProperties.mPadding = Vector2f(x, y);
		}

		if (mDefaultProperties.mImageColor != mSelectedProperties.mImageColor)
		{
			mMixedProperties.mImageColor = mixColors(mDefaultProperties.mImageColor, mSelectedProperties.mImageColor, mSelectedZoomPercent);
		}

		if (mDefaultProperties.mLabelSize != mSelectedProperties.mLabelSize)
		{
			float y = mDefaultProperties.mLabelSize.y() * (1.0 - mSelectedZoomPercent) + mSelectedProperties.mLabelSize.y() * mSelectedZoomPercent;
			mMixedProperties.mLabelSize = Vector2f(mDefaultProperties.mLabelSize.x(), y);
		}

		return mMixedProperties;
	}

	return mSelected ? mSelectedProperties : mDefaultProperties;
}