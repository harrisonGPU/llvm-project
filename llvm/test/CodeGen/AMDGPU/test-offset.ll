;RUN: llc < %s -mtriple=amdgcn -mcpu=gfx1100 -verify-machineinstrs | FileCheck -check-prefixes=GFX11 %s

define amdgpu_vs {<4 x float>, <4 x float>} @tbuffer_swap_test(<4 x i32> inreg %rsrc, i32 inreg %vid) {
entry:
  %rsrc.v4 = bitcast <4 x i32> %rsrc to <4 x i32>
  %ldA3 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 6,  i32 0, i32 13, i32 0)
  %ldA2 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 4,  i32 0, i32 13, i32 0)
  %ldA1 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 2,  i32 0, i32 13, i32 0)
  %ldA0 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 0,  i32 0, i32 13, i32 0)
  %ldB3 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 22, i32 0, i32 13, i32 0)
  %ldB2 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 20, i32 0, i32 13, i32 0)
  %ldB1 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 18, i32 0, i32 13, i32 0)
  %ldB0 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 16, i32 0, i32 13, i32 0)
  %fA3 = bitcast i32 %ldA3 to float
  %fA2 = bitcast i32 %ldA2 to float
  %fA1 = bitcast i32 %ldA1 to float
  %fA0 = bitcast i32 %ldA0 to float
  %fB3 = bitcast i32 %ldB3 to float
  %fB2 = bitcast i32 %ldB2 to float
  %fB1 = bitcast i32 %ldB1 to float
  %fB0 = bitcast i32 %ldB0 to float
  %vA0 = insertelement <4 x float> poison, float %fA0, i32 0
  %vA1 = insertelement <4 x float> %vA0,   float %fA1, i32 1
  %vA2 = insertelement <4 x float> %vA1,   float %fA2, i32 2
  %vA3 = insertelement <4 x float> %vA2,   float %fA3, i32 3
  %vB0 = insertelement <4 x float> poison, float %fB0, i32 0
  %vB1 = insertelement <4 x float> %vB0,   float %fB1, i32 1
  %vB2 = insertelement <4 x float> %vB1,   float %fB2, i32 2
  %vB3 = insertelement <4 x float> %vB2,   float %fB3, i32 3
  %r0 = insertvalue {<4 x float>, <4 x float>} poison, <4 x float> %vA3, 0
  %r1 = insertvalue {<4 x float>, <4 x float>} %r0,    <4 x float> %vB3, 1
  ret {<4 x float>, <4 x float>} %r1
}

declare i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32>, i32, i32, i32, i32, i32)
