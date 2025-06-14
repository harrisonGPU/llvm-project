

define amdgpu_vs {<4 x float>, <4 x float>} @tbuffer_merge_test(<4 x i32> inreg %rsrc, i32 inreg %vid) {
entry:
  %rsrc.v4 = bitcast <4 x i32> %rsrc to <4 x i32>
  %ld0 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 0,  i32 0,  i32 13, i32 0)
  %ld1 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 2,  i32 0,  i32 13, i32 0)
  %ld2 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 4,  i32 0,  i32 13, i32 0)
  %ld3 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 6,  i32 0,  i32 13, i32 0)
  %ld4 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 16, i32 0,  i32 13, i32 0)
  %ld5 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 18, i32 0,  i32 13, i32 0)
  %ld6 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 20, i32 0,  i32 13, i32 0)
  %ld7 = call i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32> %rsrc.v4, i32 %vid, i32 22, i32 0,  i32 13, i32 0)
  %f0 = bitcast i32 %ld0 to float
  %f1 = bitcast i32 %ld1 to float
  %f2 = bitcast i32 %ld2 to float
  %f3 = bitcast i32 %ld3 to float
  %f4 = bitcast i32 %ld4 to float
  %f5 = bitcast i32 %ld5 to float
  %f6 = bitcast i32 %ld6 to float
  %f7 = bitcast i32 %ld7 to float
  %vA0 = insertelement <4 x float> poison, float %f0, i64 0
  %vA1 = insertelement <4 x float> %vA0,  float %f1, i64 1
  %vA2 = insertelement <4 x float> %vA1,  float %f2, i64 2
  %vA3 = insertelement <4 x float> %vA2,  float %f3, i64 3
  %vB0 = insertelement <4 x float> poison, float %f4, i64 0
  %vB1 = insertelement <4 x float> %vB0,  float %f5, i64 1
  %vB2 = insertelement <4 x float> %vB1,  float %f6, i64 2
  %vB3 = insertelement <4 x float> %vB2,  float %f7, i64 3
  %ret0 = insertvalue {<4 x float>, <4 x float>} poison, <4 x float> %vA3, 0
  %ret1 = insertvalue {<4 x float>, <4 x float>} %ret0,   <4 x float> %vB3, 1
  ret {<4 x float>, <4 x float>} %ret1
}

declare i32 @llvm.amdgcn.struct.tbuffer.load.i32(<4 x i32>, i32, i32, i32, i32, i32)
