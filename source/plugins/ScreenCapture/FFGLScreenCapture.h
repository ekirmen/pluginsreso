#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <commdlg.h>
#elif defined( __APPLE__ )
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <FFGLSDK.h>
#include <vector>
#include <string>
#include <chrono>

// Must be sequential for host to find them
enum ParameterLayout
{
	PT_SWAP_RB,
	PT_CAPTURE_MODE,
	PT_SHOW_CURSOR,
	PT_FIT_MODE,
	PT_FLIP_V,
	PT_FLIP_H,
	PT_CROP_LEFT,
	PT_CROP_RIGHT,
	PT_CROP_TOP,
	PT_CROP_BOTTOM,

	// Position Params
	PT_POS_X,
	PT_POS_Y,

	PT_WINDOW_INDEX,
	PT_REFRESH_WINDOWS,

};

class FFGLScreenCapture : public CFFGLPlugin
{
public:
	FFGLScreenCapture();
	~FFGLScreenCapture();

	// CFFGLPlugin
	FFResult InitGL( const FFGLViewportStruct* vp ) override;
	FFResult ProcessOpenGL( ProcessOpenGLStruct* pGL ) override;
	FFResult DeInitGL() override;
	FFResult Resize( const FFGLViewportStruct* vp ) override;

	FFResult SetFloatParameter( unsigned int dwIndex, float value ) override;
	float GetFloatParameter( unsigned int index ) override;

private:
	void InitCaptureResources();
	void ReleaseCaptureResources();
	void RefreshWindowList();
#ifdef _WIN32
	bool UpdateCapture( HWND hwnd );
	static BOOL CALLBACK EnumWindowsCallback( HWND hwnd, LPARAM lParam );
	void DrawCursor( HDC hdc, int x_offset, int y_offset );
#elif defined( __APPLE__ )
	bool UpdateCapture( CGWindowID windowID );
#endif

	// Shader & GL
	ffglex::FFGLShader shader;
	ffglex::FFGLScreenQuad quad;
	GLuint captureTextureID;

	// Param locations
	GLint locationCaptureTexture;
	GLint locationHasCapture;
	GLint locationSwapRB;
	GLint locationAspect;
	GLint locationScaleFit;
	GLint locationCrop;
	GLint locationFlipV;
	GLint locationFlipH;

	// Params
	float swapRBParam;
	float captureModeParam;
	float showCursorParam;
	float fitModeParam;
	float flipVParam;
	float flipHParam;

	float cropLeft, cropRight, cropTop, cropBottom;

	// Position
	GLint locationPos;
	float posX, posY;

	// Window List
	// Window List
	float windowIndexParam;
	struct WindowInfo
	{
#ifdef _WIN32
		HWND hwnd;
#elif defined( __APPLE__ )
		CGWindowID windowID;
#endif
		std::string title;
	};
	std::vector< WindowInfo > windowList;

	// Capture Resources
#ifdef _WIN32
	HDC screenDC;
	HDC memDC;
	HBITMAP hBitmap;
	HBITMAP hOldBitmap;
#elif defined( __APPLE__ )
	CGImageRef captureImage;
#endif

	std::vector< unsigned char > pixelBuffer;
	int captureWidth;
	int captureHeight;

	std::chrono::steady_clock::time_point lastCaptureTime;

	float vpAspectRatio;
};
