#include "GridTileComponent.h"

#include "resources/TextureResource.h"
#include "ThemeData.h"
#include "Renderer.h"

#include <algorithm>

#include "animations/LambdaAnimation.h"

GridTileComponent::GridTileComponent(Window* window) : GuiComponent(window), mBackground(window), mLabel(window)
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

	mLabelSize = Vector2f(1.0, 0.30);

	mImage = std::make_shared<ImageComponent>(mWindow);

	mImage->setOrigin(0.5f, 0.5f);
	
	//mBackground.setOrigin(0.5f, 0.5f);


	//mLabel.setOrigin(0.5f, 0.5f);
	//mLabel.setOrigin(0.5f, 0.0f);
	//mLabel.setSize(1.0f, 1.0f);
	
	addChild(&mBackground);
	addChild(&(*mImage));
	addChild(&mLabel);

	setSelected(false);
	setVisible(true);
}

void GridTileComponent::resize()
{
	const GridTileProperties& currentProperties = getCurrentProperties();

	setSize(currentProperties.mSize);

	float height = currentProperties.mSize.y() * mLabelSize.y();

	if (mLabelMerged)
	{
		mLabel.setPosition(currentProperties.mPadding.x(), mSize.y() - height - currentProperties.mPadding.y());
		mLabel.setSize(currentProperties.mSize.x() - 2 * currentProperties.mPadding.x(), height);
	}
	else
	{
		mLabel.setPosition(0, mSize.y() - height);
		mLabel.setSize(currentProperties.mSize.x(), height);
	}

	if (!mLabelVisible || mLabelMerged)
		height = 0;

	if (mLabelSize.x() == 0)
		height = 0;

	float topPadding = currentProperties.mPadding.y();
	float bottomPadding = std::max(topPadding, height);
	float paddingX = currentProperties.mPadding.x();
	
	if (mSelectedZoomPercent != 1.0f)
	{
		paddingX = mDefaultProperties.mPadding.x() * (1.0 - mSelectedZoomPercent) + mSelectedProperties.mPadding.x() * mSelectedZoomPercent;
		topPadding = mDefaultProperties.mPadding.y() * (1.0 - mSelectedZoomPercent) + mSelectedProperties.mPadding.y() * mSelectedZoomPercent;

		if (mAnimPosition.x() != 0 && mAnimPosition.y() != 0)
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


	if (mImage != NULL)
	{
		mImage->setOrigin(0.5f, 0.5f);
		mImage->setPosition(currentProperties.mSize.x() / 2.0f, (currentProperties.mSize.y() - height) / 2.0f);
		
		if (currentProperties.mImageSizeMode == "minSize")
			mImage->setMinSize(currentProperties.mSize.x() - paddingX * 2, currentProperties.mSize.y() - topPadding - bottomPadding);
		else if (currentProperties.mImageSizeMode == "size")
			mImage->setSize(currentProperties.mSize.x() - paddingX * 2, currentProperties.mSize.y() - topPadding - bottomPadding);
		else
			mImage->setMaxSize(currentProperties.mSize.x() - paddingX * 2, currentProperties.mSize.y() - topPadding - bottomPadding);
	}



	if (!mLabelMerged && currentProperties.mImageSizeMode == "minSize")
		mBackground.setSize(currentProperties.mSize.x(), currentProperties.mSize.y() - bottomPadding);
	else
		mBackground.setSize(currentProperties.mSize);

	mBackground.setCornerSize(currentProperties.mBackgroundCornerSize);
}

void GridTileComponent::render(const Transform4x4f& parentTrans)
{
	if (!mVisible)
		return;

	Transform4x4f trans = getTransform() * parentTrans;

	Vector2f clipPos(trans.translation().x(), trans.translation().y());
	if (!Renderer::isVisibleOnScreen(clipPos.x(), clipPos.y(), mSize.x(), mSize.y()))
		return;

	//Renderer::setMatrix(trans);
	//Renderer::drawRect(0.f, 0.f, mSize.x(), mSize.y(), 0xFF0000FF);	
	
	if (mBackground.getCornerSize().x() == 0)
	{
		
		Renderer::setMatrix(trans);
		Renderer::drawRect(mBackground.getPosition().x(), mBackground.getPosition().y(), mBackground.getSize().x(), mBackground.getSize().y(), mBackground.getCenterColor());
		Renderer::setMatrix(parentTrans);
	}
	else
		mBackground.render(trans);

	if (mImage != NULL)
		mImage->render(trans);
	
	if (mLabelVisible)
		mLabel.render(trans);
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

	elem = theme->getElement(view, element, "text");
	if (elem != NULL)
	{
		if (elem && elem->has("size"))
		{
			mLabelSize = elem->get<Vector2f>("size");
			mLabelMerged = mLabelSize.x() == 0;
		}

		mLabelVisible = true;
		mLabel.applyTheme(theme, view, element, properties);
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

void GridTileComponent::setImage(const std::string& path, std::string name)
{
	if (mCurrentPath != path)
	{
		mCurrentPath = path;		
		mImage->setImage(path);
	}
			
	mLabel.setText(name);
	resize();
}

void GridTileComponent::setImage(const std::shared_ptr<TextureResource>& texture, std::string name)
{
	mImage->setImage(texture);
	mLabel.setText(name);
	
	// Resize now to prevent flickering images when scrolling
	resize();
}

void GridTileComponent::setSelected(bool selected, Vector3f* pPosition)
{
	mSelected = selected;

	if (selected && pPosition != NULL)
	{
		auto funeec = [this](float t)
		{
			this->setSelectedZoom(t);
		};
	}

	if (selected && (pPosition != NULL || mSelectedProperties.mPadding.x() != mDefaultProperties.mPadding.y()))
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

		setAnimation(new LambdaAnimation(func, 200), 0, [this] {
			this->setSelectedZoom(1);
			mAnimPosition = Vector3f(0, 0, 0);
		}, false, 3);
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

const GridTileProperties& GridTileComponent::getCurrentProperties() const
{
	return mSelected ? mSelectedProperties : mDefaultProperties;
}