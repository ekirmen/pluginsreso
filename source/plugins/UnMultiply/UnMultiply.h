#pragma once
#include "ffglquickstart/FFGLEffect.h"

class AutoMaskPro : public ffglqs::Effect
{
public:
	AutoMaskPro();
	~AutoMaskPro();

	void Update() override;
};