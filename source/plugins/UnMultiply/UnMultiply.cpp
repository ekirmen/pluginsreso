#include "UnMultiply.h"

#include "ffglquickstart/FFGLParamBool.h"
#include "ffglquickstart/FFGLParamOption.h"
#include "ffglquickstart/FFGLParamRange.h"

using namespace ffglex;
using namespace ffglqs;

static CFFGLPluginInfo PluginInfo(
	PluginFactory< UnMultiply >,
	"UNMP",
	"UnMultiply",
	2,
	1,
	1,
	0,
	FF_EFFECT,
	"Extracts alpha from color and optionally de-premultiplies/premultiplies output.",
	"TheX" );

static const char _fragmentShaderCode[] = R"(
void main()
{
	vec4 base = texture( inputTexture, i_uv );
	vec3 rgb = base.rgb;

	if (Unpremult > 0.5 && base.a > 0.00001) {
		rgb /= base.a;
	}

	float wSum = abs(R_weight) + abs(G_weight) + abs(B_weight);
	vec3 safeWeights = vec3(R_weight, G_weight, B_weight);
	if (wSum < 0.00001) {
		safeWeights = vec3(0.0, 0.0, 1.0);
		wSum = 1.0;
	}

	float matte = dot(rgb, safeWeights) / wSum;
	matte = clamp(matte, 0.0, 1.0);

	float whiteCut = max(0.0, 1.0 - White_point);
	float denom = max(whiteCut - Black_clip, 0.00001);
	float shaped = clamp((matte - Black_clip) / denom, 0.0, 1.0);

	if (Feather > 0.00001) {
		float low = clamp(0.5 - Feather * 0.5, 0.0, 1.0);
		float high = clamp(0.5 + Feather * 0.5, 0.0, 1.0);
		shaped = smoothstep(low, high, shaped);
	}

	float outA = pow(shaped, max(Alpha_gamma, 0.00001));

	if (By_input_Alpha > 0.5) {
		outA *= base.a;
	}

	outA = clamp(outA * Opacity, 0.0, 1.0);

	float fringeMask = smoothstep(Fringe_knee, 1.0, 1.0 - outA);
	vec3 neutral = vec3(dot(rgb, vec3(0.333333)));
	vec3 fringeFixed = mix(rgb, neutral, clamp(Fringe_supp * fringeMask, 0.0, 1.0));

	vec3 outRGB = fringeFixed;
	if (Premult_output > 0.5) {
		outRGB *= outA;
	}

	if (Blend_mode > 0.5) {
		outRGB = vec3(outA);
	}

	fragColor = vec4(clamp(outRGB, 0.0, 1.0), outA);
}
)";

UnMultiply::UnMultiply()
{
	AddParam( ParamOption::Create( "Blend_mode", { { "Alpha" }, { "Matte Preview" } }, 0 ) );
	AddParam( ParamRange::Create( "Opacity", 1.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamOption::Create( "Model", { { "RGB" } }, 0 ) );
	AddParam( ParamRange::Create( "R_weight", 0.0f, ParamRange::Range( -2.0f, 2.0f ) ) );
	AddParam( ParamRange::Create( "G_weight", 0.0f, ParamRange::Range( -2.0f, 2.0f ) ) );
	AddParam( ParamRange::Create( "B_weight", 1.0f, ParamRange::Range( -2.0f, 2.0f ) ) );
	AddParam( ParamRange::Create( "Black_clip", 0.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "White_point", 0.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "Feather", 0.0f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "Alpha_gamma", 0.25f, ParamRange::Range( 0.01f, 4.0f ) ) );
	AddParam( ParamBool::Create( "Unpremult", false ) );
	AddParam( ParamBool::Create( "Premult_output", false ) );
	AddParam( ParamBool::Create( "By_input_Alpha", false ) );
	AddParam( ParamRange::Create( "Fringe_knee", 0.25f, ParamRange::Range( 0.0f, 1.0f ) ) );
	AddParam( ParamRange::Create( "Fringe_supp", 0.5f, ParamRange::Range( 0.0f, 1.0f ) ) );

	SetFragmentShader( _fragmentShaderCode );
}

UnMultiply::~UnMultiply()
{
}
