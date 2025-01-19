//===-------- MergeGoCFunc.cpp - Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MergeGoCFunc.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "merge-go-c-func"

static cl::opt<bool> RenameCallerGc("rename-caller-gc", cl::init(false),
                                    cl::desc("Rename the Go caller functions"));

static cl::opt<bool> RenameCalleeGc("rename-callee-gc", cl::init(false),
                                    cl::desc("Rename the Go callee functions"));

static cl::opt<bool>
    MergeCalleeGc("merge-callee-gc", cl::init(false),
                  cl::desc("Merge the given callee functions"));

static cl::opt<bool>
    ReplaceMakeRpc("replace-make-rpc", cl::init(false),
                   cl::desc("Replace the given callee functions"));

static cl::opt<bool> ReplaceDummybyCallee(
    "replace-dummy", cl::init(false),
    cl::desc("Replace the given dummy functions by main_callee"));

static cl::opt<std::string> CallerNameGc("caller-name-gc", cl::Hidden,
                                         cl::desc("Caller function name"),
                                         cl::init(""));

static cl::opt<std::string> CalleeNameGc("callee-name-gc", cl::Hidden,
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
  } else if (ReplaceMakeRpc) {
    replaceMakeRpcCall(&M);
    Changed = true;
  } else if (ReplaceDummybyCallee) {
    replaceDummy(&M);
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

  FunctionType *NewFuncType =
      FunctionType::get(Type::getInt8PtrTy(Context), ParamTypes, false);

  std::string NewFuncName = MainFunc->getName().str() + "_callee";
  GlobalValue::LinkageTypes Linkage = MainFunc->getLinkage();

  Function *newCalleeFunc =
      Function::Create(NewFuncType, Linkage, NewFuncName, M);

  Argument *arg1 = newCalleeFunc->arg_begin();
  arg1->setName("arg1");

  newCalleeFunc->copyAttributesFrom(MainFunc);

  ValueToValueMapTy VMap;

  SmallVector<ReturnInst *, 8> Returns;
  CloneFunctionInto(newCalleeFunc, MainFunc, VMap,
                    CloneFunctionChangeType::LocalChangesOnly, Returns);

  Instruction *callGetArgInst = nullptr;
  Instruction *storeCallToInputInst = nullptr;
  Instruction *callSendReturnInst = nullptr;
  ReturnInst *returnInst = nullptr;
  Value *arraydecay1 = nullptr;
  AllocaInst *inputAlloca = nullptr;
  AllocaInst *randomStringAlloca = nullptr;

  for (auto &I : newCalleeFunc->getEntryBlock()) {
    if (AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {
      Type *allocaType = AI->getAllocatedType();
      if (allocaType == Type::getInt8PtrTy(Context)) {
        inputAlloca = AI;
      } else if (ArrayType *arrType = dyn_cast<ArrayType>(allocaType)) {
        if (arrType->getElementType() == Type::getInt8Ty(Context) &&
            arrType->getNumElements() == 17) {
          randomStringAlloca = AI;
        }
      }
    }
    if (inputAlloca && randomStringAlloca)
      break;
  }

  if (!inputAlloca || !randomStringAlloca) {
    errs() << "Failed to find expected allocas\n";
    return;
  }

  for (auto &BB : *newCalleeFunc) {
    for (auto InstIter = BB.begin(), E = BB.end(); InstIter != E;) {
      Instruction *I = &*InstIter++;
      if (CallInst *CI = dyn_cast<CallInst>(I)) {
        if (Function *Callee = CI->getCalledFunction()) {
          StringRef CalleeName = Callee->getName();
          if (CalleeName == "_Z19get_arg_from_callerv") {
            callGetArgInst = CI;
          } else if (CalleeName == "_Z27send_return_value_to_callerPc") {
            CI->eraseFromParent();
            continue;
          }
        }
      } else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
        if (SI->getPointerOperand() == inputAlloca) {
          if (SI->getValueOperand() == callGetArgInst) {
            storeCallToInputInst = SI;
          }
        }
      } else if (ReturnInst *RI = dyn_cast<ReturnInst>(I)) {
        returnInst = RI;
      } else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(I)) {
        if (GEP->getPointerOperand() == randomStringAlloca &&
            GEP->getNumIndices() == 2) {
          arraydecay1 = GEP;
        }
      }
    }
  }

  if (callGetArgInst) {
    callGetArgInst->eraseFromParent();
  }

  if (storeCallToInputInst) {
    storeCallToInputInst->eraseFromParent();
  }

  IRBuilder<> Builder(inputAlloca->getNextNode());
  Builder.CreateStore(arg1, inputAlloca);

  if (returnInst && arraydecay1) {
    IRBuilder<> RetBuilder(returnInst);
    RetBuilder.CreateRet(arraydecay1);
    returnInst->eraseFromParent();
  } else {
    errs() << "Failed to find return instruction or arraydecay1\n";
  }

  errs() << "Function '" << MainFunc->getName() << "' cloned to '"
         << newCalleeFunc->getName() << "' with modifications.\n";

  MainFunc->eraseFromParent();
  return;
}

CallInst *MergeGoCFuncPass::getCallInstByCalledFunc(Function *callerFunc,
                                                    Function *calledFunc) {
  for (Function::iterator BBB = callerFunc->begin(), BBE = callerFunc->end();
       BBB != BBE; ++BBB) {
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE;
         IB++) {
      if (isa<CallInst>(IB)) {
        CallInst *ci = dyn_cast<CallInst>(IB);
        Function *func = ci->getCalledFunction();
        if (func == calledFunc)
          return ci;
      }
    }
  }
  return NULL;
}

void MergeGoCFuncPass::replaceMakeRpcCall(Module *M) {
  Function *makeRpc = M->getFunction("main.make__rpc");
  if (makeRpc) {
    errs() << "Function " << makeRpc->getName() << " found. \n";
  } else {
    errs() << "Function 'main.make__rpc' not found!\n";
    return;
  }

  Function *wrapperGoToC = M->getFunction("main.wrapper__go2c");
  if (wrapperGoToC) {
    errs() << "Function " << wrapperGoToC->getName() << " found. \n";
  } else {
    errs() << "Function 'main.make__rpc' not found!\n";
    return;
  }

  Function *callerFunc = M->getFunction("main.main");
  CallInst *rpcInst = getCallInstByCalledFunc(callerFunc, makeRpc);
  std::vector<Value *> arguments;
  errs() << "NumOperands: " << rpcInst->getNumOperands() << "\n";
  for (unsigned i = 0; i < rpcInst->getNumOperands(); i++) {
    if (i == 0 || i == 3 || i == 4) {
      Value *arg = rpcInst->getOperand(i);
      arguments.push_back(arg);
    }
  }

  CallInst *newCall =
      CallInst::Create(wrapperGoToC->getFunctionType(), wrapperGoToC, arguments,
                       "pointer2dummy", rpcInst);
  newCall->setDebugLoc(rpcInst->getDebugLoc());
  std::vector<llvm::User *> users;
  for (const llvm::Use &use : rpcInst->uses()) {
    llvm::User *user = use.getUser();
    users.push_back(user);
  }
  for (auto user : users) {
    for (auto oi = user->op_begin(); oi != user->op_end(); oi++) {
      Value *val = *oi;
      Value *call_value = dyn_cast<Value>(rpcInst);
      if (val == call_value) {
        *oi = dyn_cast<Value>(newCall);
      }
    }
  }
  rpcInst->eraseFromParent();
  return;
}

void MergeGoCFuncPass::replaceDummy(Module *M) {
  Function *dummyFun = M->getFunction("main.dummy");
  if (dummyFun) {
    errs() << "Function " << dummyFun->getName() << " found. \n";
  } else {
    errs() << "Function 'main.dummy' not found!\n";
    return;
  }

  Function *mainCallee = M->getFunction("main_callee");
  if (mainCallee) {
    errs() << "Function " << mainCallee->getName() << " found. \n";
  } else {
    errs() << "Function 'main_callee' not found!\n";
    return;
  }

  Function *callerFunc = M->getFunction("main.wrapper__go2c");
  if (callerFunc) {
    errs() << "Function " << callerFunc->getName() << " found. \n";
  } else {
    errs() << "Call Function 'main.wrapper__go2c' not found!\n";
    return;
  }

  CallInst *callDummy = getCallInstByCalledFunc(callerFunc, dummyFun);
  std::vector<Value *> arguments;
  errs() << "NumOperands: " << callDummy->getNumOperands() << "\n";
  for (unsigned i = 0; i < callDummy->getNumOperands(); i++) {
    if (i == 1) {
      Value *arg = callDummy->getOperand(i);
      arguments.push_back(arg);
    }
  }

  CallInst *newCall =
      CallInst::Create(mainCallee->getFunctionType(), mainCallee, arguments,
                       "dummy2C", callDummy);
  newCall->setDebugLoc(callDummy->getDebugLoc());
  std::vector<llvm::User *> users;
  for (const llvm::Use &use : callDummy->uses()) {
    llvm::User *user = use.getUser();
    users.push_back(user);
  }
  for (auto user : users) {
    for (auto oi = user->op_begin(); oi != user->op_end(); oi++) {
      Value *val = *oi;
      Value *call_value = dyn_cast<Value>(callDummy);
      if (val == call_value) {
        *oi = dyn_cast<Value>(newCall);
      }
    }
  }
  callDummy->eraseFromParent();

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
