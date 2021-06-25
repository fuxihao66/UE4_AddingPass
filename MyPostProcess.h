#pragma once

#include "ScreenPass.h"


enum PassType{
	SCREEN_PASS = 0,
	VS_PS = 1,
	COMPUTE_PASS = 2
};


void AddMyCustomPostProcess(FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FRDGTextureRef& ColorTexture,
	FRHICommandListImmediate& RHICmdList,
	PassType passType);