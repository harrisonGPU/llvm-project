//===- CodeGenDataWriter.h --------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains support for writing codegen data.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CGDATA_CODEGENDATAWRITER_H
#define LLVM_CGDATA_CODEGENDATAWRITER_H

#include "llvm/CGData/CGDataPatchItem.h"
#include "llvm/CGData/CodeGenData.h"
#include "llvm/CGData/OutlinedHashTreeRecord.h"
#include "llvm/CGData/StableFunctionMapRecord.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/Error.h"

namespace llvm {

/// A wrapper class to abstract writer stream with support of bytes
/// back patching.
class CGDataOStream {
  enum class OStreamKind {
    fd,
    string,
    svector,
  };

public:
  CGDataOStream(raw_fd_ostream &FD)
      : Kind(OStreamKind::fd), OS(FD), LE(FD, llvm::endianness::little) {}
  CGDataOStream(raw_string_ostream &STR)
      : Kind(OStreamKind::string), OS(STR), LE(STR, llvm::endianness::little) {}
  CGDataOStream(raw_svector_ostream &SVEC)
      : Kind(OStreamKind::svector), OS(SVEC),
        LE(SVEC, llvm::endianness::little) {}

  uint64_t tell() { return OS.tell(); }
  void write(uint64_t V) { LE.write<uint64_t>(V); }
  void write32(uint32_t V) { LE.write<uint32_t>(V); }
  void write8(uint8_t V) { LE.write<uint8_t>(V); }

  // \c patch can only be called when all data is written and flushed.
  // For raw_string_ostream, the patch is done on the target string
  // directly and it won't be reflected in the stream's internal buffer.
  LLVM_ABI void patch(ArrayRef<CGDataPatchItem> P);

  OStreamKind Kind;
  raw_ostream &OS;
  support::endian::Writer LE;
};

class CodeGenDataWriter {
  /// The outlined hash tree to be written.
  OutlinedHashTreeRecord HashTreeRecord;

  /// The stable function map to be written.
  StableFunctionMapRecord FunctionMapRecord;

  /// A bit mask describing the kind of the codegen data.
  CGDataKind DataKind = CGDataKind::Unknown;

public:
  CodeGenDataWriter() = default;
  ~CodeGenDataWriter() = default;

  /// Add the outlined hash tree record. The input hash tree is released.
  LLVM_ABI void addRecord(OutlinedHashTreeRecord &Record);

  /// Add the stable function map record. The input function map is released.
  LLVM_ABI void addRecord(StableFunctionMapRecord &Record);

  /// Write the codegen data to \c OS
  LLVM_ABI Error write(raw_fd_ostream &OS);

  /// Write the codegen data in text format to \c OS
  LLVM_ABI Error writeText(raw_fd_ostream &OS);

  /// Return the attributes of the current CGData.
  CGDataKind getCGDataKind() const { return DataKind; }

  /// Return true if the header indicates the data has an outlined hash tree.
  bool hasOutlinedHashTree() const {
    return static_cast<uint32_t>(DataKind) &
           static_cast<uint32_t>(CGDataKind::FunctionOutlinedHashTree);
  }
  /// Return true if the header indicates the data has a stable function map.
  bool hasStableFunctionMap() const {
    return static_cast<uint32_t>(DataKind) &
           static_cast<uint32_t>(CGDataKind::StableFunctionMergingMap);
  }

private:
  /// The offset of the outlined hash tree in the file.
  uint64_t OutlinedHashTreeOffset;

  /// The offset of the stable function map in the file.
  uint64_t StableFunctionMapOffset;

  /// Write the codegen data header to \c COS
  Error writeHeader(CGDataOStream &COS);

  /// Write the codegen data header in text to \c OS
  Error writeHeaderText(raw_fd_ostream &OS);

  Error writeImpl(CGDataOStream &COS);
};

} // end namespace llvm

#endif // LLVM_CGDATA_CODEGENDATAWRITER_H
