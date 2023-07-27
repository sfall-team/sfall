@ECHO OFF

fxc /nologo /T fx_2_0 /Fh HLSL\A8PixelShader.h /Vn gpuEffectA8 HLSL\A8PixelShader.hlsl
fxc /nologo /T fx_2_0 /Fh HLSL\L8PixelShader.h /Vn gpuEffectL8 HLSL\L8PixelShader.hlsl