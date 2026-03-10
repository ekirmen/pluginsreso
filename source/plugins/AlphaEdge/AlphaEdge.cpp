#include "AlphaEdge.h"
#include "ffglquickstart/FFGLParamRange.h"

using namespace ffglex;
using namespace ffglqs;

// Plugin creation boilerplate
static CFFGLPluginInfo PluginInfo(
	PluginFactory< AlphaEdge >,
	"ALED",
	"Alpha Edge FX",
	2,
	1,
	1,
	0,
	FF_EFFECT,
	"Adds borders, inner/outer glows, drop shadows, and gradients to the alpha channel.",
	"Antigravity" );

static const char _fragmentShaderCode[] = R"(
void main()
{
	vec4 baseCol = texture( inputTexture, i_uv );
	
	int taps = 16;
	float stepAngle = 6.28318530718 / float(taps);
    
    vec2 aspect = vec2(1.0, 1.0);
    if(resolution.y > 0.0) aspect = vec2(1.0, resolution.x / resolution.y); 
	
    // Edge + Inner Glow samples
	float radius = EdgeWidth * 0.1; 
	float maxAlpha = 0.0;
	float avgAlpha = 0.0;

    // Shadow samples -> Y is inverted because Resolume UV Y is up
    vec2 shadowUV = i_uv - vec2(ShadowOffsetX, -ShadowOffsetY) * 0.1;
	float shadowAvgAlpha = 0.0;
	float shadowMaxAlpha = 0.0;
	float sRadius = ShadowSpread * 0.1;

	for(int i = 0; i < taps; i++) {
        float angle = float(i) * stepAngle;
		vec2 dir = vec2(cos(angle), sin(angle)) * aspect;
		
        // Edge samples
		float a = texture(inputTexture, i_uv + dir * radius).a;
		maxAlpha = max(maxAlpha, a);
		avgAlpha += a;
        
        // Shadow samples
        float sa = texture(inputTexture, shadowUV + dir * sRadius).a;
        shadowMaxAlpha = max(shadowMaxAlpha, sa);
        shadowAvgAlpha += sa;
	}
	avgAlpha /= float(taps);
    shadowAvgAlpha /= float(taps);
	
	float smoothedEdge = mix(maxAlpha, avgAlpha, EdgeSoftness);
	float smoothedShadow = mix(shadowMaxAlpha, shadowAvgAlpha, 0.5); 
	
    // Shadow Layer (black, drawn at bottom)
    float shadowAlpha = clamp(smoothedShadow - baseCol.a, 0.0, 1.0) * ShadowOpacity;
    vec4 shadowFinal = vec4(0.0, 0.0, 0.0, shadowAlpha);
    
    // Edge Layer
    float edgeAlpha = clamp(smoothedEdge - baseCol.a, 0.0, 1.0) * GlowIntensity;
    vec4 edgeFinal = vec4(EdgeColor.rgb * edgeAlpha, edgeAlpha);
    
    // Inner Glow Layer
    float innerAlpha = clamp(1.0 - avgAlpha, 0.0, 1.0) * baseCol.a * InnerGlowStr;
    vec4 innerFinal = vec4(InnerColor.rgb * innerAlpha, innerAlpha);
    
    // Gradient Fade
    vec2 gStart = vec2(FadeStartX, FadeStartY);
    vec2 gEnd = vec2(FadeEndX, FadeEndY);
    vec2 gDir = gEnd - gStart;
    float lenSq = dot(gDir, gDir);
    float t = 0.0;
    if (lenSq > 0.0001) {
        t = clamp(dot(i_uv - gStart, gDir) / lenSq, 0.0, 1.0);
    }
    float fadeMultiplier = mix(1.0, 1.0 - t, FadeStrength);

	// Compositing (using pre-multiplied alpha standard: C = C_over + C_under * (1 - Alpha_over))
    vec4 finalCol = shadowFinal;
    
    // composite Edge over Shadow
    finalCol = edgeFinal + finalCol * clamp(1.0 - edgeFinal.a, 0.0, 1.0);
    
    // composite Base with Inner Glow
    vec4 baseWithInner = innerFinal + baseCol * clamp(1.0 - innerFinal.a, 0.0, 1.0);
    
    // composite Base+Inner over Edge+Shadow
    finalCol = baseWithInner + finalCol * clamp(1.0 - baseWithInner.a, 0.0, 1.0);
    
    // apply Fade Mask scaling both RGB and Alpha
    finalCol *= fadeMultiplier; 

	fragColor = clamp(finalCol, 0.0, 1.0);
}
)";

AlphaEdge::AlphaEdge()
{
	// FF_EFFECT implies 1 input texture
	AddHueColorParam( "EdgeColor" );
	// Set default edge color to a nice cyan
	GetParam( "EdgeColor" )->SetValue( 0.5f );// 0.5 hue is cyan
	GetParam( "EdgeColor_saturation" )->SetValue( 1.0f );
	GetParam( "EdgeColor_brightness" )->SetValue( 1.0f );
	GetParam( "EdgeColor_alpha" )->SetValue( 1.0f );

	AddParam( ParamRange::Create( "EdgeWidth", 0.2f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "EdgeSoftness", 0.5f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "GlowIntensity", 1.0f, ParamRange::Range( 0.0f, 10.0f ) ) );

	AddHueColorParam( "InnerColor" );
	GetParam( "InnerColor" )->SetValue( 0.0f );// 0.0 hue is red
	GetParam( "InnerColor_saturation" )->SetValue( 1.0f );
	GetParam( "InnerColor_brightness" )->SetValue( 1.0f );
	GetParam( "InnerColor_alpha" )->SetValue( 1.0f );

	AddParam( ParamRange::Create( "InnerGlowStr", 0.0f, ParamRange::Range( 0.0f, 5.0f ) ) );

	AddParam( ParamRange::Create( "ShadowOffsetX", 0.0f, ParamRange::Range( -1.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "ShadowOffsetY", -0.2f, ParamRange::Range( -1.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "ShadowSpread", 0.2f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "ShadowOpacity", 0.0f, ParamRange::Range( 0.0f, 1.0f ) ) );

	AddParam( ParamRange::Create( "FadeStartX", 0.5f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "FadeStartY", 0.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "FadeEndX", 0.5f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "FadeEndY", 1.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "FadeStrength", 0.0f, ParamRange::Range( 0.0f, 1.0f ) ) );

	SetFragmentShader( _fragmentShaderCode );
}

AlphaEdge::~AlphaEdge()
{
}
