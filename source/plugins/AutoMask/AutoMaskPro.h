#pragma once
#include "ffglquickstart/FFGLEffect.h"
#include <string>
#include <vector>

class AutoMaskPro : public ffglqs::Effect
{
public:
	AutoMaskPro();
	~AutoMaskPro();

	void Update() override;
	FFResult SetFloatParameter( unsigned int index, float value ) override;

private:
	void RegisterUniforms();
	void ValidateParameters();

	// Cache de valores
	float lastThreshold1;
	float lastThreshold2;
	float lastThreshold3;
	float lastSoftness;
	bool isValidating;

	// Índices de parámetros
	unsigned int idxAutoPick;
	unsigned int idxKeyColor;
	unsigned int idxThreshold1;
	unsigned int idxEnable2;
	unsigned int idxKeyColor2;
	unsigned int idxThreshold2;
	unsigned int idxEnable3;
	unsigned int idxKeyColor3;
	unsigned int idxThreshold3;
	unsigned int idxSoftness;
	unsigned int idxLogoProtect;
	unsigned int idxByInputAlpha;
	unsigned int idxInvertAlpha;
	unsigned int idxPreviewMode;
	unsigned int idxSelectionMode;
	unsigned int idxInstaLink;
	unsigned int idxStoreLink;
};
