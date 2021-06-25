// FComputeShaderUtils::AddPass 是GraphBuilder.AddPass的compute shader的封装

#include "MyPostProcess.h"

// TODO: 绑定SRV需要GraphBuilder.CreateSRV吗
//  如果资源声明为SHADER_PARAMETER_TEXTURE(Texture2D, SSProfilesTexture) 不需要 
// 如果资源声明为 SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer, NodeData) 则需要

class FMyPostProcessShader : public FGlobalShader
{
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	FMyPostProcessShader() = default;
	FMyPostProcessShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{}
};

// ----------------------------- pixel shader 定义---------------------------------------------------

BEGIN_SHADER_PARAMETER_STRUCT(FMyPostProcessParameters, )
	// SHADER_PARAMETER(uint32, ScatterPass)
	// SHADER_PARAMETER_RDG_TEXTURE(Texture2D, VelocityTileTexture)
    SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, InputInfo)
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, gSceneColor)
	RENDER_TARGET_BINDING_SLOTS() // 添加了render target slot
END_SHADER_PARAMETER_STRUCT()


class FQuadVS : public FMyPostProcessShader
{
public:
	DECLARE_GLOBAL_SHADER(FQuadVS);
	SHADER_USE_PARAMETER_STRUCT(FQuadVS, FMyPostProcessShader);
	using FParameters = FMyPostProcessParameters;
};

IMPLEMENT_GLOBAL_SHADER(FQuadVS, "/Engine/Private/PostBlur.usf", "MyVS", SF_Vertex);

class FMyPostProcessPS : public FMyPostProcessShader
{
public:
	DECLARE_GLOBAL_SHADER(FMyPostProcessPS);
	SHADER_USE_PARAMETER_STRUCT(FMyPostProcessPS, FMyPostProcessShader);
	using FParameters = FMyPostProcessParameters;
};

IMPLEMENT_GLOBAL_SHADER(FMyPostProcessPS, "/Engine/Private/PostBlur.usf", "MyPS", SF_Pixel);

// -------------------------------screen pass -----------------------------------------------

class FMyPostProcessScreenPassPS : public FMyPostProcessShader
{
public:
	DECLARE_GLOBAL_SHADER(FMyPostProcessPS);
	SHADER_USE_PARAMETER_STRUCT(FMyPostProcessPS, FMyPostProcessShader);
	using FParameters = FMyPostProcessParameters;
};

IMPLEMENT_GLOBAL_SHADER(FMyPostProcessScreenPassPS, "/Engine/Private/PostBlurScreenPass.usf", "MyPS", SF_Pixel);

// ----------------------------- compute shader 定义---------------------------------------------------

class FMyPostProcessCS : public FMyPostProcessShader
{
public:



	// static const uint32 ThreadGroupSize = 16;

	DECLARE_GLOBAL_SHADER(FMyPostProcessCS);
	SHADER_USE_PARAMETER_STRUCT(FMyPostProcessCS, FMyPostProcessShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		// SHADER_PARAMETER_STRUCT(FMotionBlurParameters, MotionBlur)
		// SHADER_PARAMETER_RDG_TEXTURE(Texture2D, VelocityTexture)
		// SHADER_PARAMETER_RDG_TEXTURE(Texture2D, DepthTexture)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, gSceneColor)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, gFilterResult)
        SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, InputInfo)
        // SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)

	END_SHADER_PARAMETER_STRUCT()
};
IMPLEMENT_GLOBAL_SHADER(FMyPostProcessCS, "/Engine/Private/PostBlurComputeShader.usf", "MyMain", SF_Compute);



// 添加pass
void AddMyCustomPostProcess(FRDGBuilder& GraphBuilder,
	const FViewInfo& View,
	FRDGTextureRef ColorTexture,
	FRHICommandListImmediate& RHICmdList,
    PassType passType)
{
    // 复制color Texture
    // 把结果写到color texture

	// TODO: 如何用gbuffer，如何绑定gbuffer   看直接光照的部分 
	// 
	FRDGTextureDesc Desc = FRDGTextureDesc::Create2DDesc(
        ColorTexture->Desc.Extent,
		PF_FloatRGBA,
        FClearValueBinding::None,
        /* InFlags = */ TexCreate_None,
        /* InTargetableFlags = */ TexCreate_ShaderResource | TexCreate_UAV,
        /* bInForceSeparateTargetAndShaderResource = */ false);

    auto copiedTexture  = GraphBuilder.CreateTexture(Desc, TEXT("copied screen texture"));
	RHICmdList.CopyToResolveTarget(ColorTexture, copiedTexture, FResolveRect(0, 0, FamilySize.X, FamilySize.Y));


	switch (passType){
		// case PassType::VS_PS:
		// 	// FRDGTextureRef VelocityTileDepthTexture =
		// 	// 	GraphBuilder.CreateTexture(
		// 	// 		FRDGTextureDesc::Create2DDesc(
		// 	// 			VelocityTileCount,
		// 	// 			PF_ShadowDepth,
		// 	// 			FClearValueBinding::DepthOne,
		// 	// 			TexCreate_None,
		// 	// 			TexCreate_DepthStencilTargetable,
		// 	// 			false),
		// 	// 		TEXT("DilatedVelocityDepth"));

		// 	FMyPostProcessParameters* PassParameters = GraphBuilder.AllocParameters<FMyPostProcessParameters>();
		// 	// PassParameters->Dilate = VelocityDilateParameters;

		// 	// PassParameters->RenderTargets.DepthStencil =
		// 	// 	FDepthStencilBinding(
		// 	// 		VelocityTileDepthTexture,
		// 	// 		ERenderTargetLoadAction::EClear,
		// 	// 		ERenderTargetLoadAction::ENoAction,
		// 	// 		FExclusiveDepthStencil::DepthWrite_StencilNop);
		// 	PassParameters->gSceneColor = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(copiedTexture));
		// 	PassParameters->RenderTargets[0] = FRenderTargetBinding(ColorTexture, View.GetOverwriteLoadAction());
		// 	PassParameters->InputInfo = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(ColorTexture->Desc.Extent, View.ViewRect));

		// 	// PassParameters->RenderTargets[0] =
		// 	// 	FRenderTargetBinding(
		// 	// 		ColorTexture,
		// 	// 		ERenderTargetLoadAction::ENoAction);

		// 	TShaderMapRef<FQuadVS> VertexShader(View.ShaderMap);
		// 	TShaderMapRef<FMyPostProcessPS> PixelShader(View.ShaderMap);
			
		// 	ValidateShaderParameters(*VertexShader, *PassParameters);
		// 	ValidateShaderParameters(*PixelShader, *PassParameters);

		// 	GraphBuilder.AddPass(
		// 		RDG_EVENT_NAME("MyPostProcess"),
		// 		PassParameters,
		// 		ERDGPassFlags::Raster,
		// 		[VertexShader, PixelShader, PassParameters](FRHICommandListImmediate& RHICmdList)
		// 	{
		// 		FRHIVertexShader* RHIVertexShader = GETSAFERHISHADER_VERTEX(*VertexShader);

		// 		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		// 		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
		// 		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = RHIVertexShader;
		// 		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		// 		GraphicsPSOInit.PrimitiveType = PT_TriangleList;
		// 		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		// 		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);


		// 		const EMotionBlurVelocityScatterPass ScatterPass = static_cast<EMotionBlurVelocityScatterPass>(ScatterPassIndex);

		// 		if (ScatterPass == EMotionBlurVelocityScatterPass::DrawMin)
		// 		{
		// 			GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask<CW_RGBA>::GetRHI();
		// 			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<true, CF_Less>::GetRHI();
		// 		}
		// 		else
		// 		{
		// 			GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask<CW_BA>::GetRHI();
		// 			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<true, CF_Greater>::GetRHI();
		// 		}

		// 		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);


		// 		SetShaderParameters(RHICmdList, *VertexShader, RHIVertexShader, *PassParameters);
		// 		SetShaderParameters(RHICmdList, *PixelShader, PixelShader->GetPixelShader(), *PassParameters);

		// 		// Needs to be the same on shader side (faster on NVIDIA and AMD)
		// 		const int32 QuadsPerInstance = 8;

		// 		RHICmdList.SetStreamSource(0, nullptr, 0);
		// 		RHICmdList.DrawIndexedPrimitive(GScatterQuadIndexBuffer.IndexBufferRHI, 0, 0, 32, 0, 2 * QuadsPerInstance, FMath::DivideAndRoundUp(VelocityTileCount.X * VelocityTileCount.Y, QuadsPerInstance));
		// 	});
		// 	break;
		case PassType::SCREEN_PASS:
			FMyPostProcessScreenPassPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FMyPostProcessScreenPassPS::FParameters>();
			PassParameters->gSceneColor = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(copiedTexture));
			PassParameters->RenderTargets[0] = FRenderTargetBinding(ColorTexture, View.GetOverwriteLoadAction());
			PassParameters->InputInfo = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(ColorTexture->Desc.Extent, View.ViewRect));

			TShaderMapRef<FMyPostProcessScreenPassPS> PixelShader(View.ShaderMap, PermutationVector);

			AddDrawScreenPass(
				GraphBuilder,
				RDG_EVENT_NAME("My postprocess screen pass"),
				View,
				FScreenPassTextureViewport(ColorTexture),
				FScreenPassTextureViewport(ColorTexture),
				*PixelShader,
				PassParameters,
				EScreenPassDrawFlags::AllowHMDHiddenAreaMask);

			break;
		case PassType::COMPUTE_PASS:

			FMyPostProcessCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FMyPostProcessCS::FParameters>();

			PassParameters->gSceneColor = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(copiedTexture));
			PassParameters->gFilterResult = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(ColorTexture));

			PassParameters->View = View.ViewUniformBuffer;
			PassParameters->InputInfo = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(ColorTexture->Desc.Extent, View.ViewRect));

			// PassParameters->PrevClosestDepthOutput = GraphBuilder.CreateUAV(PrevClosestDepthTexture);

			TShaderMapRef<FMyPostProcessCS> ComputeShader(View.ShaderMap);

			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("Filter"),
				*ComputeShader,
				PassParameters,
				FComputeShaderUtils::GetGroupCount(View.ViewRect.Size(), 8));
	}
	




}



