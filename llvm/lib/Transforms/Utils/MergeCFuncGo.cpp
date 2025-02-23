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
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include <functional>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "merge-c-func-go"

static cl::opt<bool> ChangeLinkType_cg("change-link-type-cg", cl::init(false),
                                       cl::desc("change functions link type"));
static cl::opt<bool> RenameCallee_cg("rename-callee-cg", cl::init(false),
                                     cl::desc("rename the wrapper functions"));

static cl::opt<bool> RenameWrapper_cg("rename-wrapper-cg", cl::init(false),
                                      cl::desc("rename the wrapper functions"));

static cl::opt<bool>
    MergeCallee_cs("merge-callee-cg", cl::init(false),
                   cl::desc("merge the given callee functions"));

static cl::opt<bool> CreateNewMemory_cs("create-memory-cg", cl::init(false),
                                        cl::desc("create a new memory"));

PreservedAnalyses MergeCFuncGoPass::run(Module &M, ModuleAnalysisManager &AM) {
  bool Changed = false;
  if (ChangeLinkType_cg) {
    ChangeLinkType(&M);
    Changed = true;
  } else if (MergeCallee_cs) {
    MergeCallee(&M);
    Changed = true;
  } else if (RenameCallee_cg) {
    RenameCallee(&M);
    Changed = true;
  } else if (RenameWrapper_cg) {
    RenameWrapper(&M);
    Changed = true;
  } else if (CreateNewMemory_cs) {
    CreateNewMemory(&M);
    Changed = true;
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

void MergeCFuncGoPass::RenameCallee(Module *M) {
  Function *mainFunc = M->getFunction("main.main");
  if (mainFunc) {
    mainFunc->setName("main.callee");
  }

  // std::string fileName = M->getModuleIdentifier();
  // size_t hash = std::hash<std::string>{}(fileName);

  // for (Function &F : *M) {
  //   if ((F.getName() != "main.callee") &&
  //       (F.getName() != "main.get__arg__from__caller") &&
  //       (F.getName() != "main.send__return__value__to__caller") &&
  //       (F.getName() != "main.function")) {
  //     bool isIntrinsic = F.getName().startswith("llvm.");

  //     if (!isIntrinsic) {
  //       // std::string newName =
  //       //     "func_" + std::to_string(hash) + "_" + F.getName().str();
  //       // F.setName(newName);
  //       // errs() << "Renaming function " << F.getName() << " to " << newName
  //       //        << "\n";
  //     }
  //   }
  // }

  // // for (GlobalVariable &GV : M->globals()) {
  // //   std::string newName =
  // //       "global_" + std::to_string(hash) + "_" + GV.getName().str();

  // //   if (!GV.hasAttribute(Attribute::ImmArg)) {
  // //     GV.setName(newName);
  // //     // errs() << "Renaming global variable " << GV.getName() << " to "
  // <<
  // //     // newName
  // //     //        << "\n";
  // //   }
  // // }

  return;
}

void MergeCFuncGoPass::CreateNewMemory(Module *M) {
  Function *mainClone = M->getFunction("main.function");
  if (!mainClone) {
    errs() << "Function 'main.function' not found!\n";
    return;
  }
  if (mainClone) {
    for (auto &B : *mainClone) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (isa<CallInst>(I)) {
          CallInst *callInst = cast<CallInst>(&I);
          if (callInst->getCalledFunction() &&
              (callInst->getCalledFunction()->getIntrinsicID() ==
                   Intrinsic::lifetime_end ||
               callInst->getCalledFunction()->getIntrinsicID() ==
                   Intrinsic::lifetime_start)) {
            // I.eraseFromParent();
          }
        }
      }
    }
  }
  if (mainClone) {
    for (auto &B : *mainClone) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          Function *calledFunc = CI->getCalledFunction();
          if (calledFunc && calledFunc->getName() == "runtime.concatstrings") {
            insertBufferForConcat(CI);
          }
        }
      }
    }
  }

  Function *dummyFunc = M->getFunction("main.dummy");
  if (!mainClone) {
    errs() << "Function 'main.dummy' not found!\n";
    return;
  }
  if (dummyFunc) {
    for (auto &B : *dummyFunc) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          Function *calledFunc = CI->getCalledFunction();
          if (calledFunc && calledFunc->getName() == "runtime.concatstrings") {
            insertBufferForConcat(CI);
          }
        }
      }
    }
  }

  return;
}

void MergeCFuncGoPass::RenameWrapper(Module *M) {
  Function *mainFunc = M->getFunction("main.main");
  if (mainFunc) {
    mainFunc->setName("main.wrapper");
  }

  std::string fileName = M->getModuleIdentifier();
  size_t hash = std::hash<std::string>{}(fileName);

  for (Function &F : *M) {
    if ((F.getName() != "main.wrapper__c2go") &&
        (F.getName() != "main.wrapper") && (F.getName() != "main.dummy")) {
      bool isIntrinsic = F.getName().startswith("llvm.");

      if (!isIntrinsic) {
        std::string newName =
            "func_" + std::to_string(hash) + "_" + F.getName().str();
        F.setName(newName);
        // errs() << "Renaming function " << F.getName() << " to " << newName
        //        << "\n";
      }
    }
  }

  for (GlobalVariable &GV : M->globals()) {
    std::string newName =
        "global_" + std::to_string(hash) + "_" + GV.getName().str();

    if (!GV.hasAttribute(Attribute::ImmArg)) {
      GV.setName(newName);
      // errs() << "Renaming global variable " << GV.getName() << " to " <<
      // newName
      //        << "\n";
    }
  }

  return;
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
  Module *M = rpcInst->getModule();

  // Function *getGoContextFunc = M->getFunction("main.GetGoContext");
  // if (!getGoContextFunc) {
  //   errs() << "Error: Cannot find main.GetGoContext function!\n";
  //   return nullptr;
  // }

  // Function *customInitFunc = M->getFunction("main.custom__init");
  // if (!customInitFunc) {
  //   errs() << "Error: Cannot find main.custom__init function!\n";
  //   return nullptr;
  // }
  // IRBuilder<> Builder(rpcInst);
  // Value *undef = UndefValue::get(Type::getInt8PtrTy(rpcInst->getContext()));
  // Value *customInitCall = Builder.CreateCall(customInitFunc, {undef},
  // "custom_init_call"); Builder.SetInsertPoint(rpcInst);
  // Builder.Insert(customInitCall);

  // Function *printfFunc = M->getFunction("printf");
  // if (!printfFunc) {
  //   std::vector<Type *> printfArgs(1,
  //                                  Type::getInt8PtrTy(rpcInst->getContext()));
  //   FunctionType *printfType = FunctionType::get(
  //       Type::getInt32Ty(rpcInst->getContext()), printfArgs, true);
  //   printfFunc =
  //       Function::Create(printfType, Function::ExternalLinkage, "printf", M);
  // }
  // IRBuilder<> Builder(rpcInst);
  // Value *strFormat = Builder.CreateGlobalStringPtr("%p\n");
  // Builder.CreateCall(printfFunc, {strFormat, rpcInst->getOperand(1)});

  for (unsigned i = 0; i < rpcInst->getNumOperands(); i++) {
    Value *arg = rpcInst->getOperand(i);
    if (i == 0) {
      Value *undef = UndefValue::get(Type::getInt8PtrTy(rpcInst->getContext()));
      arguments.push_back(undef);
    } else if (i == 1) {
      arguments.push_back(arg);
    }
  }

  // IRBuilder<> Builder(rpcInst);
  // Value *nullNest = Constant::getNullValue(Builder.getInt8PtrTy());
  // Value *go_ctx = Builder.CreateCall(getGoContextFunc, {nullNest}, "go_ctx");

  // for (unsigned i = 0; i < rpcInst->getNumOperands(); i++) {
  //   if (i == 0) {
  //     arguments.push_back(go_ctx);
  //   } else if (i == 1) {
  //     Value *arg = rpcInst->getOperand(i);
  //     arguments.push_back(arg);
  //   }
  // }

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

void MergeCFuncGoPass::replaceCallsToClonedFunction(Module *M, Function *originalFunc, Function *clonedFunc) {
  for (auto &F : *M) {
      for (auto &B : F) {
          for (auto &I : B) {
              if (auto *CI = dyn_cast<CallInst>(&I)) {
                  Function *calledFunc = CI->getCalledFunction();
                  if (calledFunc && calledFunc == originalFunc) {
                      CI->setCalledFunction(clonedFunc);
                  }
              }
          }
      }
  }
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
                                       "calleeFromMain", M);

  //newFunc->copyAttributesFrom(calleeFunc);
  ValueToValueMapTy VMap;
  SmallVector<ReturnInst *, 8> Returns;

  Function::arg_iterator DestI = newFunc->arg_begin();
  for (const Argument &J : calleeFunc->args()) {
    DestI->setName(J.getName());
    VMap[&J] = &*DestI++;
  }

  CloneFunctionInto(newFunc, calleeFunc, VMap,
                    llvm::CloneFunctionChangeType::LocalChangesOnly, Returns);

  newFunc->setLinkage(GlobalValue::ExternalLinkage);
  newFunc->setVisibility(GlobalValue::DefaultVisibility);
  newFunc->setDLLStorageClass(GlobalValue::DefaultStorageClass);

  replaceCallsToClonedFunction(M, calleeFunc, newFunc);

  for (User *U : calleeFunc->users()) {
    if (CallInst *callInst = dyn_cast<CallInst>(U)) {
      callInst->setCalledFunction(newFunc);
    }
  }

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

  Argument *arg1 = &*argIt;
  arg1->setName("arg_input_ptr");
  ++argIt;
  if (argIt == newFunc->arg_end()) {
    errs() << "Error: Not enough arguments in new function.\n";
    return nullptr;
  }

  Argument *arg2 = &*argIt;
  arg2->setName("arg_input_len");
  Type *retNewType = getArgCall->getType(); // { i8*, i64 }
  Value *newArg = UndefValue::get(retNewType);

  IRBuilder<> Builder(getArgCall);
  Value *newInsertValue1 =
      Builder.CreateInsertValue(newArg, arg1, {0}, "new_arg1");
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

  Value *sendReturnCallArg1 = sendReturnCall->getArgOperand(1);
  Value *sendReturnCallArg2 = sendReturnCall->getArgOperand(2);
  Type *retType = newFunc->getReturnType();
  Value *newRetVal = UndefValue::get(retType);

  IRBuilder<> BuilderReturn(sendReturnCall);
  newRetVal = BuilderReturn.CreateInsertValue(newRetVal, sendReturnCallArg1,
                                              {0}, "new_ret0");
  newRetVal = BuilderReturn.CreateInsertValue(newRetVal, sendReturnCallArg2,
                                              {1}, "new_ret1");

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

  calleeFunc->eraseFromParent();
  return newFunc;
}

void MergeCFuncGoPass::ChangeLinkType(Module *M) {
  Function *mainFunc = M->getFunction("main.function");
  if (!mainFunc) {
    errs() << "Function 'main.function' not found!\n";
    //return;
  }
  else
    mainFunc->setLinkage(llvm::GlobalValue::ExternalLinkage);

  Function *wrapperGoToC = M->getFunction("main.wrapper__c2go");
  if (!wrapperGoToC) {
    errs() << "Function main.wrapper__go2c' not found!\n";
    //return;
  }
  else
    wrapperGoToC->setLinkage(llvm::GlobalValue::ExternalLinkage);

  Function *calleeFromMain = M->getFunction("calleeFromMain");
  if (!calleeFromMain) {
    errs() << "Function calleeFromMain not found!\n";
    //return;
  }
  else
    calleeFromMain->setLinkage(llvm::GlobalValue::InternalLinkage);
  // Function *getGoContext = M->getFunction("main.GetGoContext");
  // if (!getGoContext) {
  //   errs() << "Function main.GetGoContext not found!\n";
  //   return;
  // }
  // getGoContext->setLinkage(llvm::GlobalValue::ExternalLinkage);
}

void MergeCFuncGoPass::createCall2NewCallee(CallInst *dummyCall,
                                            Function *newCalleeFunc) {
  std::vector<Value *> arguments;
  std::vector<Type *> argumentTypes;
  Value *nestArg = dummyCall->getArgOperand(0);
  arguments.push_back(nestArg);

  for (unsigned i = 1; i < dummyCall->getNumOperands() - 1; ++i) {
    Value *arg = dummyCall->getArgOperand(i);
    arguments.push_back(arg);
    errs() << "Parameter " << i << ": ";
    if (arg->hasName()) {
      errs() << arg->getName();
    } else {
      if (Constant *C = dyn_cast<Constant>(arg)) {
        errs() << "Constant ";
        C->print(errs(), true);
      } else {
        errs() << "Value ";
        arg->print(errs(), true);
      }
    }
    errs() << "\n";
  }

  CallInst *newCall =
      CallInst::Create(newCalleeFunc, arguments, "new_callee_ret", dummyCall);

  newCall->setCallingConv(dummyCall->getCallingConv());
  newCall->setTailCallKind(dummyCall->getTailCallKind());
  // newCall->setAttributes(dummyCall->getAttributes());

  if (MDNode *Dbg = dummyCall->getMetadata("dbg")) {
    newCall->setMetadata("dbg", Dbg);
  }

  dummyCall->replaceAllUsesWith(newCall);

  dummyCall->eraseFromParent();
}

void MergeCFuncGoPass::insertBufferForConcat(CallInst *CI) {
  IRBuilder<> Builder(CI);
  Value *arg1 = CI->getArgOperand(1);
  Value *arg2 = CI->getArgOperand(2);

  Type *i8Type = Type::getInt8Ty(CI->getContext());
  Type *i8PtrType = Type::getInt8PtrTy(CI->getContext());

  // Find or create malloc function
  Function *mallocFunc = CI->getModule()->getFunction("malloc");
  if (!mallocFunc) {
    errs() << "Malloc function not found in the module!\n";
    return;
  }

  // Create malloc call to allocate memory for buffer (64 bytes)
  Value *mallocArg = ConstantInt::get(CI->getContext(), APInt(64, 64));
  Value *bufferPtr = Builder.CreateCall(mallocFunc, mallocArg, CI->getName() + "malloc_buffer");

  // Find printf function in the module
  // Function *printfFunc = CI->getModule()->getFunction("printf");
  // if (!printfFunc) {
  //   errs() << "Printf function not found in the module!\n";
  //   return;
  // }

  // Create format string for printf
  // Constant *formatStr = Builder.CreateGlobalStringPtr("Buffer address: %p\n");

  // Create the printf call to print the buffer address
  // Builder.CreateCall(printfFunc, {formatStr, bufferPtr});

  // Replace the original argument with the new allocated buffer
  CI->setArgOperand(1, bufferPtr);

  return;
}

void MergeCFuncGoPass::insertBufferForConcatForSlicebyte(CallInst *CI) {
  IRBuilder<> Builder(CI);

  Value *arg2 = CI->getArgOperand(2);

  Type *i8Type = Type::getInt8Ty(CI->getContext());
  Type *i8PtrType = Type::getInt8PtrTy(CI->getContext());

  Function *mallocFunc = CI->getModule()->getFunction("malloc");

  if (!mallocFunc) {
    errs() << "Malloc function not found in the module!\n";
    return;
  }

  Value *mallocArg = ConstantInt::get(CI->getContext(), APInt(64, 64));

  Value *bufferPtr = Builder.CreateCall(mallocFunc, mallocArg, "malloc_buffer");

  if (isa<ConstantPointerNull>(arg2)) {
    CI->setArgOperand(2, bufferPtr);
  }

  return;
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
    errs() << "Function 'main.wrapper__c2go' not found!\n";
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

  if (wrapperCToGo) {
    for (auto &B : *wrapperCToGo) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (isa<CallInst>(I)) {
          CallInst *callInst = cast<CallInst>(&I);
          if (callInst->getCalledFunction() &&
              (callInst->getCalledFunction()->getIntrinsicID() ==
                   Intrinsic::lifetime_end ||
               callInst->getCalledFunction()->getIntrinsicID() ==
                   Intrinsic::lifetime_start)) {
            // I.eraseFromParent();
          }
        }
      }
    }
  }

  Function *cFuncGoString = M->getFunction("main.goStringToCCharPointer");
  if (!cFuncGoString) {
    errs() << "Function 'main.goStringToCCharPointer' not found!\n";
    return;
  }

  if (cFuncGoString) {
    for (auto &B : *cFuncGoString) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (isa<CallInst>(I)) {
          CallInst *callInst = cast<CallInst>(&I);
          if (callInst->getCalledFunction() &&
              (callInst->getCalledFunction()->getIntrinsicID() ==
                   Intrinsic::lifetime_end ||
               callInst->getCalledFunction()->getIntrinsicID() ==
                   Intrinsic::lifetime_start)) {
            // I.eraseFromParent();
          }
        }
      }
    }
  }

  Function *goStringCFunc = M->getFunction("main.cCharPointerToGoString");
  if (!goStringCFunc) {
    errs() << "Function 'main.cCharPointerToGoString' not found!\n";
    return;
  }

  if (goStringCFunc) {
    for (auto &B : *goStringCFunc) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (isa<CallInst>(I)) {
          CallInst *callInst = cast<CallInst>(&I);
          if (callInst->getCalledFunction() &&
              (callInst->getCalledFunction()->getIntrinsicID() ==
                   Intrinsic::lifetime_end ||
               callInst->getCalledFunction()->getIntrinsicID() ==
                   Intrinsic::lifetime_start)) {
            // I.eraseFromParent();
          }
        }
      }
    }
  }

  if (goStringCFunc) {
    for (auto &B : *goStringCFunc) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          Function *calledFunc = CI->getCalledFunction();
          if (calledFunc && calledFunc->getName() == "runtime.concatstrings") {
            insertBufferForConcat(CI);
          }
        }
      }
    }
  }

  if (wrapperCToGo) {
    for (auto &B : *wrapperCToGo) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          Function *calledFunc = CI->getCalledFunction();
          if (calledFunc &&
              calledFunc->getName() == "main.goStringToCCharPointer") {
            IRBuilder<> Builder(CI);
            Value *resultGoStringPtr = CI->getArgOperand(1);
            Function *printfFunc = CI->getModule()->getFunction("printf");
            if (!printfFunc) {
              errs() << "Printf function not found in the module!\n";
              return;
            }

            Constant *formatStr = Builder.CreateGlobalStringPtr("%s\n");
            Builder.CreateCall(printfFunc, {formatStr, resultGoStringPtr});
          }
        }
      }
    }
  }

  Function *CFuncToGoSting = M->getFunction("main.goStringToCCharPointer");
  if (!goStringCFunc) {
    errs() << "Function 'main.goStringToCCharPointer' not found!\n";
    return;
  }

  if (CFuncToGoSting) {
    for (auto &B : *CFuncToGoSting) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          Function *calledFunc = CI->getCalledFunction();
          if (calledFunc &&
              calledFunc->getName() == "runtime.stringtoslicebyte") {
            insertBufferForConcatForSlicebyte(CI);
          }
        }
      }
    }
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
  createCall2NewCallee(dummyCall, newCalleeFunc);

  if (rpcFunc->use_empty()) {
    rpcFunc->eraseFromParent();
    errs() << "make_rpc function has been removed from the module.\n";
  } else {
    errs()
        << "make_rpc function is still used elsewhere and cannot be removed.\n";
  }

  Function *calleeCloneMain = M->getFunction("calleeFromMain");
  if (calleeCloneMain) {
    errs() << "Original linkage: " << calleeCloneMain->getLinkage() << "\n";
    calleeCloneMain->setLinkage(GlobalValue::ExternalLinkage);
    errs() << "New linkage: " << calleeCloneMain->getLinkage() << "\n";
    // Iterate over the basic blocks in the function
    for (auto &B : *calleeCloneMain) {
      for (auto it = B.begin(); it != B.end();) {
        Instruction &I = *it++;

        // Check if the instruction is a call to 'runtime.concatstrings'
        if (CallInst *CI = dyn_cast<CallInst>(&I)) {
          Function *calledFunc = CI->getCalledFunction();
          if (calledFunc && calledFunc->getName() == "runtime.concatstrings") {
            // We assume that the return value is a struct { i8*, i64 }, extract the i8* (string)
            Value *result = CI;  // This is the return value of 'runtime.concatstrings'

            // Extract the string (i8*) from the returned struct
            IRBuilder<> Builder(CI->getNextNode()); // Move Builder after the current instruction
            Value *stringPtr = Builder.CreateExtractValue(result, 0, "string_ptr");

            // Check if printf is declared, if not declare it
            Function *printfFunc = M->getFunction("printf");
            if (!printfFunc) {
              errs() << "Printf function not found in the module!\n";
              return;
            }

            // Create the format string for printf
            Constant *formatStr = Builder.CreateGlobalStringPtr("Concatenated String: %s\n");

            // Call printf to print the concatenated string
            Builder.CreateCall(printfFunc, {formatStr, stringPtr});
          }
        }
      }
    }
  }
}