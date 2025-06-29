//===-- AMDGPUExtendDynamicIndexAlloca.cpp --------------------------------===//
//
// This pass splits interesting allocas (struct members / array elements
// whose types are vectors or arrays-of-vectors) and linearizes dynamic
// indexing into constant-index chains of GEPs so later passes can
// vector-promote them.
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "amdgpu-extend-dynamic-index-alloca"

//===----------------------------------------------------------------------===//
// Implementation class
//===----------------------------------------------------------------------===//

namespace {

class AMDGPUExtendDynamicIndexAllocaImpl {
  const TargetMachine &TM;
  LoopInfo &LI;
  Module *Mod = nullptr;
  const DataLayout *DL = nullptr;

  /* helpers ------------------------------------------------------------- */
  static bool isVectorLike(Type *Ty);

  bool splitStruct(AllocaInst *AI, SmallVectorImpl<AllocaInst *> &Extra);

  bool shouldExpand(GetElementPtrInst *GEP, unsigned &OpIdx, unsigned &Bound,
                    unsigned Threshold = 8) const;

  void expandGEP(GetElementPtrInst *GEP, unsigned OpIdx, unsigned Bound) const;

  void rewriteGEPsForAlloca(AllocaInst &AI);

public:
  AMDGPUExtendDynamicIndexAllocaImpl(const TargetMachine &TM, LoopInfo &LI)
      : TM(TM), LI(LI) {}

  bool run(Function &F);
};

} // end anonymous namespace

namespace {

class AMDGPUExtendDynamicIndexAlloca : public FunctionPass {
public:
  static char ID;
  AMDGPUExtendDynamicIndexAlloca() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    auto *TPC = getAnalysisIfAvailable<TargetPassConfig>();
    if (!TPC)
      return false;

    auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    return AMDGPUExtendDynamicIndexAllocaImpl(TPC->getTM<TargetMachine>(), LI)
        .run(F);
  }

  StringRef getPassName() const override {
    return "AMDGPU Extend Dynamic Index Alloca";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
    FunctionPass::getAnalysisUsage(AU);
  }
};

} // end anonymous namespace

char AMDGPUExtendDynamicIndexAlloca::ID = 0;
char &llvm::AMDGPUExtendDynamicIndexAllocaID =
    AMDGPUExtendDynamicIndexAlloca::ID;

INITIALIZE_PASS_BEGIN(AMDGPUExtendDynamicIndexAlloca, DEBUG_TYPE,
                      "AMDGPU Extend Dynamic Index Alloca", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(AMDGPUExtendDynamicIndexAlloca, DEBUG_TYPE,
                    "AMDGPU Extend Dynamic Index Alloca", false, false)

FunctionPass *llvm::createAMDGPUExtendDynamicIndexAlloca() {
  return new AMDGPUExtendDynamicIndexAlloca();
}

PreservedAnalyses
AMDGPUExtendDynamicIndexAllocaPass::run(Function &F,
                                        FunctionAnalysisManager &AM) {
  auto &LI = AM.getResult<LoopAnalysis>(F);

  bool Changed = AMDGPUExtendDynamicIndexAllocaImpl(TM, LI).run(F);

  if (!Changed)
    return PreservedAnalyses::all();

  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}

bool AMDGPUExtendDynamicIndexAllocaImpl::run(Function &F) {
    llvm::errs() << "Hello world!\n";
    return false;
}
