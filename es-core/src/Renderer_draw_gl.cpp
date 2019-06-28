#include "Renderer.h"

#include "math/Misc.h"
#include "Log.h"
#include <stack>

namespace Renderer {
	struct ClipRect {
		ClipRect(const int x, const int y, const int w, const int h) :
			x(x), y(y), w(w), h(h) {};
		int x;
		int y;
		int w;
		int h;
	};

	std::stack<ClipRect> clipStack;
	std::stack<ClipRect> nativeClipStack;

	void setColor4bArray(GLubyte* array, unsigned int color)
	{
		array[0] = ((color & 0xff000000) >> 24) & 255;
		array[1] = ((color & 0x00ff0000) >> 16) & 255;
		array[2] = ((color & 0x0000ff00) >>  8) & 255;
		array[3] = ((color & 0x000000ff)      ) & 255;
	}

	void buildGLColorArray(GLubyte* ptr, unsigned int color, unsigned int vertCount)
	{
		unsigned int colorGl;
		setColor4bArray((GLubyte*)&colorGl, color);
		for(unsigned int i = 0; i < vertCount; i++)
		{
			((GLuint*)ptr)[i] = colorGl;
		}
	}

	void pushClipRect(Vector2i pos, Vector2i dim)
	{
		ClipRect box(pos.x(), pos.y(), dim.x(), dim.y());
		if(box.w == 0)
			box.w = Renderer::getScreenWidth() - box.x;
		if(box.h == 0)
			box.h = Renderer::getScreenHeight() - box.y;

		//glScissor starts at the bottom left of the window
		//so (0, 0, 1, 1) is the bottom left pixel
		//everything else uses y+ = down, so flip it to be consistent
		switch(Renderer::getScreenRotate())
		{
			case 0: { box = ClipRect(box.x,                                         Renderer::getWindowHeight() - (box.y + box.h),                     box.w, box.h); } break;
			case 1: { box = ClipRect(Renderer::getScreenHeight() - (box.y + box.h), Renderer::getWindowWidth()  - (box.x + box.w),                     box.h, box.w); } break;
			case 2: { box = ClipRect(Renderer::getScreenWidth()  - (box.x + box.w), Renderer::getWindowHeight() - Renderer::getScreenHeight() + box.y, box.w, box.h); } break;
			case 3: { box = ClipRect(box.y,                                         Renderer::getWindowWidth()  - Renderer::getScreenWidth()  + box.x, box.h, box.w); } break;
		}

		switch(Renderer::getScreenRotate())
		{
			case 0: { box.x += Renderer::getScreenOffsetX(); box.y -= Renderer::getScreenOffsetY(); } break;
			case 1: { box.x += Renderer::getScreenOffsetY(); box.y -= Renderer::getScreenOffsetX(); } break;
			case 2: { box.x += Renderer::getScreenOffsetX(); box.y -= Renderer::getScreenOffsetY(); } break;
			case 3: { box.x += Renderer::getScreenOffsetY(); box.y -= Renderer::getScreenOffsetX(); } break;
		}

		//make sure the box fits within clipStack.top(), and clip further accordingly
		if (clipStack.size())
		{
			const ClipRect& top = clipStack.top();
			if(top.x > box.x)
				box.x = top.x;
			if(top.y > box.y)
				box.y = top.y;
			if(top.x + top.w < box.x + box.w)
				box.w = (top.x + top.w) - box.x;
			if(top.y + top.h < box.y + box.h)
				box.h = (top.y + top.h) - box.y;
		}

		if(box.w < 0)
			box.w = 0;
		if(box.h < 0)
			box.h = 0;

		clipStack.push(box);
		nativeClipStack.push(ClipRect(pos.x(), pos.y(), dim.x(), dim.y()));

		glScissor(box.x, box.y, box.w, box.h);
		glEnable(GL_SCISSOR_TEST);
	}

	void popClipRect()
	{
		if(clipStack.empty())
		{
			LOG(LogError) << "Tried to popClipRect while the stack was empty!";
			return;
		}

		clipStack.pop();
		nativeClipStack.pop();

		if(clipStack.empty())
		{
			glDisable(GL_SCISSOR_TEST);
		}
		else
		{
			const ClipRect& top = clipStack.top();
			glScissor(top.x, top.y, top.w, top.h);
		}
	}

	bool isClippingEnabled() { return !clipStack.empty(); }

	bool valueInRange(int value, int min, int max)
	{
		return (value >= min) && (value <= max);
	}

	bool rectOverlap(ClipRect A, ClipRect B)
	{
		bool xOverlap = valueInRange(A.x, B.x, B.x + B.w) ||
			valueInRange(B.x, A.x, A.x + A.w);

		bool yOverlap = valueInRange(A.y, B.y, B.y + B.h) ||
			valueInRange(B.y, A.y, A.y + A.h);

		return xOverlap && yOverlap;
	}

	bool isVisibleOnScreen(float x, float y, float w, float h)
	{		
		ClipRect screen = ClipRect(0, 0, Renderer::getWindowWidth(), Renderer::getWindowHeight());
		ClipRect box = ClipRect(x, y, w, h);

		if (w > 0 && x + w <= 0)
			return false;

		if (h > 0 && y + h <= 0)
			return false;
		
		if (x == screen.w || y == screen.h)
			return false;

		if (!rectOverlap(screen, box))
			return false;

		if (clipStack.empty())
			return true;
		
		screen = nativeClipStack.top();
		return rectOverlap(screen, box);
	}


	void drawRect(float x, float y, float w, float h, unsigned int color, GLenum blend_sfactor, GLenum blend_dfactor)
	{
		drawRect((int)Math::round(x), (int)Math::round(y), (int)Math::round(w), (int)Math::round(h), color, blend_sfactor, blend_dfactor);
	}

	#define MAKEQUAD(x) (((x) & 0xff000000) >> 24) / 255.0,  (((x) & 0x00ff0000) >> 16) / 255.0, (((x) & 0x0000ff00) >> 8) / 255.0, (((x) & 0x000000ff)) / 255.0

	void drawGradientRect(int x, int y, int w, int h, unsigned int color, unsigned int colorBottom, bool horz, GLenum blend_sfactor, GLenum blend_dfactor)
	{
		glEnable(GL_BLEND);
		glBlendFunc(blend_sfactor, blend_dfactor);
		
		glBegin(GL_QUADS);

		glColor4f(MAKEQUAD(horz ? colorBottom : color));
		glVertex2f(x, y);

		glColor4f(MAKEQUAD(color));
		glVertex2f(x + w, y);

		glColor4f(MAKEQUAD(horz ? color : colorBottom));
		glVertex2f(x + w, y + h);

		glColor4f(MAKEQUAD(colorBottom));
		glVertex2f(x, y + h);

		glEnd();

		glDisable(GL_BLEND);

		return;
	}

	void drawRect(int x, int y, int w, int h, unsigned int color, GLenum blend_sfactor, GLenum blend_dfactor)
	{
		glEnable(GL_BLEND);
		glBlendFunc(blend_sfactor, blend_dfactor);
		glBegin(GL_QUADS);

		GLfloat red = ((color & 0xff000000) >> 24) / 255.0;
		GLfloat green = ((color & 0x00ff0000) >> 16) / 255.0;
		GLfloat blue = ((color & 0x0000ff00) >> 8) / 255.0;
		GLfloat alpha = ((color & 0x000000ff)) / 255.0;

		glColor4f(red, green, blue, alpha);
		glVertex2f(x, y);

		glColor4f(red, green, blue, alpha);
		glVertex2f(x+w, y);

		glColor4f(red, green, blue, alpha);
		glVertex2f(x+w, y+h);

		glColor4f(red, green, blue, alpha);
		glVertex2f(x, y+h);

		glEnd();
		glDisable(GL_BLEND);

		return;

#ifdef USE_OPENGL_ES
		GLshort points[12];
#else
		GLint points[12];
#endif

		points[0] = x; points [1] = y;
		points[2] = x; points[3] = y + h;
		points[4] = x + w; points[5] = y;

		points[6] = x + w; points[7] = y;
		points[8] = x; points[9] = y + h;
		points[10] = x + w; points[11] = y + h;

		GLubyte colors[6*4];
		buildGLColorArray(colors, color, 6);

		glEnable(GL_BLEND);
		glBlendFunc(blend_sfactor, blend_dfactor);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

#ifdef USE_OPENGL_ES
		glVertexPointer(2, GL_SHORT, 0, points);
#else
		glVertexPointer(2, GL_INT, 0, points);
#endif
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisable(GL_BLEND);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	void setMatrix(const Transform4x4f& matrix)
	{
		glLoadMatrixf((GLfloat*)&matrix);
	}
};
