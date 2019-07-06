#pragma once
#ifndef ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H
#define ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H

#include "Log.h"
#include "components/IList.h"
#include "resources/TextureResource.h"
#include "GridTileComponent.h"
#include "Settings.h"

#define EXTRAITEMS 2

enum ScrollDirection
{
	SCROLL_VERTICALLY,
	SCROLL_HORIZONTALLY,
	SCROLL_VERTICALLY_CENTER,
	SCROLL_HORIZONTALLY_CENTER,
};

enum ImageSource
{
	THUMBNAIL,
	IMAGE,
	MARQUEE
};

struct ImageGridData
{
	std::string texturePath;
	std::string videoPath;
};

template<typename T>
class ImageGridComponent : public IList<ImageGridData, T>
{
protected:
	using IList<ImageGridData, T>::mEntries;
	using IList<ImageGridData, T>::mScrollTier;
	using IList<ImageGridData, T>::listUpdate;
	using IList<ImageGridData, T>::listInput;
	using IList<ImageGridData, T>::listRenderTitleOverlay;
	using IList<ImageGridData, T>::getTransform;
	using IList<ImageGridData, T>::mSize;
	using IList<ImageGridData, T>::mCursor;
	using IList<ImageGridData, T>::Entry;
	using IList<ImageGridData, T>::mWindow;

public:
	using IList<ImageGridData, T>::size;
	using IList<ImageGridData, T>::isScrolling;
	using IList<ImageGridData, T>::stopScrolling;

	ImageGridComponent(Window* window);

	void add(const std::string& name, const std::string& imagePath, const std::string& videoPath, const T& obj);

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

	void onSizeChanged() override;
	inline void setCursorChangedCallback(const std::function<void(CursorState state)>& func) { mCursorChangedCallback = func; }

	void setThemeName(std::string name) { mName = name; };

	virtual void topWindow(bool isTop);
	virtual void onShow();
	virtual void onHide();

	ImageSource		getImageSource() { return mImageSource; };

	void setGridSizeOverride(Vector2f size);

protected:
	virtual void onCursorChanged(const CursorState& state) override;	

private:
	// TILES
	void buildTiles();
	void updateTiles(bool ascending = true, bool allowAnimation = true, bool updateSelectedState = true);
	void updateTileAtPos(int tilePos, int imgPos, bool allowAnimation = true, bool updateSelectedState = true);
	void calcGridDimension();

	bool isVertical() { return mScrollDirection == SCROLL_VERTICALLY || mScrollDirection == SCROLL_VERTICALLY_CENTER; };

	bool mEntriesDirty;
	int mLastCursor;
	std::string mDefaultGameTexture;
	std::string mDefaultFolderTexture;

	// TILES
	bool mLastRowPartial;
	Vector2f mAutoLayout;
	float mAutoLayoutZoom;
	Vector4f mPadding;
	Vector2f mMargin;
	Vector2f mTileSize;
	Vector2i mGridDimension;
	Vector2f mGridSizeOverride;

	std::shared_ptr<ThemeData> mTheme;
	std::vector< std::shared_ptr<GridTileComponent> > mTiles;

	std::string mName;

	int mStartPosition;
	bool mAllowVideo;
	float mVideoDelay;

	float mCamera;
	float mCameraDirection;

	// MISCELLANEOUS
	ScrollDirection mScrollDirection;
	ImageSource		mImageSource;

	std::function<void(CursorState state)> mCursorChangedCallback;
};

template<typename T>
ImageGridComponent<T>::ImageGridComponent(Window* window) : IList<ImageGridData, T>(window)
{
	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	mCamera = 0.0;
	mCameraDirection = 1.0;

	mGridSizeOverride = Vector2f(0, 0);
	mAutoLayout = Vector2f(0, 0);
	mAutoLayoutZoom = 1.0;

	mVideoDelay = 750;
	mAllowVideo = false;
	mName = "grid";
	mStartPosition = 0;
	mEntriesDirty = true;
	mLastCursor = 0;
	mDefaultGameTexture = ":/cartridge.svg";
	mDefaultFolderTexture = ":/folder.svg";

	mSize = screen * 0.80f;
	mMargin = screen * 0.07f;
	mPadding = Vector4f(0, 0, 0, 0);
	mTileSize = GridTileComponent::getDefaultTileSize();

	mImageSource = THUMBNAIL;
	mScrollDirection = SCROLL_VERTICALLY;
}

template<typename T>
void ImageGridComponent<T>::add(const std::string& name, const std::string& imagePath, const std::string& videoPath, const T& obj)
{
	typename IList<ImageGridData, T>::Entry entry;
	entry.name = name;
	entry.object = obj;
	entry.data.texturePath = imagePath;
	entry.data.videoPath = videoPath;

	static_cast<IList< ImageGridData, T >*>(this)->add(entry);
	mEntriesDirty = true;
}

template<typename T>
bool ImageGridComponent<T>::input(InputConfig* config, Input input)
{
	if(input.value != 0)
	{
		int idx = isVertical() ? 0 : 1;

		Vector2i dir = Vector2i::Zero();
		if(config->isMappedLike("up", input))
			dir[1 ^ idx] = -1;
		else if(config->isMappedLike("down", input))
			dir[1 ^ idx] = 1;
		else if(config->isMappedLike("left", input))
			dir[0 ^ idx] = -1;
		else if(config->isMappedLike("right", input))
			dir[0 ^ idx] = 1;

		if(dir != Vector2i::Zero())
		{
			if (isVertical())
				listInput(dir.x() + dir.y() * mGridDimension.x());
			else
				listInput(dir.x() + dir.y() * mGridDimension.y());

			return true;
		}
	}else{
		if(config->isMappedLike("up", input) || config->isMappedLike("down", input) || config->isMappedLike("left", input) || config->isMappedLike("right", input))
		{
			stopScrolling();
		}
	}

	return GuiComponent::input(config, input);
}

template<typename T>
void ImageGridComponent<T>::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	listUpdate(deltaTime);
	
	for(auto it = mTiles.begin(); it != mTiles.end(); it++)
		(*it)->update(deltaTime);
}

template<typename T>
void ImageGridComponent<T>::topWindow(bool isTop)
{
	GuiComponent::topWindow(isTop);

	for (int ti = 0; ti < (int)mTiles.size(); ti++)
	{
		std::shared_ptr<GridTileComponent> tile = mTiles.at(ti);
		tile->topWindow(isTop);
	}
}

template<typename T>
void ImageGridComponent<T>::onShow()
{
	if (mEntriesDirty)
	{
		updateTiles();
		mEntriesDirty = false;
	}

	GuiComponent::onShow();

	for (int ti = 0; ti < (int)mTiles.size(); ti++)
	{
		std::shared_ptr<GridTileComponent> tile = mTiles.at(ti);
		tile->onShow();
	}
}

template<typename T>
void ImageGridComponent<T>::setGridSizeOverride(Vector2f size)
{
	mGridSizeOverride = size;
}

template<typename T>
void ImageGridComponent<T>::onHide()
{
	GuiComponent::onHide();

	for (int ti = 0; ti < (int)mTiles.size(); ti++)
	{
		std::shared_ptr<GridTileComponent> tile = mTiles.at(ti);
		tile->onHide();
	}
}

template<typename T>
void ImageGridComponent<T>::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = getTransform() * parentTrans;

	Vector2f clipPos(trans.translation().x(), trans.translation().y());
	if (!Renderer::isVisibleOnScreen(clipPos.x(), clipPos.y(), mSize.x(), mSize.y()))
		return;

	if (Settings::getInstance()->getBool("DebugGrid"))
	{
		Renderer::setMatrix(trans);
		Renderer::drawRect(0.0f, 0.0f, mSize.x(), mSize.y(), 0xFF000033);
		Renderer::setMatrix(parentTrans);
	}

	float offsetX = isVertical() ? 0 : mCamera * mCameraDirection * (mTileSize.x() + mMargin.x());
	float offsetY = isVertical() ? mCamera * mCameraDirection * (mTileSize.y() + mMargin.y()) : 0;
	
	if (mEntriesDirty)
	{
		updateTiles();
		mEntriesDirty = false;
	}

	// Create a clipRect to hide tiles used to buffer texture loading
	float scaleX = trans.r0().x();
	float scaleY = trans.r1().y();

	Vector2i pos((int)Math::round(trans.translation()[0]), (int)Math::round(trans.translation()[1]));
	Vector2i size((int)Math::round(mSize.x() * scaleX), (int)Math::round(mSize.y() * scaleY));

	if (Settings::getInstance()->getBool("DebugGrid"))
	{
		for (auto it = mTiles.begin(); it != mTiles.end(); it++)
		{
			std::shared_ptr<GridTileComponent> tile = (*it);

			auto tt = tile->getTransform() * trans;
			Renderer::setMatrix(tt);
			Renderer::drawRect(0.0, 0.0, tile->getSize().x(), tile->getSize().y(), 0x00FF0033);
		}

		Renderer::setMatrix(parentTrans);
	}

	Renderer::pushClipRect(pos, size);

	if (mCamera != 0)
	{
		for (auto it = mTiles.begin(); it != mTiles.end(); it++)
			(*it)->setPosition((*it)->getPosition().x() + offsetX, (*it)->getPosition().y() + offsetY);
	}

	

	// Render the selected image background on bottom of the others if needed
	std::shared_ptr<GridTileComponent> selectedTile = NULL;
	for(auto it = mTiles.begin(); it != mTiles.end(); it++)
	{
		std::shared_ptr<GridTileComponent> tile = (*it);
		if (tile->isSelected())
		{
			selectedTile = tile;
			if (tile->shouldSplitRendering())
				tile->renderBackground(trans);

			break;
		}
	}

	for (auto it = mTiles.begin(); it != mTiles.end(); it++)
	{
		std::shared_ptr<GridTileComponent> tile = (*it);
		if (!tile->isSelected())
			tile->render(trans);
	}

	// Render the selected image content on top of the others
	if (selectedTile != NULL)
	{
		if (selectedTile->shouldSplitRendering())
			selectedTile->renderContent(trans);
		else 
			selectedTile->render(trans);
	}

	Renderer::popClipRect();

	listRenderTitleOverlay(trans);

	if (mCamera != 0)
	{
		for (auto it = mTiles.begin(); it != mTiles.end(); it++)
			(*it)->setPosition((*it)->getPosition().x() - offsetX, (*it)->getPosition().y() - offsetY);
	}

	GuiComponent::renderChildren(trans);
}

template<typename T>
void ImageGridComponent<T>::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	// Apply theme to GuiComponent but not size property, which will be applied at the end of this function
	GuiComponent::applyTheme(theme, view, element, properties ^ ThemeFlags::SIZE);

	// Keep the theme pointer to apply it on the tiles later on
	mTheme = theme;

	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "imagegrid");
	if (elem)
	{
		if (elem->has("margin"))
			mMargin = elem->get<Vector2f>("margin") * screen;

		if (elem->has("padding"))
			mPadding = elem->get<Vector4f>("padding") * Vector4f(screen.x(), screen.y(), screen.x(), screen.y());

		if (elem->has("autoLayout"))
			mAutoLayout = elem->get<Vector2f>("autoLayout");		

		if (elem->has("autoLayoutSelectedZoom"))
			mAutoLayoutZoom = elem->get<float>("autoLayoutSelectedZoom");

		if (elem->has("imageSource"))
		{
			auto direction = elem->get<std::string>("imageSource");
			if (direction == "image")
				mImageSource = IMAGE;
			else if (direction == "marquee")
				mImageSource = MARQUEE;
			else
				mImageSource = THUMBNAIL;
		}
		else 
			mImageSource = THUMBNAIL;

		if (elem->has("scrollDirection"))
		{
			auto direction = elem->get<std::string>("scrollDirection");
			if (direction == "horizontal")
				mScrollDirection = SCROLL_HORIZONTALLY;
			else if (direction == "horizontalCenter")
				mScrollDirection = SCROLL_HORIZONTALLY_CENTER;
			else if (direction == "verticalCenter")
				mScrollDirection = SCROLL_VERTICALLY_CENTER;
			else 
				mScrollDirection = SCROLL_VERTICALLY;
		}

		if (elem->has("showVideoAtDelay"))
		{
			mVideoDelay = elem->get<float>("showVideoAtDelay");
			mAllowVideo = true;
		}
		else
			mAllowVideo = false;

		if (elem->has("gameImage"))
		{
			std::string path = elem->get<std::string>("gameImage");

			if (!ResourceManager::getInstance()->fileExists(path))
				LOG(LogWarning) << "Could not replace default game image, check path: " << path;
			else
			{
				std::string oldDefaultGameTexture = mDefaultGameTexture;
				mDefaultGameTexture = path;

				// mEntries are already loaded at this point,
				// so we need to update them with new game image texture
				for (auto it = mEntries.begin(); it != mEntries.end(); it++)
				{
					if ((*it).data.texturePath == oldDefaultGameTexture)
						(*it).data.texturePath = mDefaultGameTexture;
				}
			}
		}

		if (elem->has("folderImage"))
		{
			std::string path = elem->get<std::string>("folderImage");

			if (!ResourceManager::getInstance()->fileExists(path))
				LOG(LogWarning) << "Could not replace default folder image, check path: " << path;
			else
			{
				std::string oldDefaultFolderTexture = mDefaultFolderTexture;
				mDefaultFolderTexture = path;

				// mEntries are already loaded at this point,
				// so we need to update them with new folder image texture
				for (auto it = mEntries.begin(); it != mEntries.end(); it++)
				{
					if ((*it).data.texturePath == oldDefaultFolderTexture)
						(*it).data.texturePath = mDefaultFolderTexture;
				}
			}
		}
	}

	// We still need to manually get the grid tile size here,
	// so we can recalculate the new grid dimension, and THEN (re)build the tiles
	elem = theme->getElement(view, "default", "gridtile");

	mTileSize = elem && elem->has("size") ?
				elem->get<Vector2f>("size") * screen :
				GridTileComponent::getDefaultTileSize();

	// Apply size property, will trigger a call to onSizeChanged() which will build the tiles
	GuiComponent::applyTheme(theme, view, element, ThemeFlags::SIZE | ThemeFlags::Z_INDEX);

	// Trigger the call manually if the theme have no "imagegrid" element
	if (!elem)
		buildTiles();
}

template<typename T>
void ImageGridComponent<T>::onSizeChanged()
{
	buildTiles();
	updateTiles();
}

template<typename T>
void ImageGridComponent<T>::onCursorChanged(const CursorState& state)
{


	if (mLastCursor == mCursor)
	{
		if (state == CURSOR_STOPPED && mCursorChangedCallback)
			mCursorChangedCallback(state);

		TRACE("skip ImageGridComponent<T>::onCursorChanged " << state);

		return;
	}
	
	TRACE("ImageGridComponent<T>::onCursorChanged " << state);

	bool centerSel = (mScrollDirection == SCROLL_HORIZONTALLY_CENTER || mScrollDirection == SCROLL_VERTICALLY_CENTER);

	bool direction = mCursor >= mLastCursor;

	int oldStart = mStartPosition;

	float dimScrollable = isVertical() ? mGridDimension.y() - 2 * EXTRAITEMS : mGridDimension.x() - 2 * EXTRAITEMS;
	float dimOpposite = isVertical() ? mGridDimension.x() : mGridDimension.y();

	int centralCol = (int)(dimScrollable - 0.5) / 2;
	int maxCentralCol = (int)(dimScrollable) / 2;

	int oldCol = (mLastCursor / dimOpposite);
	int col = (mCursor / dimOpposite);

	int lastCol = ((mEntries.size() - 1) / dimOpposite);

	int lastScroll = std::max(0, (int)(lastCol + 1 - dimScrollable));

	float startPos = 0;
	float endPos = 1;

	if (((GuiComponent*)this)->isAnimationPlaying(2))
	{		
		/*
		startPos = -(mCamera * 0.75);

		if (startPos > 1)
			startPos = 1;
		else if (startPos < -1)
			startPos = -1;
		*/	
		startPos = 0;
		((GuiComponent*)this)->cancelAnimation(2);
		updateTiles(direction, false, !GuiComponent::ALLOWANIMATIONS);		
	}
	
	if (GuiComponent::ALLOWANIMATIONS)
	{
		std::shared_ptr<GridTileComponent> oldTile = nullptr;
		std::shared_ptr<GridTileComponent> newTile = nullptr;

		int oldIdx = mLastCursor - mStartPosition + (dimOpposite * EXTRAITEMS);
		if (oldIdx >= 0 && oldIdx < mTiles.size())
			oldTile = mTiles[oldIdx];

		int newIdx = mCursor - mStartPosition + (dimOpposite * EXTRAITEMS);
		if (newIdx >= 0 && newIdx < mTiles.size())
			newTile = mTiles[newIdx];

		for (auto it = mTiles.begin(); it != mTiles.end(); it++)
		{
			if ((*it)->isSelected() && *it != oldTile && *it != newTile)
			{
				startPos = 0;
				(*it)->setSelected(false, false, nullptr);
			}
		}

		Vector3f oldPos = Vector3f(0, 0);

		if (oldTile != nullptr && oldTile != newTile)
		{
			oldPos = oldTile->getBackgroundPosition();
			oldTile->setSelected(false, true, nullptr, true);
		}

		if (newTile != nullptr)
			newTile->setSelected(true, true, oldPos == Vector3f(0, 0) ? nullptr : &oldPos, true);
	}
	
	int firstVisibleCol = mStartPosition / dimOpposite;

	if ((col < centralCol || (col == 0 && col == centralCol)) && !centerSel)
		mStartPosition = 0;
	else if ((col - centralCol) > lastScroll && !centerSel)
		mStartPosition = lastScroll * dimOpposite;
	else if (maxCentralCol != centralCol && col == firstVisibleCol + maxCentralCol || col == firstVisibleCol + centralCol)
	{
		if (col == firstVisibleCol + maxCentralCol)
			mStartPosition = (col - maxCentralCol) * dimOpposite;
		else
			mStartPosition = (col - centralCol) * dimOpposite;
	}
	else
	{
		if (oldCol == firstVisibleCol + maxCentralCol)
			mStartPosition = (col - maxCentralCol) * dimOpposite;
		else
			mStartPosition = (col - centralCol) * dimOpposite;
	}

	auto lastCursor = mLastCursor;
	mLastCursor = mCursor;

	mCameraDirection = direction ? -1.0 : 1.0;
	mCamera = 0;

	if (lastCursor < 0 || !GuiComponent::ALLOWANIMATIONS)
	{
		updateTiles(direction, lastCursor >= 0 && GuiComponent::ALLOWANIMATIONS);

		if (mCursorChangedCallback)
			mCursorChangedCallback(state);

		return;
	}

	if (mCursorChangedCallback)
		mCursorChangedCallback(state);

	bool moveCamera = (oldStart != mStartPosition);

	auto func = [this, startPos, endPos, moveCamera](float t)
	{
		if (!moveCamera)
			return;

		t -= 1; // cubic ease out
		float pct = Math::lerp(0, 1, t*t*t + 1);
		t = startPos * (1.0 - pct) + endPos * pct;

		mCamera = t;
	};

	((GuiComponent*)this)->setAnimation(new LambdaAnimation(func, 250), 0, [this, direction] {
		mCamera = 0;
		updateTiles(direction, false);
	}, false, 2);
}


template<typename T>
void ImageGridComponent<T>::updateTiles(bool ascending, bool allowAnimation, bool updateSelectedState = true)
{
	if (!mTiles.size())
		return;

	// Stop updating the tiles at highest scroll speed
	if (mScrollTier == 3)
	{
		for (int ti = 0; ti < (int)mTiles.size(); ti++)
		{
			std::shared_ptr<GridTileComponent> tile = mTiles.at(ti);
			
			tile->setSelected(false, allowAnimation);
			tile->setLabel("");
			tile->setImage(mDefaultGameTexture);
			tile->setVisible(false);
		}
		return;
	}

	// Temporary store previous texture so they can't be unloaded
	std::vector<std::shared_ptr<TextureResource>> previousTextures;
	for (int ti = 0; ti < (int)mTiles.size(); ti++)
	{
		std::shared_ptr<GridTileComponent> tile = mTiles.at(ti);
		previousTextures.push_back(tile->getTexture());
	}

	if (!ascending)
	{
		int i = (int)mTiles.size() - 1;
		int end = -1;
		int img = mStartPosition + (int)mTiles.size() - 1;
		
		if (isVertical())
			img -= EXTRAITEMS * mGridDimension.x();
		else
			img -= EXTRAITEMS * mGridDimension.y();

		while (i != end)
		{
			updateTileAtPos(i, img, allowAnimation, updateSelectedState);
			i--; img--;
		}
	}
	else
	{
		int i = 0;
		int end = (int)mTiles.size();
		int img = mStartPosition;

		if (isVertical())
			img -= EXTRAITEMS * mGridDimension.x();
		else
			img -= EXTRAITEMS * mGridDimension.y();

		while (i != end)
		{
			updateTileAtPos(i, img, allowAnimation, updateSelectedState);
			i++; img++;
		}
	}

	if (updateSelectedState)
		mLastCursor = mCursor;

	mEntriesDirty = false;
}


template<typename T>
void ImageGridComponent<T>::updateTileAtPos(int tilePos, int imgPos, bool allowAnimation, bool updateSelectedState)
{
	std::shared_ptr<GridTileComponent> tile = mTiles.at(tilePos);

	// If we have more tiles than we have to display images on screen, hide them
	if(imgPos < 0 || imgPos >= size() || tilePos < 0 || tilePos >= (int) mTiles.size()) // Same for tiles out of the buffer
	{
		if (updateSelectedState)
			tile->setSelected(false, allowAnimation);

		tile->reset();
		tile->setVisible(false);
	}
	else
	{
		tile->setVisible(true);

		std::string name = mEntries.at(imgPos).name; // .object->getName();
		tile->setLabel(name);

		std::string imagePath = mEntries.at(imgPos).data.texturePath;

		if (ResourceManager::getInstance()->fileExists(imagePath))
			tile->setImage(imagePath);
		else if (mEntries.at(imgPos).object->getType() == 2)		
			tile->setImage(mDefaultFolderTexture);
		else
			tile->setImage(mDefaultGameTexture);		
		
		if (mAllowVideo && imgPos == mCursor)
		{			
			std::string videoPath = mEntries.at(imgPos).data.videoPath;

			if (ResourceManager::getInstance()->fileExists(videoPath))
				tile->setVideo(videoPath, mVideoDelay);
			else if (mEntries.at(imgPos).object->getType() == 2)
				tile->setVideo("");
			else
				tile->setVideo("");
		}
		else
			tile->setVideo("");
			
		if (updateSelectedState)
		{
			if (imgPos == mCursor && mCursor != mLastCursor)
			{
				int dif = mCursor - tilePos;
				int idx = mLastCursor - dif;

				if (idx < 0 || idx >= mTiles.size())
					idx = 0;

				Vector3f pos = mTiles.at(idx)->getBackgroundPosition();
				tile->setSelected(true, allowAnimation, &pos);
			}
			else
				tile->setSelected(imgPos == mCursor, allowAnimation);
		}
	}
}


// Create and position tiles (mTiles)
template<typename T>
void ImageGridComponent<T>::buildTiles()
{
	if (mGridSizeOverride.x() != 0 && mGridSizeOverride.y() != 0)
		mAutoLayout = mGridSizeOverride;

	mStartPosition = 0;
	mTiles.clear();

	calcGridDimension();

	Vector2f tileDistance = mTileSize + mMargin;
	Vector2f tileSize = mTileSize;

	if (mAutoLayout.x() != 0 && mAutoLayout.y() != 0)
	{
		auto x = (mSize.x() - (mMargin.x() * (mAutoLayout.x() - 1)) - mPadding.x() - mPadding.z()) / (int) mAutoLayout.x();
		auto y = (mSize.y() - (mMargin.y() * (mAutoLayout.y() - 1)) - mPadding.y() - mPadding.w()) / (int) mAutoLayout.y();

		tileSize = Vector2f(x, y);
		mTileSize = tileSize;
		tileDistance = tileSize + mMargin;
	}

	bool vert = isVertical();

	Vector2f bufferSize = Vector2f(/*vert && mGridDimension.y() == 1 ? tileDistance.x() :*/ 0, 0);
	Vector2f startPosition = tileSize / 2 - bufferSize;

	startPosition += Vector2f(mPadding.x(), mPadding.y());

	int X, Y;

	// Layout tile size and position
	for (int y = 0; y < (vert ? mGridDimension.y() : mGridDimension.x()); y++)
	{
		for (int x = 0; x < (vert ? mGridDimension.x() : mGridDimension.y()); x++)
		{
			// Create tiles
			auto tile = std::make_shared<GridTileComponent>(mWindow);

			// In Vertical mod, tiles are ordered from left to right, then from top to bottom
			// In Horizontal mod, tiles are ordered from top to bottom, then from left to right
			X = vert ? x : y - EXTRAITEMS;
			Y = vert ? y - EXTRAITEMS : x;
			
			//if (!isVertical())
			//	X--;

			tile->setPosition(X * tileDistance.x() + startPosition.x(), Y * tileDistance.y() + startPosition.y());
			tile->setOrigin(0.5f, 0.5f);
			tile->reset();

			if (mTheme)
				tile->applyTheme(mTheme, mName, "gridtile", ThemeFlags::ALL);

			if (mAutoLayout.x() != 0 && mAutoLayout.y() != 0)
				tile->forceSize(mTileSize, mAutoLayoutZoom);

			mTiles.push_back(tile);
		}
	}

	mLastCursor = -1;
	onCursorChanged(CURSOR_STOPPED);
}


// Calculate how much tiles of size mTileSize we can fit in a grid of size mSize using a margin of size mMargin
template<typename T>
void ImageGridComponent<T>::calcGridDimension()
{
	// GRID_SIZE = COLUMNS * TILE_SIZE + (COLUMNS - 1) * MARGIN
	// <=> COLUMNS = (GRID_SIZE + MARGIN) / (TILE_SIZE + MARGIN)
	Vector2f gridDimension = (mSize + mMargin) / (mTileSize + mMargin);
	if (mAutoLayout.x() != 0 && mAutoLayout.y() != 0)
		gridDimension = mAutoLayout;

	mLastRowPartial = Math::floorf(gridDimension.y()) != gridDimension.y();
	mGridDimension = Vector2i((int) gridDimension.x(), (int) gridDimension.y());

	// Grid dimension validation
	if (mGridDimension.x() < 1)
		LOG(LogError) << "Theme defined grid X dimension below 1";
	if (mGridDimension.y() < 1)
		LOG(LogError) << "Theme defined grid Y dimension below 1";

	// Add extra tiles to both sides : Add EXTRAITEMS before, EXTRAITEMS after
	if (isVertical())
		mGridDimension.y() += 2 * EXTRAITEMS;
	else
		mGridDimension.x() += 2 * EXTRAITEMS;
};


#endif // ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H
