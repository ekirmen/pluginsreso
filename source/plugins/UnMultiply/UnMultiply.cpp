#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#pragma comment( lib, "shell32.lib" )

#include "UnMultiply.h"
#include "ffglquickstart/FFGLParamRange.h"
#include "ffglquickstart/FFGLParamOption.h"
#include "ffglquickstart/FFGLParamTrigger.h"

using namespace ffglex;
using namespace ffglqs;

static CFFGLPluginInfo PluginInfo(
	PluginFactory< AutoMaskPro >,
	"AMPR",
	"AutoMaskPro",
	2,
	1,
	1,
	2,
	FF_EFFECT,
	"Ultimate Surgical Keyer. Intel UHD Stable Build.",
	"By: Thexresolume@gmail.com | IG: @thex.led" );

static const char _fragmentShaderCode[] = R"(
void main()
{
	vec4 base = texture( inputTexture, i_uv );
	vec3 rgb = base.rgb;

	// 1. AUTO-PICK SENSING
	vec3 c1 = texture( inputTexture, vec2( 0.02, 0.02 ) ).rgb;
	vec3 c2 = texture( inputTexture, vec2( 0.98, 0.02 ) ).rgb;
	vec3 c3 = texture( inputTexture, vec2( 0.02, 0.98 ) ).rgb;
	vec3 c4 = texture( inputTexture, vec2( 0.98, 0.98 ) ).rgb;
	vec3 avgCorner = ( c1 + c2 + c3 + c4 ) / 4.0;
	
	vec3 k1 = mix( Key_Color.rgb, avgCorner, step( 0.5, Auto_Pick ) );

	float soft = max( Softness, 0.001 );
	
	// Primary Mask
	float m1 = 1.0 - smoothstep( Threshold_1 * 0.5, Threshold_1 * 0.5 + soft, distance( rgb, k1 ) );
	
	// Extra Mask 2
	float m2 = 0.0;
	if ( Enable_2 > 0.5 ) {
		m2 = 1.0 - smoothstep( Threshold_2 * 0.5, Threshold_2 * 0.5 + soft, distance( rgb, Key_Color_2.rgb ) );
	}

	// Extra Mask 3
	float m3 = 0.0;
	if ( Enable_3 > 0.5 ) {
		m3 = 1.0 - smoothstep( Threshold_3 * 0.5, Threshold_3 * 0.5 + soft, distance( rgb, Key_Color_3.rgb ) );
	}

	float finalMask = max( m1, max( m2, m3 ) );
	float a = 1.0 - finalMask;

	// LOGO PRESERVATION
	float lum = dot( rgb, vec3( 0.299, 0.587, 0.114 ) );
	float blackArea = 1.0 - smoothstep( 0.0, 0.15, lum );
	a = max( a, blackArea * Logo_Protect );

	if ( by_Input_Alpha > 0.5 ) a *= base.a;
	if ( Invert_Alpha > 0.5 ) a = 1.0 - a;
	
	vec3 outRGB = rgb;
	float outA = a;

	if ( Preview_Mode > 0.5 ) {
		if ( Preview_Mode < 1.5 ) {
			vec2 cp = floor( i_uv * resolution.xy / 20.0 );
			float pat = mod( cp.x + cp.y, 2.0 );
			outRGB = mix( mix( vec3( 0.1 ), vec3( 0.2 ), pat ), rgb, outA );
			outA = 1.0;
		} else {
			outRGB = vec3( outA );
			outA = 1.0;
		}
	}

	fragColor = vec4( outRGB * ( Preview_Mode > 0.5 ? 1.0 : outA ), outA );
}
)";

AutoMaskPro::AutoMaskPro()
{
	AddParam( ParamOption::Create( "ID", { { "@thex.led" }, { "Thexresolume@gmail.com" } }, 0 ) );

	// Link buttons using shader-safe internal names + descriptive display names
	AddParam( ParamTrigger::Create( "Insta_Link" ) );
	SetParamDisplayName( (unsigned int)params.size() - 1, "[ >>> VISITAR INSTAGRAM <<< ]", false );

	AddParam( ParamTrigger::Create( "Store_Link" ) );
	SetParamDisplayName( (unsigned int)params.size() - 1, "[ >>> IR A LA TIENDA GUMROAD <<< ]", false );

	AddParam( ParamOption::Create( "Auto_Pick", { { "OFF" }, { "ON (4 Corners)" } }, 0 ) );
	AddHueColorParam( "Key_Color" );
	GetParam( "Key_Color" )->SetValue( 0.33f );
	AddParam( ParamRange::Create( "Threshold_1", 0.5f, ParamRange::Range( 0.0f, 2.0f ) ) );

	AddParam( ParamOption::Create( "Enable_2", { { "OFF" }, { "ON" } }, 0 ) );
	AddHueColorParam( "Key_Color_2" );
	AddParam( ParamRange::Create( "Threshold_2", 0.0f, ParamRange::Range( 0.0f, 2.0f ) ) );

	AddParam( ParamOption::Create( "Enable_3", { { "OFF" }, { "ON" } }, 0 ) );
	AddHueColorParam( "Key_Color_3" );
	AddParam( ParamRange::Create( "Threshold_3", 0.0f, ParamRange::Range( 0.0f, 2.0f ) ) );

	AddParam( ParamRange::Create( "Softness", 0.1f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "Logo_Protect", 1.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "by_Input_Alpha", 1.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "Invert_Alpha", 0.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "Preview_Mode", 0.0f, ParamRange::Range( 0.0f, 2.0f ) ) );

	SetFragmentShader( _fragmentShaderCode );
}

void AutoMaskPro::Update()
{
	if( GetParam( "Insta_Link" )->GetValue() > 0.5f )
	{
		ShellExecuteA( NULL, "open", "https://www.instagram.com/thex.led/", NULL, NULL, SW_SHOWNORMAL );
	}
	if( GetParam( "Store_Link" )->GetValue() > 0.5f )
	{
		ShellExecuteA( NULL, "open", "https://thexresolume.gumroad.com/", NULL, NULL, SW_SHOWNORMAL );
	}
}

AutoMaskPro::~AutoMaskPro()
{
}