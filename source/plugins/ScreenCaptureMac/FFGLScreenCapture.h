#pragma once
#include <Windows.h>
#include <commdlg.h>
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

	// License Param
	PT_LICENSE_INFO
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

	// License Methods
	void CheckLicense();
	std::string GetHardwareID();
	std::string GenerateHash( std::string input );// Simple hash for demo purposes

private:
	void InitCaptureResources();
	void ReleaseCaptureResources();
	bool UpdateCapture( HWND hwnd );
	void RefreshWindowList();
	static BOOL CALLBACK EnumWindowsCallback( HWND hwnd, LPARAM lParam );
	void DrawCursor( HDC hdc, int x_offset, int y_offset );

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

	// License Locations / State
	GLint locationIsLicensed;// Shader uniform to enable "Demo Mode" visual
	bool isLicensed;
	std::string myHardwareID;
	std::string licenseDebugMsg;// Stores "File not found" or "Mismatch..."

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
		HWND hwnd;
		std::string title;
	};
	std::vector< WindowInfo > windowList;

	// GDI Capture Resources
	HDC screenDC;
	HDC memDC;
	HBITMAP hBitmap;
	HBITMAP hOldBitmap;

	std::vector< unsigned char > pixelBuffer;
	int captureWidth;
	int captureHeight;

	std::chrono::steady_clock::time_point lastCaptureTime;

	float vpAspectRatio;
};
