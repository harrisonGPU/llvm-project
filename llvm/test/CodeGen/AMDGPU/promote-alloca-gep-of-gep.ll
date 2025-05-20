; ModuleID = 'lgcPipeline'
source_filename = "lgcPipeline"
target datalayout = "e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-p7:160:256:256:32-p8:128:128-p9:192:256:256:32-p10:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-S32-A5-G1-ni:7:8:9"
target triple = "amdgcn--amdpal"

; Function Attrs: alwaysinline nounwind memory(readwrite)
define dllexport amdgpu_ps void @_amdgpu_ps_main(i32 inreg noundef %globalTable, i32 inreg noundef %userdata7, i32 inreg noundef %userdata9, i32 inreg noundef %PrimMask, <2 x float> noundef %PerspInterpSample, <2 x float> noundef %PerspInterpCenter, <2 x float> noundef %PerspInterpCentroid, <3 x float> noundef %PerspInterpPullMode, <2 x float> noundef %LinearInterpSample, <2 x float> noundef %LinearInterpCenter, <2 x float> noundef %LinearInterpCentroid, float noundef %LineStipple, float noundef %FragCoordX, float noundef %FragCoordY, float noundef %FragCoordZ, float noundef %FragCoordW, i32 noundef %FrontFacing, i32 noundef %Ancillary, i32 noundef %SampleCoverage, i32 noundef %FixedXY) #1 !spirv.ExecutionModel !7 !lgc.shaderstage !8 {
.entry:
  %SortedFragments = alloca [10 x <2 x i32>], align 8, addrspace(5)
  %0 = call i64 @llvm.amdgcn.s.getpc()
  %1 = and i64 %0, -4294967296
  %2 = zext i32 %userdata7 to i64
  %3 = or disjoint i64 %1, %2
  %4 = inttoptr i64 %3 to ptr addrspace(4)
  call void @llvm.assume(i1 true) [ "align"(ptr addrspace(4) %4, i32 4), "dereferenceable"(ptr addrspace(4) %4, i32 -1) ]
  %5 = load <4 x i32>, ptr addrspace(4) %4, align 16
  %6 = zext i32 %userdata9 to i64
  %7 = or disjoint i64 %1, %6
  %8 = inttoptr i64 %7 to ptr addrspace(4)
  call void @llvm.assume(i1 true) [ "align"(ptr addrspace(4) %8, i32 4), "dereferenceable"(ptr addrspace(4) %8, i32 -1) ]
  call void @llvm.assume(i1 true) [ "align"(ptr addrspace(4) %8, i32 4), "dereferenceable"(ptr addrspace(4) %8, i32 -1) ]
  store <2 x i32> zeroinitializer, ptr addrspace(5) %SortedFragments, align 8
  %.fca.1.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 8
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.1.gep, align 8
  %.fca.2.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 16
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.2.gep, align 8
  %.fca.3.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 24
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.3.gep, align 8
  %.fca.4.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 32
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.4.gep, align 8
  %.fca.5.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 40
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.5.gep, align 8
  %.fca.6.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 48
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.6.gep, align 8
  %.fca.7.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 56
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.7.gep, align 8
  %.fca.8.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 64
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.8.gep, align 8
  %.fca.9.gep = getelementptr inbounds nuw i8, ptr addrspace(5) %SortedFragments, i32 72
  store <2 x i32> zeroinitializer, ptr addrspace(5) %.fca.9.gep, align 8
  %9 = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %5, i32 1264, i32 0), !invariant.load !9
  %10 = call i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32> %5, i32 1272, i32 0), !invariant.load !9
  %.i0 = fptosi float %FragCoordX to i32
  %.i1 = fptosi float %FragCoordY to i32
  %11 = mul i32 %9, %.i1
  %12 = add i32 %11, %.i0
  %13 = load <8 x i32>, ptr addrspace(4) %8, align 4, !invariant.load !9
  %14 = call i32 @llvm.amdgcn.image.load.2d.i32.i32.v8i32(i32 1, i32 %.i0, i32 %.i1, <8 x i32> %13, i32 0, i32 0)
  %.fr44.i0 = freeze i32 %14
  %15 = call i32 @llvm.smin.i32(i32 %.fr44.i0, i32 10)
  %16 = icmp sgt i32 %.fr44.i0, 0
  br i1 %16, label %.lr.ph, label %.preheader1

.lr.ph:                                           ; preds = %.entry
  %17 = getelementptr i8, ptr addrspace(4) %8, i64 32
  %18 = load <4 x i32>, ptr addrspace(4) %17, align 4, !invariant.load !9
  br label %20

.preheader1:                                      ; preds = %20, %.entry
  %19 = icmp sgt i32 %.fr44.i0, 1
  br i1 %19, label %.preheader, label %._crit_edge

20:                                               ; preds = %.lr.ph, %20
  %i.02 = phi i32 [ 0, %.lr.ph ], [ %25, %20 ]
  %21 = getelementptr [10 x <2 x i32>], ptr addrspace(5) %SortedFragments, i32 0, i32 %i.02
  %22 = mul i32 %i.02, %10
  %23 = add i32 %12, %22
  %24 = call <2 x i32> @llvm.amdgcn.struct.buffer.load.format.v2i32.v4i32(<4 x i32> %18, i32 %23, i32 0, i32 0, i32 0)
  store <2 x i32> %24, ptr addrspace(5) %21, align 8
  %25 = add nuw nsw i32 %i.02, 1
  %exitcond.not = icmp eq i32 %25, %15
  br i1 %exitcond.not, label %.preheader1, label %20, !llvm.loop !10

.preheader:                                       ; preds = %.preheader1, %.critedge
  %i.16 = phi i32 [ %42, %.critedge ], [ 1, %.preheader1 ]
  br label %.lr.ph4

.lr.ph4:                                          ; preds = %.preheader, %33
  %j.03 = phi i32 [ %36, %33 ], [ %i.16, %.preheader ]
  %26 = add nsw i32 %j.03, -1
  %27 = getelementptr [10 x <2 x i32>], ptr addrspace(5) %SortedFragments, i32 0, i32 %26, i32 1
  %28 = load float, ptr addrspace(5) %27, align 4
  %29 = getelementptr [10 x <2 x i32>], ptr addrspace(5) %SortedFragments, i32 0, i32 %j.03, i32 1
  %30 = load float, ptr addrspace(5) %29, align 4
  %31 = fcmp olt float %28, %30
  %32 = freeze i1 %31
  br i1 %32, label %33, label %.critedge

33:                                               ; preds = %.lr.ph4
  %34 = getelementptr [10 x <2 x i32>], ptr addrspace(5) %SortedFragments, i32 0, i32 %j.03
  %.ii0 = load i32, ptr addrspace(5) %34, align 8
  %.i189 = getelementptr i8, ptr addrspace(5) %34, i32 4
  %.ii1 = load i32, ptr addrspace(5) %.i189, align 4
  %.upto052 = insertelement <2 x i32> poison, i32 %.ii0, i64 0
  %35 = insertelement <2 x i32> %.upto052, i32 %.ii1, i64 1
  %36 = add nsw i32 %j.03, -1
  %37 = getelementptr [10 x <2 x i32>], ptr addrspace(5) %SortedFragments, i32 0, i32 %36
  %.ii091 = load i32, ptr addrspace(5) %37, align 8
  %.i192 = getelementptr i8, ptr addrspace(5) %37, i32 4
  %.ii193 = load i32, ptr addrspace(5) %.i192, align 4
  %.upto053 = insertelement <2 x i32> poison, i32 %.ii091, i64 0
  %38 = insertelement <2 x i32> %.upto053, i32 %.ii193, i64 1
  %39 = getelementptr [10 x <2 x i32>], ptr addrspace(5) %SortedFragments, i32 0, i32 %j.03
  store <2 x i32> %38, ptr addrspace(5) %39, align 8
  %40 = getelementptr [10 x <2 x i32>], ptr addrspace(5) %SortedFragments, i32 0, i32 %36
  store <2 x i32> %35, ptr addrspace(5) %40, align 8
  %41 = icmp sgt i32 %j.03, 1
  br i1 %41, label %.lr.ph4, label %.critedge, !llvm.loop !11

.critedge:                                        ; preds = %33, %.lr.ph4
  %42 = add nuw nsw i32 %i.16, 1
  %exitcond15.not = icmp eq i32 %42, %15
  br i1 %exitcond15.not, label %._crit_edge, label %.preheader, !llvm.loop !12

._crit_edge:                                      ; preds = %.critedge, %.preheader1
  br i1 %16, label %43, label %46

43:                                               ; preds = %._crit_edge
  %44 = load <4 x i8>, ptr addrspace(5) %SortedFragments, align 8
  %.fr13 = freeze <4 x i8> %44
  %45 = uitofp <4 x i8> %.fr13 to <4 x float>
  %.i027 = extractelement <4 x float> %45, i64 0
  %.i028 = fmul reassoc nsz arcp contract afn float %.i027, 0x3F70101020000000
  %.i129 = extractelement <4 x float> %45, i64 1
  %.i130 = fmul reassoc nsz arcp contract afn float %.i129, 0x3F70101020000000
  %.i231 = extractelement <4 x float> %45, i64 2
  %.i232 = fmul reassoc nsz arcp contract afn float %.i231, 0x3F70101020000000
  %.i333 = extractelement <4 x float> %45, i64 3
  %.i334 = fmul reassoc nsz arcp contract afn float %.i333, 0x3F70101020000000
  br label %46

46:                                               ; preds = %43, %._crit_edge
  %color.0.i0 = phi float [ %.i028, %43 ], [ 0.000000e+00, %._crit_edge ]
  %color.0.i1 = phi float [ %.i130, %43 ], [ 0.000000e+00, %._crit_edge ]
  %color.0.i2 = phi float [ %.i232, %43 ], [ 0.000000e+00, %._crit_edge ]
  %color.0.i3 = phi float [ %.i334, %43 ], [ 0.000000e+00, %._crit_edge ]
  br i1 %19, label %.lr.ph11, label %._crit_edge12

.lr.ph11:                                         ; preds = %46, %.lr.ph11
  %color.1.fr9.i0 = phi float [ %color.1.fr.i0, %.lr.ph11 ], [ %color.0.i0, %46 ]
  %color.1.fr9.i1 = phi float [ %color.1.fr.i1, %.lr.ph11 ], [ %color.0.i1, %46 ]
  %color.1.fr9.i2 = phi float [ %color.1.fr.i2, %.lr.ph11 ], [ %color.0.i2, %46 ]
  %color.1.fr9.i3 = phi float [ %color.1.fr.i3, %.lr.ph11 ], [ %color.0.i3, %46 ]
  %i.28 = phi i32 [ %53, %.lr.ph11 ], [ 1, %46 ]
  %47 = getelementptr [10 x <2 x i32>], ptr addrspace(5) %SortedFragments, i32 0, i32 %i.28, i32 0
  %48 = load <4 x i8>, ptr addrspace(5) %47, align 8
  %49 = uitofp <4 x i8> %48 to <4 x float>
  %.i035 = extractelement <4 x float> %49, i64 0
  %.i036 = fmul reassoc nnan nsz arcp contract afn float %.i035, 0x3F70101020000000
  %.i137 = extractelement <4 x float> %49, i64 1
  %.i138 = fmul reassoc nnan nsz arcp contract afn float %.i137, 0x3F70101020000000
  %.i239 = extractelement <4 x float> %49, i64 2
  %.i240 = fmul reassoc nnan nsz arcp contract afn float %.i239, 0x3F70101020000000
  %.i341 = extractelement <4 x float> %49, i64 3
  %.i342 = fmul reassoc nnan nsz arcp contract afn float %.i341, 0x3F70101020000000
  %50 = fsub reassoc nnan nsz arcp contract afn float 1.000000e+00, %.i342
  %51 = fmul reassoc nnan nsz arcp contract afn float %50, %color.1.fr9.i3
  %52 = fadd reassoc nnan nsz arcp contract afn float %51, %.i342
  %scale.i0 = fmul reassoc nnan nsz arcp contract afn float %.i036, %.i342
  %scale.i1 = fmul reassoc nnan nsz arcp contract afn float %.i138, %.i342
  %scale.i2 = fmul reassoc nnan nsz arcp contract afn float %.i240, %.i342
  %scale7.i0 = fmul reassoc nnan nsz arcp contract afn float %50, %color.1.fr9.i0
  %scale7.i1 = fmul reassoc nnan nsz arcp contract afn float %50, %color.1.fr9.i1
  %scale7.i2 = fmul reassoc nnan nsz arcp contract afn float %50, %color.1.fr9.i2
  %.i043 = fadd reassoc nnan nsz arcp contract afn float %scale7.i0, %scale.i0
  %.i144 = fadd reassoc nnan nsz arcp contract afn float %scale7.i1, %scale.i1
  %.i245 = fadd reassoc nnan nsz arcp contract afn float %scale7.i2, %scale.i2
  %53 = add nuw nsw i32 %i.28, 1
  %color.1.fr.i0 = freeze float %.i043
  %color.1.fr.i1 = freeze float %.i144
  %color.1.fr.i2 = freeze float %.i245
  %color.1.fr.i3 = freeze float %52
  %exitcond16.not = icmp eq i32 %53, %15
  br i1 %exitcond16.not, label %._crit_edge12, label %.lr.ph11, !llvm.loop !13

._crit_edge12:                                    ; preds = %.lr.ph11, %46
  %color.1.fr.lcssa.i0 = phi float [ %color.0.i0, %46 ], [ %color.1.fr.i0, %.lr.ph11 ]
  %color.1.fr.lcssa.i1 = phi float [ %color.0.i1, %46 ], [ %color.1.fr.i1, %.lr.ph11 ]
  %color.1.fr.lcssa.i2 = phi float [ %color.0.i2, %46 ], [ %color.1.fr.i2, %.lr.ph11 ]
  %color.1.fr.lcssa.i3 = phi float [ %color.0.i3, %46 ], [ %color.1.fr.i3, %.lr.ph11 ]
  %54 = fcmp oeq float %color.1.fr.lcssa.i3, 0.000000e+00
  br i1 %54, label %55, label %56

55:                                               ; preds = %._crit_edge12
  call void @llvm.amdgcn.kill(i1 false)
  br label %61

56:                                               ; preds = %._crit_edge12
  %57 = call reassoc nnan nsz arcp contract afn float @llvm.amdgcn.fmed3.f32(float %color.1.fr.lcssa.i0, float 0.000000e+00, float 1.000000e+00)
  %58 = call reassoc nnan nsz arcp contract afn float @llvm.amdgcn.fmed3.f32(float %color.1.fr.lcssa.i1, float 0.000000e+00, float 1.000000e+00)
  %59 = call reassoc nnan nsz arcp contract afn float @llvm.amdgcn.fmed3.f32(float %color.1.fr.lcssa.i2, float 0.000000e+00, float 1.000000e+00)
  %60 = call reassoc nnan nsz arcp contract afn float @llvm.amdgcn.fmed3.f32(float %color.1.fr.lcssa.i3, float 0.000000e+00, float 1.000000e+00)
  br label %61

61:                                               ; preds = %56, %55
  %__llpc_input_proxy_ColorOut.0.i0 = phi float [ undef, %55 ], [ %57, %56 ]
  %__llpc_input_proxy_ColorOut.0.i1 = phi float [ undef, %55 ], [ %58, %56 ]
  %__llpc_input_proxy_ColorOut.0.i2 = phi float [ undef, %55 ], [ %59, %56 ]
  %__llpc_input_proxy_ColorOut.0.i3 = phi float [ undef, %55 ], [ %60, %56 ]
  call void @llvm.amdgcn.exp.f32(i32 8, i32 1, float undef, float poison, float poison, float poison, i1 false, i1 true)
  %62 = call <2 x half> @llvm.amdgcn.cvt.pkrtz(float %__llpc_input_proxy_ColorOut.0.i0, float %__llpc_input_proxy_ColorOut.0.i1)
  %63 = call <2 x half> @llvm.amdgcn.cvt.pkrtz(float %__llpc_input_proxy_ColorOut.0.i2, float %__llpc_input_proxy_ColorOut.0.i3)
  %64 = bitcast <2 x half> %62 to float
  %65 = bitcast <2 x half> %63 to float
  call void @llvm.amdgcn.exp.f32(i32 0, i32 3, float %64, float %65, float poison, float poison, i1 true, i1 true)
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare ptr @llvm.invariant.start.p7(i64 immarg, ptr addrspace(7) captures(none)) #2

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smin.i32(i32, i32) #3

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare noundef i64 @llvm.amdgcn.s.getpc() #3

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #4

; Function Attrs: nocallback nofree nounwind
declare void @llvm.amdgcn.kill(i1) #5

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare float @llvm.amdgcn.fmed3.f32(float, float, float) #3

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.amdgcn.exp.f32(i32 immarg, i32 immarg, float, float, float, float, i1 immarg, i1 immarg) #4

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare <2 x half> @llvm.amdgcn.cvt.pkrtz(float, float) #3

; Function Attrs: nodivergencesource nounwind willreturn memory(none)
declare ptr addrspace(7) @lgc.buffer.load.desc.to.ptr(ptr addrspace(4), i1, i1, i1) #6

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(read)
declare <2 x i32> @llvm.amdgcn.struct.buffer.load.format.v2i32.v4i32(<4 x i32>, i32, i32, i32, i32 immarg) #7

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(read)
declare i32 @llvm.amdgcn.image.load.2d.i32.i32.v8i32(i32 immarg, i32, i32, <8 x i32>, i32 immarg, i32 immarg) #7

; Function Attrs: convergent nocallback nofree nounwind willreturn
declare void @llvm.amdgcn.init.exec(i64 immarg) #8

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(none)
declare i32 @llvm.amdgcn.mbcnt.lo(i32, i32) #9

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(none)
declare i32 @llvm.amdgcn.mbcnt.hi(i32, i32) #9

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.amdgcn.exp.i32(i32 immarg, i32 immarg, i32, i32, i32, i32, i1 immarg, i1 immarg) #4

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(none)
declare i32 @llvm.amdgcn.s.buffer.load.i32(<4 x i32>, i32, i32 immarg) #9

attributes #0 = { "amdgpu-flat-work-group-size"="128,128" "amdgpu-memory-bound"="false" "amdgpu-prealloc-sgpr-spill-vgprs" "amdgpu-promote-alloca-to-vector-max-regs"="32" "amdgpu-unroll-threshold"="700" "amdgpu-wave-limiter"="false" "denormal-fp-math-f32"="preserve-sign" "target-features"=",+DumpCode,+wavefrontsize64,-cumode" }
attributes #1 = { alwaysinline nounwind memory(readwrite) "InitialPSInputAddr"="3840" "amdgpu-color-export"="1" "amdgpu-depth-export"="1" "amdgpu-memory-bound"="false" "amdgpu-prealloc-sgpr-spill-vgprs" "amdgpu-promote-alloca-to-vector-max-regs"="32" "amdgpu-unroll-threshold"="700" "amdgpu-wave-limiter"="false" "denormal-fp-math-f32"="preserve-sign" "target-features"=",+DumpCode,+wavefrontsize64,+cumode" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #3 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #4 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #5 = { nocallback nofree nounwind }
attributes #6 = { nodivergencesource nounwind willreturn memory(none) }
attributes #7 = { nocallback nofree nosync nounwind willreturn memory(read) }
attributes #8 = { convergent nocallback nofree nounwind willreturn }
attributes #9 = { nocallback nofree nosync nounwind willreturn memory(none) }

!lgc.client = !{!0}
!lgc.options.VS = !{!1}
!lgc.options.FS = !{!2}
!lgc.tessellation.level.state = !{!3}
!lgc.wave.size = !{!4}
!lgc.subgroup.size = !{!4}
!amdgpu.pal.metadata.msgpack = !{!5}

!0 = !{!"OpenGL"}
!1 = !{i32 499169782, i32 1887314908, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 64, i32 0, i32 0, i32 3, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 20, i32 1800, i32 0, i32 0, i32 1}
!2 = !{i32 -1341484304, i32 -980843929, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 64, i32 64, i32 0, i32 0, i32 3, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 20, i32 1800, i32 0, i32 0, i32 1}
!3 = !{i32 1065353216, i32 1065353216, i32 1065353216, i32 1065353216, i32 1065353216, i32 1065353216}
!4 = !{i32 32, i32 64, i32 64, i32 64, i32 64, i32 32, i32 64, i32 32}
!5 = !{!"\82\B0amdpal.pipelines\91\8B\A4.api\A6OpenGL\B3.graphics_registers\DE\00\1C\BD.aa_coverage_to_shader_select\ADInputCoverage\AF.cb_shader_mask\88\AF.output0_enable\0F\AF.output1_enable\00\AF.output2_enable\00\AF.output3_enable\00\AF.output4_enable\00\AF.output5_enable\00\AF.output6_enable\00\AF.output7_enable\00\B2.db_shader_control\8C\B6.alpha_to_mask_disable\C3\B6.conservative_z_export\00\B4.depth_before_shader\00\B2.exec_on_hier_fail\C2\AD.exec_on_noop\C2\AC.kill_enable\C3\B3.mask_export_enable\C2\D9!.pre_shader_depth_coverage_enable\00\BF.primitive_ordered_pixel_shader\C2\BF.stencil_test_val_export_enable\00\B0.z_export_enable\01\A8.z_order\01\B1.es_vgpr_comp_cnt\00\B3.ge_ngg_subgrp_cntl\82\B0.prim_amp_factor\01\B5.threads_per_subgroup\CD\01\00\B1.gs_vgpr_comp_cnt\01\B3.ia_multi_vgt_param\81\AF.primgroup_size\7F\B7.max_verts_per_subgroup\CC\80\B0.pa_cl_clip_cntl\83\B8.dx_linear_attr_clip_ena\C3\B3.rasterization_kill\C2\BA.vte_vport_provoke_disable\C2\AF.pa_cl_vte_cntl\87\AB.vtx_w0_fmt\C3\AD.x_offset_ena\C3\AC.x_scale_ena\C3\AD.y_offset_ena\C3\AC.y_scale_ena\C3\AD.z_offset_ena\C3\AC.z_scale_ena\C3\AF.pa_su_vtx_cntl\83\AB.pix_center\01\AB.quant_mode\05\AB.round_mode\02\AF.ps_iter_sample\C2\AF.spi_baryc_cntl\82\B4.front_face_all_bits\C3\B3.pos_float_location\00\B2.spi_ps_in_control\82\AC.num_interps\00\AA.ps_w32_en\C2\B2.spi_ps_input_cntl\91\87\AC.attr0_valid\00\AC.attr1_valid\00\AB.flat_shade\C2\B1.fp16_interp_mode\C2\A7.offset\00\AA.prim_attr\C2\AE.pt_sprite_tex\C2\B6.spi_shader_col_format\88\B4.col_0_export_format\04\B4.col_1_export_format\00\B4.col_2_export_format\00\B4.col_3_export_format\00\B4.col_4_export_format\00\B4.col_5_export_format\00\B4.col_6_export_format\00\B4.col_7_export_format\00\B6.spi_shader_idx_format\01\B6.spi_shader_pos_format\95\04\00\00\00\00\B4.spi_shader_z_format\01\B2.spi_vs_out_config\81\AD.no_pc_export\C3\B7.vgt_esgs_ring_itemsize\01\B4.vgt_gs_max_vert_out\01\AC.vgt_gs_mode\84\B2.es_write_optimize\C2\B2.gs_write_optimize\C3\A5.mode\03\A7.onchip\01\B3.vgt_gs_onchip_cntl\83\B6.es_verts_per_subgroup\CC\80\B9.gs_inst_prims_per_subgrp\CC\80\B6.gs_prims_per_subgroup\CC\80\B5.vgt_gs_out_prim_type\81\AD.outprim_type\A8TriStrip\AE.vgt_gs_per_vs\02\AE.vgt_reuse_off\C2\B5.vgt_shader_stages_en\88\AC.es_stage_en\02\AA.gs_w32_en\C2\B6.max_primgroup_in_wave\02\AF.ngg_wave_id_en\C2\AB.primgen_en\C3\B4.primgen_passthru_en\C3\B8.primgen_passthru_no_msg\C3\AC.vs_stage_en\00\B0.hardware_stages\82\A3.gs\8D\AF.checksum_value\CEm\BE\9A*\AB.debug_mode\00\AB.float_mode\CC\C0\A9.image_op\C2\AC.mem_ordered\C3\AF.offchip_lds_en\C2\AB.sgpr_limitj\AD.trap_present\00\B2.user_data_reg_map\DC\00 \CE\10\00\00\00\CE\10\00\00\03\CE\10\00\00\04\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\AB.user_sgprs\03\AB.vgpr_limit\CD\01\00\AF.wavefront_size@\A9.wgp_mode\C3\A3.ps\8E\AF.checksum_value\CEu\83\10\97\AB.debug_mode\00\AB.float_mode\CC\C0\A9.image_op\C3\AC.mem_ordered\C3\AB.sgpr_limitj\AD.trap_present\00\B2.user_data_reg_map\DC\00 \CE\10\00\00\00\07\09\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\CE\FF\FF\FF\FF\AB.user_sgprs\03\AA.uses_uavs\C3\AB.vgpr_limit\CD\01\00\AF.wavefront_size@\AD.writes_depth\01\AC.writes_uavs\C2\B7.internal_pipeline_hash\92\CF\10\E3\AA,\B51[~\CF\BA\84\E0n:S\98:\B0.nggSubgroupSize\CC\80\B1.num_interpolants\01\A8.shaders\82\A6.pixel\82\B0.api_shader_hash\92\CF\C5\89\82g\B0\0A\92\F0\00\B1.hardware_mapping\91\A3.ps\A7.vertex\82\B0.api_shader_hash\92\CFp~#\DC\1D\C0\B9\F6\00\B1.hardware_mapping\91\A3.gs\B0.spill_threshold\CD\FF\FF\A5.type\A3Ngg\B0.user_data_limit\0A\AF.xgl_cache_info\82\B3.128_bit_cache_hash\92\CF\13\1Ct\EA\D0\C9\DBu\CF\14\A2\F2\ED={\EA4\AD.llpc_version\A476.0\AEamdpal.version\92\03\00"}
!6 = !{i32 1}
!7 = !{i32 4}
!8 = !{i32 6}
!9 = !{}
!10 = distinct !{!10}
!11 = distinct !{!11}
!12 = distinct !{!12}
!13 = distinct !{!13}
