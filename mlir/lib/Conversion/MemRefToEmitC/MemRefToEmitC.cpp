//===- MemRefToEmitC.cpp - MemRef to EmitC conversion ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements patterns to convert memref ops into emitc ops.
//
//===----------------------------------------------------------------------===//

#include "mlir/Conversion/MemRefToEmitC/MemRefToEmitC.h"

#include "mlir/Conversion/ConvertToEmitC/ToEmitCInterface.h"
#include "mlir/Dialect/EmitC/IR/EmitC.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/Transforms/DialectConversion.h"

using namespace mlir;

namespace {
/// Implement the interface to convert MemRef to EmitC.
struct MemRefToEmitCDialectInterface : public ConvertToEmitCPatternInterface {
  using ConvertToEmitCPatternInterface::ConvertToEmitCPatternInterface;

  /// Hook for derived dialect interface to provide conversion patterns
  /// and mark dialect legal for the conversion target.
  void populateConvertToEmitCConversionPatterns(
      ConversionTarget &target, TypeConverter &typeConverter,
      RewritePatternSet &patterns) const final {
    populateMemRefToEmitCTypeConversion(typeConverter);
    populateMemRefToEmitCConversionPatterns(patterns, typeConverter);
  }
};
} // namespace

void mlir::registerConvertMemRefToEmitCInterface(DialectRegistry &registry) {
  registry.addExtension(+[](MLIRContext *ctx, memref::MemRefDialect *dialect) {
    dialect->addInterfaces<MemRefToEmitCDialectInterface>();
  });
}

//===----------------------------------------------------------------------===//
// Conversion Patterns
//===----------------------------------------------------------------------===//

namespace {
struct ConvertAlloca final : public OpConversionPattern<memref::AllocaOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(memref::AllocaOp op, OpAdaptor operands,
                  ConversionPatternRewriter &rewriter) const override {

    if (!op.getType().hasStaticShape()) {
      return rewriter.notifyMatchFailure(
          op.getLoc(), "cannot transform alloca with dynamic shape");
    }

    if (op.getAlignment().value_or(1) > 1) {
      // TODO: Allow alignment if it is not more than the natural alignment
      // of the C array.
      return rewriter.notifyMatchFailure(
          op.getLoc(), "cannot transform alloca with alignment requirement");
    }

    auto resultTy = getTypeConverter()->convertType(op.getType());
    if (!resultTy) {
      return rewriter.notifyMatchFailure(op.getLoc(), "cannot convert type");
    }
    auto noInit = emitc::OpaqueAttr::get(getContext(), "");
    rewriter.replaceOpWithNewOp<emitc::VariableOp>(op, resultTy, noInit);
    return success();
  }
};

Type convertMemRefType(MemRefType opTy, const TypeConverter *typeConverter) {
  Type resultTy;
  if (opTy.getRank() == 0) {
    resultTy = typeConverter->convertType(mlir::getElementTypeOrSelf(opTy));
  } else {
    resultTy = typeConverter->convertType(opTy);
  }
  return resultTy;
}

struct ConvertGlobal final : public OpConversionPattern<memref::GlobalOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(memref::GlobalOp op, OpAdaptor operands,
                  ConversionPatternRewriter &rewriter) const override {
    MemRefType opTy = op.getType();
    if (!op.getType().hasStaticShape()) {
      return rewriter.notifyMatchFailure(
          op.getLoc(), "cannot transform global with dynamic shape");
    }

    if (op.getAlignment().value_or(1) > 1) {
      // TODO: Extend GlobalOp to specify alignment via the `alignas` specifier.
      return rewriter.notifyMatchFailure(
          op.getLoc(), "global variable with alignment requirement is "
                       "currently not supported");
    }

    Type resultTy = convertMemRefType(opTy, getTypeConverter());

    if (!resultTy) {
      return rewriter.notifyMatchFailure(op.getLoc(),
                                         "cannot convert result type");
    }

    SymbolTable::Visibility visibility = SymbolTable::getSymbolVisibility(op);
    if (visibility != SymbolTable::Visibility::Public &&
        visibility != SymbolTable::Visibility::Private) {
      return rewriter.notifyMatchFailure(
          op.getLoc(),
          "only public and private visibility is currently supported");
    }
    // We are explicit in specifing the linkage because the default linkage
    // for constants is different in C and C++.
    bool staticSpecifier = visibility == SymbolTable::Visibility::Private;
    bool externSpecifier = !staticSpecifier;

    Attribute initialValue = operands.getInitialValueAttr();
    if (opTy.getRank() == 0) {
      auto elementsAttr = llvm::cast<ElementsAttr>(*op.getInitialValue());
      initialValue = elementsAttr.getSplatValue<Attribute>();
    }
    if (isa_and_present<UnitAttr>(initialValue))
      initialValue = {};

    rewriter.replaceOpWithNewOp<emitc::GlobalOp>(
        op, operands.getSymName(), resultTy, initialValue, externSpecifier,
        staticSpecifier, operands.getConstant());
    return success();
  }
};

struct ConvertGetGlobal final
    : public OpConversionPattern<memref::GetGlobalOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(memref::GetGlobalOp op, OpAdaptor operands,
                  ConversionPatternRewriter &rewriter) const override {

    MemRefType opTy = op.getType();
    Type resultTy = convertMemRefType(opTy, getTypeConverter());

    if (!resultTy) {
      return rewriter.notifyMatchFailure(op.getLoc(),
                                         "cannot convert result type");
    }

    if (opTy.getRank() == 0) {
      emitc::LValueType lvalueType = emitc::LValueType::get(resultTy);
      emitc::GetGlobalOp globalLValue = rewriter.create<emitc::GetGlobalOp>(
          op.getLoc(), lvalueType, operands.getNameAttr());
      emitc::PointerType pointerType = emitc::PointerType::get(resultTy);
      rewriter.replaceOpWithNewOp<emitc::ApplyOp>(
          op, pointerType, rewriter.getStringAttr("&"), globalLValue);
      return success();
    }
    rewriter.replaceOpWithNewOp<emitc::GetGlobalOp>(op, resultTy,
                                                    operands.getNameAttr());
    return success();
  }
};

struct ConvertLoad final : public OpConversionPattern<memref::LoadOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(memref::LoadOp op, OpAdaptor operands,
                  ConversionPatternRewriter &rewriter) const override {

    auto resultTy = getTypeConverter()->convertType(op.getType());
    if (!resultTy) {
      return rewriter.notifyMatchFailure(op.getLoc(), "cannot convert type");
    }

    auto arrayValue =
        dyn_cast<TypedValue<emitc::ArrayType>>(operands.getMemref());
    if (!arrayValue) {
      return rewriter.notifyMatchFailure(op.getLoc(), "expected array type");
    }

    auto subscript = rewriter.create<emitc::SubscriptOp>(
        op.getLoc(), arrayValue, operands.getIndices());

    rewriter.replaceOpWithNewOp<emitc::LoadOp>(op, resultTy, subscript);
    return success();
  }
};

struct ConvertStore final : public OpConversionPattern<memref::StoreOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(memref::StoreOp op, OpAdaptor operands,
                  ConversionPatternRewriter &rewriter) const override {
    auto arrayValue =
        dyn_cast<TypedValue<emitc::ArrayType>>(operands.getMemref());
    if (!arrayValue) {
      return rewriter.notifyMatchFailure(op.getLoc(), "expected array type");
    }

    auto subscript = rewriter.create<emitc::SubscriptOp>(
        op.getLoc(), arrayValue, operands.getIndices());
    rewriter.replaceOpWithNewOp<emitc::AssignOp>(op, subscript,
                                                 operands.getValue());
    return success();
  }
};
} // namespace

void mlir::populateMemRefToEmitCTypeConversion(TypeConverter &typeConverter) {
  typeConverter.addConversion(
      [&](MemRefType memRefType) -> std::optional<Type> {
        if (!memRefType.hasStaticShape() ||
            !memRefType.getLayout().isIdentity() || memRefType.getRank() == 0 ||
            llvm::is_contained(memRefType.getShape(), 0)) {
          return {};
        }
        Type convertedElementType =
            typeConverter.convertType(memRefType.getElementType());
        if (!convertedElementType)
          return {};
        return emitc::ArrayType::get(memRefType.getShape(),
                                     convertedElementType);
      });

  auto materializeAsUnrealizedCast = [](OpBuilder &builder, Type resultType,
                                        ValueRange inputs,
                                        Location loc) -> Value {
    if (inputs.size() != 1)
      return Value();

    return builder.create<UnrealizedConversionCastOp>(loc, resultType, inputs)
        .getResult(0);
  };

  typeConverter.addSourceMaterialization(materializeAsUnrealizedCast);
  typeConverter.addTargetMaterialization(materializeAsUnrealizedCast);
}

void mlir::populateMemRefToEmitCConversionPatterns(
    RewritePatternSet &patterns, const TypeConverter &converter) {
  patterns.add<ConvertAlloca, ConvertGlobal, ConvertGetGlobal, ConvertLoad,
               ConvertStore>(converter, patterns.getContext());
}
