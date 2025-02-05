//===-------- MergeCFuncGo.cpp - Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/MergeCFuncGo.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "merge-c-func-go"

static cl::opt<bool> ChangeLinkType_cs("change-link-type-cg", cl::init(false),
                                       cl::desc("change functions link type"));

static cl::opt<bool>
    MergeCallee_cs("merge-callee-cg", cl::init(false),
                   cl::desc("merge the given callee functions"));

PreservedAnalyses MergeCFuncGoPass::run(Module &M, ModuleAnalysisManager &AM) {
  bool Changed = false;
  if (ChangeLinkType_cs) {
    ChangeLinkType(&M);
    Changed = true;
  } else if (MergeCallee_cs) {
    MergeCallee(&M);
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

Function *MergeCFuncGoPass::getCFunctionByDemangledName(Module *M,
                                                        std::string fname) {
  for (Module::iterator f = M->begin(); f != M->end(); f++) {
    Function *func = dyn_cast<Function>(f);
    std::string funcName = func->getName().str();
    std::string demangledName = llvm::demangle(funcName);
    if (demangledName == fname) {
      return func;
    }
  }
  return NULL;
}

CallInst *MergeCFuncGoPass::getCallInstByCalledFunc(Function *callerFunc,
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

CallInst *MergeCFuncGoPass::createCallWrapper(CallInst *rpcInst,
                                              Function *wrapperFunc) {
  std::vector<Value *> arguments;
  std::vector<Type *> argumentTypes;
  for (unsigned i = 0; i < rpcInst->getNumOperands(); i++) {
    Value *arg = rpcInst->getOperand(i);
    if (i == 1 || i == 0) {
      arguments.push_back(arg);
      argumentTypes.push_back(arg->getType());
    }
  }

  CallInst *newCall =
      CallInst::Create(wrapperFunc->getFunctionType(), wrapperFunc, arguments,
                       "pointer2dummy", rpcInst);

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

  return newCall;
}

Function *MergeCFuncGoPass::createNewCalleeFunc(Function *calleeFunc,
                                                CallInst *dummyCall) {
  Module *M = calleeFunc->getParent();
  LLVMContext &Context = M->getContext();
  // copy this function
  std::vector<Value *> arguments;
  std::vector<Type *> argumentTypes;

  // the last argument of a CallInst is the called function
  // so we don't need to include it in the arguments.
  for (unsigned i = 0; i < dummyCall->getNumOperands() - 1; i++) {
    Value *arg = dummyCall->getOperand(i);
    arguments.push_back(arg);
    argumentTypes.push_back(arg->getType());
  }

  Function *dummyFunc = dummyCall->getCalledFunction();
  FunctionType *dummyFuncType = dummyFunc->getFunctionType();
  Function *newFunc = Function::Create(dummyFuncType, calleeFunc->getLinkage(),
                                       calleeFunc->getName() + ".cloned", M);

  newFunc->copyAttributesFrom(calleeFunc);
  ValueToValueMapTy VMap;
  SmallVector<ReturnInst*, 8> Returns;

  Function::arg_iterator DestI = newFunc->arg_begin();
  for (const Argument &J : calleeFunc->args()) {
    DestI->setName(J.getName());
    VMap[&J] = &*DestI++;
  }

  CloneFunctionInto(newFunc, calleeFunc, VMap, llvm::CloneFunctionChangeType::LocalChangesOnly, Returns);

  CallInst *getArgCall = nullptr;
  for (BasicBlock &BB : *newFunc) {
    for (Instruction &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        Function *calledFunc = CI->getCalledFunction();
        if (calledFunc &&
            calledFunc->getName() == "main.get__arg__from__caller") {
          getArgCall = CI;
          errs() << "getArgCall is found.\n";
          break;
        }
      }
    }
    if (getArgCall)
      break;
  }

  if (!getArgCall) {
    errs() << "Error: Call to 'main.get__arg__from__caller' not found in "
              "function.\n";
    return nullptr;
  }

  Function::arg_iterator argIt = newFunc->arg_begin();
  ++argIt;
  if (argIt == newFunc->arg_end()) {  
    errs() << "Error: Not enough arguments in new function.\n";  
    return nullptr;  
  }

  Argument* arg1 = &*argIt;
  arg1->setName("arg_input_ptr");  
  ++argIt;
  if (argIt == newFunc->arg_end()) {  
    errs() << "Error: Not enough arguments in new function.\n";  
    return nullptr;  
  } 

  Argument* arg2 = &*argIt;
  arg2->setName("arg_input_len");
  Type* retNewType = getArgCall->getType(); // { i8*, i64 }  
  Value* newArg = UndefValue::get(retNewType);

  IRBuilder<> Builder(getArgCall);
  Value* newInsertValue1 = Builder.CreateInsertValue(newArg, arg1, {0}, "new_arg1");
  Value *newInsertValue2 =
      Builder.CreateInsertValue(newInsertValue1, arg2, {1}, "new_arg2");

  getArgCall->replaceAllUsesWith(newInsertValue2);
  getArgCall->eraseFromParent();

  // change how the new callee function returns
  ReturnInst *ret = nullptr;
  for (Function::iterator BBB = newFunc->begin(), BBE = newFunc->end();
       BBB != BBE; ++BBB) {
    for (BasicBlock::iterator IB = BBB->begin(), IE = BBB->end(); IB != IE;
         IB++) {
      if (isa<ReturnInst>(IB)) {
        ret = dyn_cast<ReturnInst>(IB);
      }
    }
  }

  if (!ret) {
    errs() << "Function ret not found!\n";
  }

  // create a new return value
  CallInst *sendReturnCall = nullptr;
  for (BasicBlock &BB : *newFunc) {
    for (Instruction &Inst : BB) {
      if (CallInst *CI = dyn_cast<CallInst>(&Inst)) {
        Function *calledFunc = CI->getCalledFunction();
        if (calledFunc &&
            calledFunc->getName() == "main.send__return__value__to__caller") {
          sendReturnCall = CI;
          errs() << "sendReturnCall is found.\n";
          break;
        }
      }
    }
    if (sendReturnCall)
      break;
  }
  if (!sendReturnCall) {
    errs() << "Error: Call to 'main.send__return__value__to__caller' not found "
              "in function.\n";
    return nullptr;
  }

  Value* sendReturnCallArg1 = sendReturnCall->getArgOperand(1);
  Value* sendReturnCallArg2 = sendReturnCall->getArgOperand(2);
  Type* retType = newFunc->getReturnType();
  Value* newRetVal = UndefValue::get(retType);

  IRBuilder<> BuilderReturn(sendReturnCall);
  newRetVal = BuilderReturn.CreateInsertValue(newRetVal, sendReturnCallArg1, {0}, "new_ret0");
  newRetVal = BuilderReturn.CreateInsertValue(newRetVal, sendReturnCallArg2, {1}, "new_ret1");

  if (sendReturnCall && newRetVal) {
    sendReturnCall->eraseFromParent();
    for (ReturnInst *ret : Returns) {
      IRBuilder<> builder(ret);
      ReturnInst::Create(newFunc->getContext(), newRetVal, ret);
      ret->eraseFromParent();
    }
  } else {
    errs() << "Failed to find send_return_value_to_caller() or its argument.\n";
  }

  return newFunc;
}

void MergeCFuncGoPass::ChangeLinkType(Module *M) {
  Function *mainFunc = M->getFunction("main.function");
  if (!mainFunc) {
    errs() << "Function 'main.function' not found!\n";
    return;
  }
  mainFunc->setLinkage(llvm::GlobalValue::ExternalLinkage);

  Function *wrapperGoToC = M->getFunction("main.wrapper__c2go");
  if (!wrapperGoToC) {
    errs() << "Function 'main.wrapper__go2c' not found!\n";
    return;
  }
  wrapperGoToC->setLinkage(llvm::GlobalValue::ExternalLinkage);
}

void MergeCFuncGoPass::MergeCallee(Module *M) {
  Function *callerFunc = M->getFunction("main");
  if (!callerFunc) {
    errs() << "Function 'main' not found!\n";
    return;
  }

  Function *rpcFunc =
      getCFunctionByDemangledName(M, "make_rpc(char const*, char*)");
  if (!rpcFunc) {
    errs() << "Function 'make_rpc' not found!\n";
    return;
  }

  Function *wrapperCToGo = M->getFunction("main.wrapper__c2go");
  if (!wrapperCToGo) {
    errs() << "Function 'main.wrapper__go2c' not found!\n";
    return;
  }

  if ((!callerFunc) || (!rpcFunc) || (!wrapperCToGo)) {
    errs() << "cannot find caller function or make_rpc or wrapper function\n";
    return;
  }

  CallInst *rpcInst = getCallInstByCalledFunc(callerFunc, rpcFunc);
  if (!rpcInst) {
    llvm::errs() << "caller doesn't contain make_rpc call\n";
    return;
  }

  CallInst *callWrapper = createCallWrapper(rpcInst, wrapperCToGo);
  if (!callWrapper) {
    llvm::errs() << "fail to create a call to wrapper\n";
  }

  Function *calleeFunc = M->getFunction("main.function");
  if (!calleeFunc) {
    errs() << "Function callee function not found!\n";
    return;
  }

  Function *dummyFunc = M->getFunction("main.dummy");
  if (!dummyFunc) {
    errs() << "Function callee dummy not found!\n";
    return;
  }

  CallInst *dummyCall = getCallInstByCalledFunc(wrapperCToGo, dummyFunc);
  if (!dummyCall) {
    errs() << "dummy call doesn't exist\n";
  }

  Function *newCalleeFunc = createNewCalleeFunc(calleeFunc, dummyCall);
}
