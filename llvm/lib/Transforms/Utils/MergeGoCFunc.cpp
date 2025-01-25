//===-------- MergeGoCFunc.cpp - Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MergeGoCFunc.h"
#include "llvm/ADT/SmallVector.h"
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

  FunctionType *NewFuncType = FunctionType::get(
      Type::getInt8PtrTy(Context), {Type::getInt8PtrTy(Context)}, false);

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

  // change how the new callee function returns
  std::vector<ReturnInst *> RetInsts;
  for (Function::iterator BBB = newCalleeFunc->begin(),
                          BBE = newCalleeFunc->end();
       BBB != BBE; ++BBB) {
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE;
         ++IB) {
      if (ReturnInst *ret = dyn_cast<ReturnInst>(&*IB)) {
        RetInsts.push_back(ret);
      }
    }
  }

  CallInst *sendReturnCall = nullptr;
  Value *newRetVal = nullptr;
  for (Function::iterator BBB = newCalleeFunc->begin(),
                          BBE = newCalleeFunc->end();
       BBB != BBE; ++BBB) {
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE;
         ++IB) {
      if (CallInst *ci = dyn_cast<CallInst>(&*IB)) {
        Function *CalledFunction = ci->getCalledFunction();
        if (CalledFunction &&
            CalledFunction->getName() == "_Z27send_return_value_to_callerPc") {
          sendReturnCall = ci;
          if (ci->arg_size() > 0) {
            newRetVal = ci->getArgOperand(0);
          } else {
            errs() << "send_return_value_to_caller() has no arguments.\n";
          }
          break;
        }
      }
    }
    if (sendReturnCall)
      break;
  }

  if (sendReturnCall && newRetVal) {
    sendReturnCall->eraseFromParent();
    for (ReturnInst *ret : RetInsts) {
      IRBuilder<> builder(ret);
      ReturnInst::Create(newCalleeFunc->getContext(), newRetVal, ret);
      ret->eraseFromParent();
    }
  } else {
    errs() << "Failed to find send_return_value_to_caller() or its argument.\n";
  }

  // change how the new callee function get arguments
  CallInst *getArgCall = nullptr;
  for (Function::iterator BBB = newCalleeFunc->begin(),
                          BBE = newCalleeFunc->end();
       BBB != BBE; ++BBB) {
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE;
         ++IB) {
      if (CallInst *ci = dyn_cast<CallInst>(&*IB)) {
        Function *CalledFunction = ci->getCalledFunction();
        if (CalledFunction) {
          StringRef CalleeName = CalledFunction->getName();
          // errs() << "Called function name is: " << CalleeName << "\n";
          if (CalleeName == "_Z19get_arg_from_callerv") {
            getArgCall = ci;
            break;
          }
        }
      }
    }
    if (getArgCall)
      break;
  }

  if (getArgCall) {
    unsigned numArgs = sendReturnCall->arg_size();
    if (numArgs > 0) {
      Value *arg = sendReturnCall->getArgOperand(0);
    } else {
      errs() << "Not enough arguments in get_arg_from_caller().\n";
    }
  } else
    errs() << "get_arg_from_caller() is not found.\n";

  AllocaInst *allocFuncArg = new AllocaInst(newCalleeFunc->getArg(0)->getType(),
                                            0, NULL, "", getArgCall);
  StoreInst *storeInst1 =
      new StoreInst(newCalleeFunc->getArg(0), allocFuncArg, getArgCall);
  LoadInst *loadInst1 = new LoadInst(newCalleeFunc->getArg(0)->getType(),
                                     allocFuncArg, "", getArgCall);

  SmallVector<llvm::User *> users;
  for (const llvm::Use &use : getArgCall->uses()) {
    llvm::User *user = use.getUser();
    users.push_back(user);
  }
  for (auto user : users) {
    for (auto oi = user->op_begin(); oi != user->op_end(); oi++) {
      Value *val = *oi;
      Value *call_value = dyn_cast<Value>(getArgCall);
      if (val == call_value) {
        *oi = dyn_cast<Value>(loadInst1);
      }
    }
  }

  getArgCall->eraseFromParent();

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
  if (!makeRpc) {
    errs() << "Function 'main.make__rpc' not found!\n";
    return;
  }

  Function *wrapperGoToC = M->getFunction("main.wrapper__go2c");
  if (!wrapperGoToC) {
    errs() << "Function 'main.wrapper__go2c' not found!\n";
    return;
  }

  Function *callerFunc = M->getFunction("main.main");
  CallInst *rpcInst = getCallInstByCalledFunc(callerFunc, makeRpc);
  std::vector<Value *> arguments;
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
  if (!dummyFun) {
    errs() << "Function 'main.dummy' not found!\n";
    return;
  }

  Function *mainCallee = M->getFunction("main_callee");
  if (!mainCallee) {
    errs() << "Function 'main_callee' not found!\n";
    return;
  }

  Function *callerFunc = M->getFunction("main.wrapper__go2c");
  if (!callerFunc) {
    errs() << "Call Function 'main.wrapper__go2c' not found!\n";
    return;
  }

  CallInst *callDummy = getCallInstByCalledFunc(callerFunc, dummyFun);
  std::vector<Value *> arguments;
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
