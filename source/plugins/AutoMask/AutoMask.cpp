#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#pragma comment( lib, "shell32.lib" )

#include "AutoMaskPro.h"
#include "ffglquickstart/FFGLParamRange.h"
#include "ffglquickstart/FFGLParamOption.h"
#include "ffglquickstart/FFGLParamTrigger.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

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
#version 130

uniform sampler2D inputTexture;
uniform vec2 resolution;
uniform float i_time;
varying vec2 i_uv;

uniform float Auto_Pick;
uniform vec3  Key_Color;
uniform float Threshold_1;
uniform float Enable_2;
uniform vec3  Key_Color_2;
uniform float Threshold_2;
uniform float Enable_3;
uniform vec3  Key_Color_3;
uniform float Threshold_3;
uniform float Softness;
uniform float Logo_Protect;
uniform float by_Input_Alpha;
uniform float Invert_Alpha;
uniform float Preview_Mode;
uniform float Selection_Mode;

void main()
{
    vec4 base = texture2D(inputTexture, i_uv);
    vec3 rgb = base.rgb;

    vec3 k1 = Key_Color.rgb;
    if (Auto_Pick > 0.5) {
        // Auto-pick de 4 esquinas (solo cuando está activo)
        vec3 c1 = texture2D(inputTexture, vec2(0.02, 0.02)).rgb;
        vec3 c2 = texture2D(inputTexture, vec2(0.98, 0.02)).rgb;
        vec3 c3 = texture2D(inputTexture, vec2(0.02, 0.98)).rgb;
        vec3 c4 = texture2D(inputTexture, vec2(0.98, 0.98)).rgb;
        k1 = (c1 + c2 + c3 + c4) * 0.25;
    }

    vec3 k2 = Key_Color_2.rgb;
    vec3 k3 = Key_Color_3.rgb;

    float soft = max(Softness, 0.001);
    float softEnd1 = Threshold_1 * 0.5 + soft;
    float softEnd2 = Threshold_2 * 0.5 + soft;
    float softEnd3 = Threshold_3 * 0.5 + soft;

    // Calcular máscaras
    float dist1 = dot(rgb - k1, rgb - k1);
    float m1 = 1.0 - smoothstep(Threshold_1 * 0.5, softEnd1, sqrt(dist1));
    
    float m2 = 0.0;
    if (Enable_2 > 0.5) {
        float dist2 = dot(rgb - k2, rgb - k2);
        m2 = 1.0 - smoothstep(Threshold_2 * 0.5, softEnd2, sqrt(dist2));
    }

    float m3 = 0.0;
    if (Enable_3 > 0.5) {
        float dist3 = dot(rgb - k3, rgb - k3);
        m3 = 1.0 - smoothstep(Threshold_3 * 0.5, softEnd3, sqrt(dist3));
    }

    float finalMask = max(m1, max(m2, m3));
    
    // MODO SELECT o REMOVE
    float a;
    if (Selection_Mode > 0.5) {
        a = finalMask;  // Select: conservar colores elegidos
    } else {
        a = 1.0 - finalMask;  // Remove: eliminar colores elegidos
    }

    // Protección de logo
    float lum = dot(rgb, vec3(0.299, 0.587, 0.114));
    float blackArea = 1.0 - smoothstep(0.0, 0.25, lum);
    a = max(a, blackArea * Logo_Protect);

    // Manejo de alpha
    if (by_Input_Alpha > 0.5) a *= base.a;
    if (Invert_Alpha > 0.5) a = 1.0 - a;
    a = clamp(a, 0.0, 1.0);
    
    // Preview modes
    vec3 outRGB = rgb;
    float outA = a;

    if (Preview_Mode > 0.5) {
        if (Preview_Mode < 1.5) {
            vec2 cp = floor(i_uv * resolution.xy / 20.0);
            float pat = mod(cp.x + cp.y, 2.0);
            vec3 bgColor = mix(vec3(0.1), vec3(0.2), pat);
            outRGB = mix(bgColor, rgb, outA);
            outA = 1.0;
        } else if (Preview_Mode < 2.5) {
            outRGB = vec3(outA);
            outA = 1.0;
        } else {
            vec3 heatColor = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), outA);
            outRGB = mix(heatColor, vec3(1.0, 1.0, 1.0), outA);
            outA = 1.0;
        }
    }

    gl_FragColor = vec4(outRGB, outA);
}
)";

AutoMaskPro::AutoMaskPro() :
	ffglqs::Effect( std::string( _fragmentShaderCode ) ),
	lastThreshold1( 0.5f ),
	lastThreshold2( 0.0f ),
	lastThreshold3( 0.0f ),
	lastSoftness( 0.1f ),
	isValidating( false )
{
	// Parámetro informativo
	AddParam( ParamOption::Create( "ID", { { "@thex.led" }, { "Thexresolume@gmail.com" } }, 0 ) );

	// Botones de enlace
	ParamTrigger* instaBtn                = ParamTrigger::Create( "Insta_Link" );
	idxInstaLink                          = AddParam( instaBtn );
	GetParam( idxInstaLink )->DisplayName = "[ >>> VISITAR INSTAGRAM <<< ]";

	ParamTrigger* storeBtn                = ParamTrigger::Create( "Store_Link" );
	idxStoreLink                          = AddParam( storeBtn );
	GetParam( idxStoreLink )->DisplayName = "[ >>> IR A LA TIENDA GUMROAD <<< ]";

	// KEY 1
	ParamOption* autoPick = ParamOption::Create( "Auto_Pick", { { "OFF" }, { "ON (4 Corners)" } }, 0 );
	idxAutoPick           = AddParam( autoPick );

	ParamRange* keyColor                 = ParamRange::Create( "Key_Color", 0.33f, ParamRange::Range( 0.0f, 1.0f ) );
	idxKeyColor                          = AddParam( keyColor );
	GetParam( idxKeyColor )->DisplayName = "Key Color 1 (Hue)";

	ParamRange* th1 = ParamRange::Create( "Threshold_1", 0.5f, ParamRange::Range( 0.0f, 2.0f ) );
	idxThreshold1   = AddParam( th1 );

	// KEY 2
	ParamOption* en2 = ParamOption::Create( "Enable_2", { { "OFF" }, { "ON" } }, 0 );
	idxEnable2       = AddParam( en2 );

	ParamRange* keyColor2                 = ParamRange::Create( "Key_Color_2", 0.5f, ParamRange::Range( 0.0f, 1.0f ) );
	idxKeyColor2                          = AddParam( keyColor2 );
	GetParam( idxKeyColor2 )->DisplayName = "Key Color 2 (Hue)";

	ParamRange* th2 = ParamRange::Create( "Threshold_2", 0.0f, ParamRange::Range( 0.0f, 2.0f ) );
	idxThreshold2   = AddParam( th2 );

	// KEY 3
	ParamOption* en3 = ParamOption::Create( "Enable_3", { { "OFF" }, { "ON" } }, 0 );
	idxEnable3       = AddParam( en3 );

	ParamRange* keyColor3                 = ParamRange::Create( "Key_Color_3", 0.66f, ParamRange::Range( 0.0f, 1.0f ) );
	idxKeyColor3                          = AddParam( keyColor3 );
	GetParam( idxKeyColor3 )->DisplayName = "Key Color 3 (Hue)";

	ParamRange* th3 = ParamRange::Create( "Threshold_3", 0.0f, ParamRange::Range( 0.0f, 2.0f ) );
	idxThreshold3   = AddParam( th3 );

	// Controles maestros
	ParamRange* soft = ParamRange::Create( "Softness", 0.1f, ParamRange::Range( 0.0f, 1.0f ) );
	idxSoftness      = AddParam( soft );

	ParamRange* logo = ParamRange::Create( "Logo_Protect", 1.0f, ParamRange::Range( 0.0f, 1.0f ) );
	idxLogoProtect   = AddParam( logo );

	ParamRange* inputAlpha = ParamRange::Create( "by_Input_Alpha", 1.0f, ParamRange::Range( 0.0f, 1.0f ) );
	idxByInputAlpha        = AddParam( inputAlpha );

	ParamRange* invertAlpha = ParamRange::Create( "Invert_Alpha", 0.0f, ParamRange::Range( 0.0f, 1.0f ) );
	idxInvertAlpha          = AddParam( invertAlpha );

	ParamRange* preview = ParamRange::Create( "Preview_Mode", 0.0f, ParamRange::Range( 0.0f, 3.0f ) );
	idxPreviewMode      = AddParam( preview );

	// MODO SELECT/REMOVE
	ParamOption* selMode                      = ParamOption::Create( "Selection_Mode", { { "Remove" }, { "Select" } }, 0 );
	idxSelectionMode                          = AddParam( selMode );
	GetParam( idxSelectionMode )->DisplayName = "Modo: Eliminar/Seleccionar";

	RegisterUniforms();
	ValidateParameters();
}

void AutoMaskPro::RegisterUniforms()
{
	RegisterUniform( "Auto_Pick", idxAutoPick );
	RegisterUniform( "Key_Color", idxKeyColor );
	RegisterUniform( "Threshold_1", idxThreshold1 );
	RegisterUniform( "Enable_2", idxEnable2 );
	RegisterUniform( "Key_Color_2", idxKeyColor2 );
	RegisterUniform( "Threshold_2", idxThreshold2 );
	RegisterUniform( "Enable_3", idxEnable3 );
	RegisterUniform( "Key_Color_3", idxKeyColor3 );
	RegisterUniform( "Threshold_3", idxThreshold3 );
	RegisterUniform( "Softness", idxSoftness );
	RegisterUniform( "Logo_Protect", idxLogoProtect );
	RegisterUniform( "by_Input_Alpha", idxByInputAlpha );
	RegisterUniform( "Invert_Alpha", idxInvertAlpha );
	RegisterUniform( "Preview_Mode", idxPreviewMode );
	RegisterUniform( "Selection_Mode", idxSelectionMode );
}

void AutoMaskPro::ValidateParameters()
{
	if( isValidating )
	{
		return;
	}

	isValidating = true;

	float threshold1 = std::max( 0.0f, std::min( 2.0f, GetFloatParameter( idxThreshold1 ) ) );
	float threshold2 = std::max( 0.0f, std::min( 2.0f, GetFloatParameter( idxThreshold2 ) ) );
	float threshold3 = std::max( 0.0f, std::min( 2.0f, GetFloatParameter( idxThreshold3 ) ) );
	float softness   = std::max( 0.0f, std::min( 1.0f, GetFloatParameter( idxSoftness ) ) );

	ffglqs::Effect::SetFloatParameter( idxThreshold1, threshold1 );
	ffglqs::Effect::SetFloatParameter( idxThreshold2, threshold2 );
	ffglqs::Effect::SetFloatParameter( idxThreshold3, threshold3 );
	ffglqs::Effect::SetFloatParameter( idxSoftness, softness );

	if( fabs( softness - lastSoftness ) > 0.001f )
	{
		lastSoftness = softness;
	}

	if( fabs( threshold1 - lastThreshold1 ) > 0.001f )
	{
		lastThreshold1 = threshold1;
	}

	if( fabs( threshold2 - lastThreshold2 ) > 0.001f )
	{
		lastThreshold2 = threshold2;
	}

	if( fabs( threshold3 - lastThreshold3 ) > 0.001f )
	{
		lastThreshold3 = threshold3;
	}

	isValidating = false;
}

FFResult AutoMaskPro::SetFloatParameter( unsigned int index, float value )
{
	if( index == idxThreshold1 || index == idxThreshold2 || index == idxThreshold3 )
	{
		value = std::max( 0.0f, std::min( 2.0f, value ) );
	}
	else if( index == idxSoftness )
	{
		value = std::max( 0.0f, std::min( 1.0f, value ) );
	}

	FFResult result = ffglqs::Effect::SetFloatParameter( index, value );
	ValidateParameters();
	return result;
}

void AutoMaskPro::Update()
{
	static bool instaTriggered = false;
	static bool storeTriggered = false;

	float instaVal = GetFloatParameter( idxInstaLink );
	float storeVal = GetFloatParameter( idxStoreLink );

	if( instaVal > 0.5f && !instaTriggered )
	{
		instaTriggered = true;
		ShellExecuteA( NULL, "open", "https://www.instagram.com/thex.led/", NULL, NULL, SW_SHOWNORMAL );
	}
	else if( instaVal < 0.5f )
	{
		instaTriggered = false;
	}

	if( storeVal > 0.5f && !storeTriggered )
	{
		storeTriggered = true;
		ShellExecuteA( NULL, "open", "https://thexresolume.gumroad.com/", NULL, NULL, SW_SHOWNORMAL );
	}
	else if( storeVal < 0.5f )
	{
		storeTriggered = false;
	}

	ValidateParameters();
}

AutoMaskPro::~AutoMaskPro()
{
}
