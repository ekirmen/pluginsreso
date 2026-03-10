#include "FFGLScreenCapture.h"
#include <math.h>
#include <algorithm>
#include <vector>

#ifdef _WIN32
// Windows includes are already in header or not needed here if in header
#elif defined( __APPLE__ )
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace ffglex;

// Enum is now in the header file

static CFFGLPluginInfo PluginInfo(
	PluginFactory< FFGLScreenCapture >,
	"SC01",
	"Screen Capture",
	2,
	1,
	1,
	000,
	FF_SOURCE,
	"Screen Capture Plugin",
	"Resolume Screen Capture" );

// Vertex Shader
static const char vertexShaderCode[] = R"(#version 410 core
layout( location = 0 ) in vec4 vPosition;
layout( location = 1 ) in vec2 vUV;
out vec2 uv;
void main()
{
	gl_Position = vPosition;
	uv = vUV;
}
)";

// Fragment Shader with Safes Crop & Flip Logic
static const char fragmentShaderCode[] = R"(#version 410 core
uniform sampler2D CaptureTexture;
uniform int HasCapture;
uniform float SwapRB;
uniform vec2 AspectParams; 
uniform float ScaleFit; 
uniform vec4 CropParams; 
uniform vec2 PosParams; // X, Y (0.5 = Center)
uniform float FlipV; 
uniform float FlipH;


in vec2 uv;
out vec4 fragColor;

void main()
{
	vec4 finalColor = vec4(0.0);
	
	if (HasCapture == 1) {
        vec2 coord = uv;
        
        // --- 1. DEFAULT BASE ORIENTATION ---
        // GDI captures are usually upside-down relative to OpenGL standard texture coords.
        // We ALWAYS flip Y by default to make it "upright".
        coord.y = 1.0 - coord.y; 
        
        // --- 1.5 POSITION OFFSET (SCROLL) ---
        // Input is 0.0..1.0, Center is 0.5. Range effectively -0.5 to +0.5
        vec2 offset;
        offset.x = (PosParams.x - 0.5); 
        offset.y = (PosParams.y - 0.5);
        
        // Apply offset BEFORE flips so it moves in "screen space" logic? 
        // Or AFTER flips to move locally? Usually better before flips if we think "Texture Scroll".
        // Let's apply it here to the base coordinate.
        // NOTE: if you scroll too much, you see clamp edge or wrap. WE set ClampToEdge in InitGL.
        coord -= offset;
        
        // --- 2. OPTIONAL USER FLIPS ---
        // If user wants Flip Vertical (FlipV > 0.5), we invert Y *again*.
        if (FlipV > 0.5) coord.y = 1.0 - coord.y;
        
        // If user wants Flip Horizontal (FlipH > 0.5), we invert X.
        if (FlipH > 0.5) coord.x = 1.0 - coord.x;
        
        // --- 3. CROP LOGIC ---
        float cL = CropParams.x;
        float cR = CropParams.y;
        float cT = CropParams.z;
        float cB = CropParams.w;
        
        // Clamp crop params
        if (cL + cR >= 0.99) cR = 0.99 - cL;
        if (cT + cB >= 0.99) cB = 0.99 - cT;
        
        float widthFactor = 1.0 - (cL + cR);
        float heightFactor = 1.0 - (cT + cB);

        vec2 sampleUV = coord; // Start with oriented coord
        
        // --- 4. ASPECT FIT LOGIC ---
        if (ScaleFit > 0.5) {
             float vpAspect = AspectParams.y;
             float imgAspect = AspectParams.x; 

             if (imgAspect > 0.0 && vpAspect > 0.0) {
                 // We need to apply aspect fit in the *visual* space (after flips)
                 // But since we modify UVs, we can just center around 0.5
                 
                 vec2 centered = uv - 0.5; // Use raw UV for centering logic
                 if (vpAspect > imgAspect) {
                      centered.x *= (vpAspect / imgAspect);
                 } else {
                      centered.y *= (imgAspect / vpAspect);
                 }
                 
                 // Check bounds in visual space
                 vec2 visualPos = centered + 0.5;
                 if (visualPos.x < 0.0 || visualPos.x > 1.0 || visualPos.y < 0.0 || visualPos.y > 1.0) {
                     fragColor = vec4(0.0);
                     return;
                 }
                 
                 // Now map back to texture space using flips
                 sampleUV = visualPos; 
                 // Re-apply orientation logic to this fitted UV
                 sampleUV.y = 1.0 - sampleUV.y; // Base Flip
                 if (FlipV > 0.5) sampleUV.y = 1.0 - sampleUV.y;
                 if (FlipH > 0.5) sampleUV.x = 1.0 - sampleUV.x;
             }
        }
        
        // --- 5. APPLY CROP MAPPING ---
        // Now 'sampleUV' is the desired point in the full texture (0..1)
        // We remap this 0..1 range into the cropped sub-rectangle
        
        sampleUV.x = cL + sampleUV.x * widthFactor;
        // Y direction: 0 is visual top (texture bottom after base flip), 1 is visual bottom.
        // It's safer to map 0->TopCrop, 1->BottomCrop relative to visual Y.
        // But since we already flipped Y to match visual, this linear map works on visual Y?
        // Let's assume texture coords are now "Visual".
        // Wait, texture sampling `texture(tex, uv)` uses standard GL coords (0=bottom).
        // Our `sampleUV` has been manipulated.
        
        // Let's standardise: 
        // 1. Calculate Crop Rect in Texture Space.
        // Texture is "upright" in memory? No, GDI bitmap.
        // Let's trust the Base Flip.
        
        // Just apply linear interpolation on the final UVs.
        sampleUV.y = cT + sampleUV.y * heightFactor; // This assumes sampleUV.y=0 is Top of crop area?
        // If we flipped base, then 0 is Top visually. So yes.
        
        vec4 c = texture(CaptureTexture, sampleUV);
        c.a = 1.0; 
        
        if (SwapRB > 0.5) finalColor = vec4(c.b, c.g, c.r, 1.0);
        else finalColor = c;
        
	} else {
		finalColor = vec4(0.0, 0.0, 0.0, 1.0);
	}


	
	fragColor = finalColor;
}
)";

FFGLScreenCapture::FFGLScreenCapture() :
	locationCaptureTexture( -1 ),
	locationHasCapture( -1 ),
	locationSwapRB( -1 ),
	locationAspect( -1 ),
	locationScaleFit( -1 ),
	locationCrop( -1 ),
	locationFlipV( -1 ),
	locationFlipH( -1 ),
	locationPos( -1 ),

	swapRBParam( 0.0f ),
	captureModeParam( 0.0f ),
	showCursorParam( 1.0f ),// Default ON
	fitModeParam( 0.0f ),
	flipVParam( 1.0f ),// Default ON (Standard GDI Capture)
	flipHParam( 0.0f ),
	cropLeft( 0.0f ),
	cropRight( 0.0f ),
	cropTop( 0.0f ),
	cropBottom( 0.0f ),
	posX( 0.5f ),
	posY( 0.5f ),
	windowIndexParam( 0.0f ),
	lastCaptureTime( std::chrono::steady_clock::now() )
{
	SetMinInputs( 0 );
	SetMaxInputs( 0 );

	// Calculate and State

	SetParamInfof( PT_SWAP_RB, "Swap RB", FF_TYPE_BOOLEAN );
	SetParamInfof( PT_CAPTURE_MODE, "Force Screen Blt", FF_TYPE_BOOLEAN );
	SetParamInfof( PT_SHOW_CURSOR, "Show Cursor", FF_TYPE_BOOLEAN );
	SetParamInfof( PT_FIT_MODE, "Fit Aspect Ratio", FF_TYPE_BOOLEAN );
	// Flip Controls
	SetParamInfof( PT_FLIP_V, "Flip Vertical", FF_TYPE_BOOLEAN );
	SetParamInfof( PT_FLIP_H, "Flip Horizontal", FF_TYPE_BOOLEAN );

	SetParamInfof( PT_CROP_LEFT, "Crop Left", FF_TYPE_STANDARD );
	SetParamInfof( PT_CROP_RIGHT, "Crop Right", FF_TYPE_STANDARD );
	SetParamInfof( PT_CROP_TOP, "Crop Top", FF_TYPE_STANDARD );
	SetParamInfof( PT_CROP_BOTTOM, "Crop Bottom", FF_TYPE_STANDARD );

	// Position Params (Ranges -1 to 1 logic, but input 0..1)
	SetParamInfof( PT_POS_X, "Position X", FF_TYPE_STANDARD );
	SetParamInfof( PT_POS_Y, "Position Y", FF_TYPE_STANDARD );

	SetOptionParamInfo( PT_WINDOW_INDEX, "Window Select", 1, 0.0f );
	SetParamElementInfo( PT_WINDOW_INDEX, 0, "[ Click Refresh Button ]", 0.0f );
	SetParamInfof( PT_REFRESH_WINDOWS, "Refresh List", FF_TYPE_EVENT );

	FFGLLog::LogToHost( "Created Screen Capture generator" );

	InitCaptureResources();
}

FFGLScreenCapture::~FFGLScreenCapture()
{
}

void FFGLScreenCapture::InitCaptureResources()
{
#ifdef _WIN32
	screenDC = GetDC( NULL );
	memDC    = CreateCompatibleDC( screenDC );
#elif defined( __APPLE__ )
	captureImage = NULL;
	// MacOS specific init if needed
#endif
}

void FFGLScreenCapture::ReleaseCaptureResources()
{
#ifdef _WIN32
	if( hOldBitmap && memDC )
		SelectObject( memDC, hOldBitmap );
	if( hBitmap )
		DeleteObject( hBitmap );
	if( memDC )
		DeleteDC( memDC );
	if( screenDC )
		ReleaseDC( NULL, screenDC );
	hBitmap  = NULL;
	memDC    = NULL;
	screenDC = NULL;
#elif defined( __APPLE__ )
	if( captureImage )
	{
		CGImageRelease( captureImage );
		captureImage = NULL;
	}
#endif
}

#ifdef _WIN32
BOOL CALLBACK FFGLScreenCapture::EnumWindowsCallback( HWND hwnd, LPARAM lParam )
{
	std::vector< WindowInfo >* windows = reinterpret_cast< std::vector< WindowInfo >* >( lParam );
	if( windows->size() >= 100 )
		return FALSE;

	if( IsWindowVisible( hwnd ) )
	{
		if( IsHungAppWindow( hwnd ) )
			return TRUE;

		DWORD winPID;
		GetWindowThreadProcessId( hwnd, &winPID );
		if( winPID == GetCurrentProcessId() )
			return TRUE;

		int length = GetWindowTextLengthA( hwnd );
		if( length > 0 )
		{
			LONG_PTR exStyle = GetWindowLongPtr( hwnd, GWL_EXSTYLE );
			if( ( exStyle & WS_EX_TOOLWINDOW ) && !( exStyle & WS_EX_APPWINDOW ) )
				return TRUE;

			std::vector< char > buffer( length + 1 );
			GetWindowTextA( hwnd, buffer.data(), length + 1 );
			std::string title = buffer.data();

			if( title == "Program Manager" || title == "Settings" || title == "Cortana" )
				return TRUE;

			windows->push_back( { hwnd, title } );
		}
	}
	return TRUE;
}
#endif

void FFGLScreenCapture::RefreshWindowList()
{
	windowList.clear();
	std::vector< std::string > names;
	std::vector< float > values;

	windowList.push_back( { 0, "[ Select a Window ]" } );// 0 or NULL handles both
	names.push_back( "[ Select a Window ]" );
	values.push_back( 0.0f );

#ifdef _WIN32
	EnumWindows( EnumWindowsCallback, reinterpret_cast< LPARAM >( &windowList ) );
#elif defined( __APPLE__ )
	CFArrayRef windowListCF = CGWindowListCopyWindowInfo( kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID );
	if( windowListCF )
	{
		CFIndex count = CFArrayGetCount( windowListCF );
		for( CFIndex i = 0; i < count; i++ )
		{
			if( windowList.size() >= 100 )
				break;

			CFDictionaryRef windowInfo = (CFDictionaryRef)CFArrayGetValueAtIndex( windowListCF, i );
			if( !windowInfo )
				continue;

			// Get Window ID
			CFNumberRef windowIDRef = (CFNumberRef)CFDictionaryGetValue( windowInfo, kCGWindowNumber );
			CGWindowID wid          = 0;
			if( windowIDRef )
				CFNumberGetValue( windowIDRef, kCFNumberIntType, &wid );

			// Get Window Name (Owner Name usually better than Window Name for identification)
			CFStringRef ownerNameRef = (CFStringRef)CFDictionaryGetValue( windowInfo, kCGWindowOwnerName );
			CFStringRef nameRef      = (CFStringRef)CFDictionaryGetValue( windowInfo, kCGWindowName );

			std::string nameStr = "";
			if( ownerNameRef )
			{
				char buffer[ 256 ];
				if( CFStringGetCString( ownerNameRef, buffer, 256, kCFStringEncodingUTF8 ) )
				{
					nameStr += buffer;
				}
			}
			if( nameRef )
			{
				char buffer[ 256 ];
				if( CFStringGetCString( nameRef, buffer, 256, kCFStringEncodingUTF8 ) )
				{
					if( !nameStr.empty() )
						nameStr += " - ";
					nameStr += buffer;
				}
			}

			if( !nameStr.empty() )
			{
				windowList.push_back( { wid, nameStr } );
			}
		}
		CFRelease( windowListCF );
	}
#endif

	for( size_t i = 1; i < windowList.size(); ++i )
	{
		names.push_back( windowList[ i ].title );
		values.push_back( (float)i );
	}
	SetParamElements( PT_WINDOW_INDEX, names, values, true );
}

#ifdef _WIN32
void FFGLScreenCapture::DrawCursor( HDC hdc, int x_offset, int y_offset )
{
	CURSORINFO ci = { sizeof( CURSORINFO ) };
	if( GetCursorInfo( &ci ) )
	{
		if( ci.flags == CURSOR_SHOWING )
		{
			ICONINFO ii;
			if( GetIconInfo( ci.hCursor, &ii ) )
			{
				DrawIcon( hdc, ci.ptScreenPos.x - x_offset - ii.xHotspot, ci.ptScreenPos.y - y_offset - ii.yHotspot, ci.hCursor );
				if( ii.hbmColor )
					DeleteObject( ii.hbmColor );
				if( ii.hbmMask )
					DeleteObject( ii.hbmMask );
			}
		}
	}
}
#endif

#ifdef _WIN32
bool FFGLScreenCapture::UpdateCapture( HWND hwnd )
{
	if( !hwnd || !IsWindow( hwnd ) )
		return false;

	auto now     = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >( now - lastCaptureTime ).count();
	if( elapsed < 33 )
		return false;
	lastCaptureTime = now;

	RECT rect;
	if( !GetWindowRect( hwnd, &rect ) )
		return false;

	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;

	if( w <= 0 || h <= 0 )
		return false;
	if( w > 3840 )
		w = 3840;
	if( h > 2160 )
		h = 2160;

	if( w != captureWidth || h != captureHeight || hBitmap == NULL )
	{
		if( hBitmap )
			DeleteObject( hBitmap );
		hBitmap = CreateCompatibleBitmap( screenDC, w, h );
		SelectObject( memDC, hBitmap );
		captureWidth  = w;
		captureHeight = h;
		pixelBuffer.resize( w * h * 4 );
	}

	BOOL result = FALSE;

	if( captureModeParam < 0.5f )
	{
		if( IsIconic( hwnd ) )
			return false;// Cannot capture minimized

		// Force the window to repaint its backbuffer immediately.
		RedrawWindow( hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN );

		result = PrintWindow( hwnd, memDC, 2 );
		if( !result )
			result = PrintWindow( hwnd, memDC, 0 );
	}
	else
	{
		result = BitBlt( memDC, 0, 0, w, h, screenDC, rect.left, rect.top, SRCCOPY | CAPTUREBLT );
	}

	if( result )
	{
		if( showCursorParam > 0.5f )
		{
			DrawCursor( memDC, rect.left, rect.top );
		}

		BITMAPINFO bmi              = { 0 };
		bmi.bmiHeader.biSize        = sizeof( BITMAPINFOHEADER );
		bmi.bmiHeader.biWidth       = w;
		bmi.bmiHeader.biHeight      = h;
		bmi.bmiHeader.biPlanes      = 1;
		bmi.bmiHeader.biBitCount    = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		if( GetDIBits( memDC, hBitmap, 0, h, pixelBuffer.data(), &bmi, DIB_RGB_COLORS ) )
		{
			glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
			glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
			glBindTexture( GL_TEXTURE_2D, captureTextureID );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixelBuffer.data() );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glBindTexture( GL_TEXTURE_2D, 0 );
			return true;
		}
	}
	return false;
}
#elif defined( __APPLE__ )
bool FFGLScreenCapture::UpdateCapture( CGWindowID windowID )
{
	auto now     = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >( now - lastCaptureTime ).count();
	if( elapsed < 33 )
		return false;
	lastCaptureTime = now;

	if( captureImage )
	{
		CGImageRelease( captureImage );
		captureImage = NULL;
	}

	// Capture the window
	captureImage = CGWindowListCreateImage( CGRectNull, kCGWindowListOptionIncludingWindow, windowID, kCGWindowImageBoundsIgnoreFraming | kCGWindowImageNominalResolution );

	if( captureImage )
	{
		int w = (int)CGImageGetWidth( captureImage );
		int h = (int)CGImageGetHeight( captureImage );

		if( w <= 0 || h <= 0 )
			return false;

		captureWidth  = w;
		captureHeight = h;

		// Get data provider
		CGDataProviderRef provider = CGImageGetDataProvider( captureImage );
		CFDataRef data             = CGDataProviderCopyData( provider );
		const unsigned char* bytes = CFDataGetBytePtr( data );

		// Ensure BGRA/RGBA match or just upload. MacOS usually gives BGRA or RGBA.
		// We will just upload what we get and SwapRB if needed.

		glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
		glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
		glBindTexture( GL_TEXTURE_2D, captureTextureID );
		// Quartz typically returns BGRA 32-bit for window captures
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bytes );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glBindTexture( GL_TEXTURE_2D, 0 );

		CFRelease( data );
		return true;
	}

	return false;
}
#endif

FFResult FFGLScreenCapture::InitGL( const FFGLViewportStruct* vp )
{
	if( !shader.Compile( vertexShaderCode, fragmentShaderCode ) )
	{
		DeInitGL();
		return FF_FAIL;
	}
	if( !quad.Initialise() )
	{
		DeInitGL();
		return FF_FAIL;
	}

	ScopedShaderBinding shaderBinding( shader.GetGLID() );
	locationCaptureTexture = shader.FindUniform( "CaptureTexture" );
	locationHasCapture     = shader.FindUniform( "HasCapture" );
	locationSwapRB         = shader.FindUniform( "SwapRB" );
	locationAspect         = shader.FindUniform( "AspectParams" );
	locationScaleFit       = shader.FindUniform( "ScaleFit" );
	locationCrop           = shader.FindUniform( "CropParams" );
	locationCrop           = shader.FindUniform( "CropParams" );
	locationPos            = shader.FindUniform( "PosParams" );
	locationFlipV          = shader.FindUniform( "FlipV" );
	locationFlipH          = shader.FindUniform( "FlipH" );

	glGenTextures( 1, &captureTextureID );
	glBindTexture( GL_TEXTURE_2D, captureTextureID );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glBindTexture( GL_TEXTURE_2D, 0 );

	if( vp )
		Resize( vp );

	return CFFGLPlugin::InitGL( vp );
}

FFResult FFGLScreenCapture::Resize( const FFGLViewportStruct* vp )
{
	if( vp && vp->height > 0 )
	{
		vpAspectRatio = (float)vp->width / (float)vp->height;
	}
	return FF_SUCCESS;
}

FFResult FFGLScreenCapture::ProcessOpenGL( ProcessOpenGLStruct* pGL )
{
	bool hasContent = ( captureTextureID != 0 && captureWidth > 0 );
	int index       = (int)windowIndexParam;

	if( index > 0 && index < windowList.size() )
	{
#ifdef _WIN32
		HWND target = windowList[ index ].hwnd;
		if( IsWindow( target ) )
		{
			if( UpdateCapture( target ) )
			{
				hasContent = true;
			}
		}
#elif defined( __APPLE__ )
		CGWindowID target = windowList[ index ].windowID;
		if( UpdateCapture( target ) )
		{
			hasContent = true;
		}
#endif
	}

	ScopedShaderBinding shaderBinding( shader.GetGLID() );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, captureTextureID );
	glUniform1i( locationCaptureTexture, 0 );
	glUniform1i( locationHasCapture, hasContent ? 1 : 0 );
	glUniform1f( locationSwapRB, swapRBParam );

	float imgAspect = 1.0f;
	if( captureHeight > 0 )
		imgAspect = (float)captureWidth / (float)captureHeight;

	glUniform2f( locationAspect, imgAspect, vpAspectRatio );
	glUniform1f( locationScaleFit, fitModeParam );
	glUniform4f( locationCrop, cropLeft, cropRight, cropTop, cropBottom );
	glUniform2f( locationPos, posX, posY );
	glUniform1f( locationFlipV, flipVParam );
	glUniform1f( locationFlipH, flipHParam );

	quad.Draw();
	glBindTexture( GL_TEXTURE_2D, 0 );

	return FF_SUCCESS;
}

FFResult FFGLScreenCapture::DeInitGL()
{
	shader.FreeGLResources();
	quad.Release();
	if( captureTextureID )
	{
		glDeleteTextures( 1, &captureTextureID );
		captureTextureID = 0;
	}
	ReleaseCaptureResources();
	return FF_SUCCESS;
}

FFResult FFGLScreenCapture::SetFloatParameter( unsigned int dwIndex, float value )
{
	switch( dwIndex )
	{
	case PT_SWAP_RB:
		swapRBParam = value;
		break;
	case PT_CAPTURE_MODE:
		captureModeParam = value;
		break;
	case PT_SHOW_CURSOR:
		showCursorParam = value;
		break;
	case PT_FIT_MODE:
		fitModeParam = value;
		break;
	case PT_FLIP_V:
		flipVParam = value;
		break;
	case PT_FLIP_H:
		flipHParam = value;
		break;
	case PT_CROP_LEFT:
		cropLeft = value;
		break;
	case PT_CROP_RIGHT:
		cropRight = value;
		break;
	case PT_CROP_TOP:
		cropTop = value;
		break;
	case PT_CROP_BOTTOM:
		cropBottom = value;
		break;
	case PT_POS_X:
		posX = value;
		break;
	case PT_POS_Y:
		posY = value;
		break;

	case PT_WINDOW_INDEX:
		windowIndexParam = value;
		break;
	case PT_REFRESH_WINDOWS:
		if( value > 0.5f )
			RefreshWindowList();
		break;
	}
	return FF_SUCCESS;
}

float FFGLScreenCapture::GetFloatParameter( unsigned int index )
{
	switch( index )
	{
	case PT_SWAP_RB:
		return swapRBParam;
	case PT_CAPTURE_MODE:
		return captureModeParam;
	case PT_SHOW_CURSOR:
		return showCursorParam;
	case PT_FIT_MODE:
		return fitModeParam;
	case PT_FLIP_V:
		return flipVParam;
	case PT_FLIP_H:
		return flipHParam;
	case PT_CROP_LEFT:
		return cropLeft;
	case PT_CROP_RIGHT:
		return cropRight;
	case PT_CROP_TOP:
		return cropTop;
	case PT_CROP_BOTTOM:
		return cropBottom;
	case PT_POS_X:
		return posX;
	case PT_POS_Y:
		return posY;

	case PT_WINDOW_INDEX:
		return windowIndexParam;
	case PT_REFRESH_WINDOWS:
		return 0.0f;
	}
	return 0.0f;
}
