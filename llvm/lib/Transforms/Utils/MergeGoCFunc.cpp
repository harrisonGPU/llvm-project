//===-------- MergeGoCFunc.cpp - Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MergeGoCFunc.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

#define DEBUG_TYPE "merge-go-c-func"

static cl::opt<bool> RenameCallerGc("rename-caller-gc", cl::init(false),
                                    cl::desc("Rename the Go caller functions"));

static cl::opt<bool> RenameCalleeGc("rename-callee-gc", cl::init(false),
                                    cl::desc("Rename the Go callee functions"));

static cl::opt<bool>
    MergeCalleeGc("merge-callee-gc", cl::init(false),
                  cl::desc("Merge the given callee functions"));

static cl::opt<std::string> CallerNameGc("caller-name-rra", cl::Hidden,
                                         cl::desc("Caller function name"),
                                         cl::init(""));

static cl::opt<std::string> CalleeNameGc("callee-name-rra", cl::Hidden,
                                         cl::desc("Callee function name"),
                                         cl::init(""));

PreservedAnalyses MergeGoCFuncPass::run(Module &M, ModuleAnalysisManager &AM) {
  bool Changed = false;
  if (RenameCallerGc) {
    if (CallerNameGc.empty()) {
      llvm::errs()
          << "RenameCaller Error: didn't specify caller function name\n";
    }
    renameCaller(&M);
    Changed = true;
  } else if (RenameCalleeGc) {
    if (CalleeNameGc.empty()) {
      llvm::errs()
          << "RenameCallee Error: didn't specify callee function name\n";
    }
    renameCallee(&M);
    Changed = true;
  } else if (MergeCalleeGc) {
    cloneAndReplaceFunc(&M);
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

void MergeGoCFuncPass::renameFunctionMainClosure(Module *M,
                                                 std::string Suffix) {
  std::vector<Function *> MainFuncs;
  for (Module::iterator F = M->begin(); F != M->end(); F++) {
    Function *Func = dyn_cast<Function>(F);
    if (Func->isIntrinsic())
      continue;

    std::string FuncName = Func->getName().str();
    FuncName = FuncName + "_" + Suffix;
    Func->setName(FuncName);
  }
}

void MergeGoCFuncPass::renameCaller(Module *M) {
  std::string Suffix = CallerNameGc;
  for (Module::iterator F = M->begin(); F != M->end(); F++) {
    Function *Func = dyn_cast<Function>(F);
    if (Func->isIntrinsic())
      continue;
    std::string FuncName = Func->getName().str();
    FuncName = FuncName + "_" + Suffix;
    Func->setName(FuncName);
  }
}

void MergeGoCFuncPass::renameCallee(Module *M) {
  Function *MainFunc = M->getFunction("main");
  renameRealCallee(MainFunc, "main_2nd_for_" + CalleeNameGc);
  MainFunc->setName("main_for_" + CalleeNameGc);
  renameFunctionMainClosure(M, CalleeNameGc);
}

void MergeGoCFuncPass::cloneAndReplaceFunc(Module *M) {
  Function *MainFunc = M->getFunction("main");
  if (!MainFunc) {
    errs() << "Function 'main' not found!\n";
    return;
  }

  LLVMContext &Context = M->getContext();
  std::vector<Type *> ParamTypes;
  ParamTypes.push_back(Type::getInt8PtrTy(Context));
  ParamTypes.push_back(Type::getInt8PtrTy(Context));

  FunctionType *NewFuncType =
      FunctionType::get(MainFunc->getReturnType(), ParamTypes, false);

  std::string NewFuncName = MainFunc->getName().str() + "_callee";
  GlobalValue::LinkageTypes Linkage = MainFunc->getLinkage();

  Function *newCalleeFunc =
      Function::Create(NewFuncType, Linkage, NewFuncName, M);

  auto ArgIter = newCalleeFunc->arg_begin();
  ArgIter->setName("arg1");
  (++ArgIter)->setName("arg2");

  newCalleeFunc->copyAttributesFrom(MainFunc);

  ValueToValueMapTy VMap;

  SmallVector<ReturnInst *, 8> Returns;
  CloneFunctionInto(newCalleeFunc, MainFunc, VMap,
                    CloneFunctionChangeType::LocalChangesOnly, Returns);

  errs() << "Function '" << MainFunc->getName() << "' cloned to '"
         << newCalleeFunc->getName() << "' with two additional arguments.\n";
  return;
}

void MergeGoCFuncPass::renameRealCallee(Function *MainFunc,
                                        std::string NewCalleeName) {
  if (!MainFunc) {
    errs() << "renameRealCallee Error: MainFunc is null\n";
    return;
  }

  Function *GoRtFunc = nullptr;
  for (auto &BB : *MainFunc) {
    for (auto &I : BB) {
      if (auto *callInst = dyn_cast<CallInst>(&I)) {
        if (callInst->getNumOperands() < 1) {
          errs() << "renameRealCallee Warning: CallInst has fewer than 1 "
                    "argument\n";
          continue;
        }
        if (auto *RealMainFunc =
                dyn_cast<Function>(callInst->getArgOperand(0))) {
          GoRtFunc = callInst->getCalledFunction();
          if (RealMainFunc) {
            RealMainFunc->setName(NewCalleeName);
            outs() << "Renamed callee function to: " << NewCalleeName << "\n";
          }
        } else {
          errs() << "renameRealCallee Warning: First argument is not a "
                    "Function*\n";
        }
      }
    }
  }
}
