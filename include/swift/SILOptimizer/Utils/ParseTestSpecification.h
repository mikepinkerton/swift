//===- ParseTestSpecification.h - Parsing for test instructions -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2022 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines Test::Argument.
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_SIL_PARSETESTSPECIFICATION
#define SWIFT_SIL_PARSETESTSPECIFICATION

#include "swift/Basic/TaggedUnion.h"
#include "swift/SIL/SILValue.h"
#include "llvm/ADT/StringRef.h"

namespace llvm {
template <class T>
class SmallVectorImpl;
}
using llvm::StringRef;

namespace swift {

class SILFunction;

namespace test {

struct Argument {
  enum class Kind {
    String,
    Bool,
    UInt,
    Value,
    Operand,
    Instruction,
    Block,
    Function,
  };
  using Union = TaggedUnion<StringRef,          // StringArgument
                            bool,               // BoolArgument
                            unsigned long long, // UIntArgument
                            SILValue,           // ValueArgument
                            Operand *,          // OperandArgument
                            SILInstruction *,   // InstructionArgument
                            SILBasicBlock *,    // BlockArgument
                            SILFunction *       // FunctionArgument
                            >;

private:
  Kind kind;

protected:
  Argument(Union storage, Kind kind) : kind(kind), storage(storage) {}
  Union storage;

public:
  Kind getKind() const { return kind; }
};

template <typename Stored, Argument::Kind TheKind>
struct ConcreteArgument : Argument {
  using Super = ConcreteArgument<Stored, TheKind>;
  ConcreteArgument(Stored stored) : Argument(Union{stored}, TheKind) {}
  Stored getValue() { return storage.get<Stored>(); }
  static bool classof(const Argument *argument) {
    return argument->getKind() == TheKind;
  }
};

struct ValueArgument : ConcreteArgument<SILValue, Argument::Kind::Value> {
  ValueArgument(SILValue stored) : Super(stored) {}
};

struct OperandArgument : ConcreteArgument<Operand *, Argument::Kind::Operand> {
  OperandArgument(Operand *stored) : Super(stored) {}
};

struct InstructionArgument
    : ConcreteArgument<SILInstruction *, Argument::Kind::Instruction> {
  InstructionArgument(SILInstruction *stored) : Super(stored) {}
};

struct BlockArgument
    : ConcreteArgument<SILBasicBlock *, Argument::Kind::Block> {
  BlockArgument(SILBasicBlock *stored) : Super(stored) {}
};

struct FunctionArgument
    : ConcreteArgument<SILFunction *, Argument::Kind::Function> {
  FunctionArgument(SILFunction *stored) : Super(stored) {}
};

struct BoolArgument : ConcreteArgument<bool, Argument::Kind::Bool> {
  BoolArgument(bool stored) : Super(stored) {}
};

struct UIntArgument
    : ConcreteArgument<unsigned long long, Argument::Kind::UInt> {
  UIntArgument(unsigned long long stored) : Super(stored) {}
};

struct StringArgument : ConcreteArgument<StringRef, Argument::Kind::String> {
  StringArgument(StringRef stored) : Super(stored) {}
};

struct Arguments {
  llvm::SmallVector<Argument, 8> storage;
  unsigned untakenIndex = 0;

  void assertUsed() {
    assert(untakenIndex == storage.size() && "arguments remain!");
  }

  void clear() {
    assertUsed();
    storage.clear();
    untakenIndex = 0;
  }

  bool hasUntaken() { return untakenIndex < storage.size(); }

  Arguments() {}
  Arguments(Arguments const &) = delete;
  Arguments &operator=(Arguments const &) = delete;
  ~Arguments() { assertUsed(); }
  Argument &takeArgument() { return storage[untakenIndex++]; }
  StringRef takeString() {
    return cast<StringArgument>(takeArgument()).getValue();
  }
  bool takeBool() { return cast<BoolArgument>(takeArgument()).getValue(); }
  unsigned long long takeUInt() {
    return cast<UIntArgument>(takeArgument()).getValue();
  }
  SILValue takeValue() {
    return cast<ValueArgument>(takeArgument()).getValue();
  }
  Operand *takeOperand() {
    return cast<OperandArgument>(takeArgument()).getValue();
  }
  SILInstruction *takeInstruction() {
    return cast<InstructionArgument>(takeArgument()).getValue();
  }
  SILBasicBlock *takeBlock() {
    return cast<BlockArgument>(takeArgument()).getValue();
  }
  SILFunction *takeFunction() {
    return cast<FunctionArgument>(takeArgument()).getValue();
  }
};

/// Finds and deletes each test_specification instruction in \p function and
/// appends its string payload to the provided vector.
void getTestSpecifications(SILFunction *function,
                           SmallVectorImpl<std::string> &specifications);

/// Given the string \p specification operand of a test_specification
/// instruction from \p function, parse the arguments which it refers to into
/// \p arguments and the component strings into \p argumentStrings.
void parseTestArgumentsFromSpecification(
    SILFunction *function, StringRef specification, Arguments &arguments,
    SmallVectorImpl<StringRef> &argumentStrings);

} // namespace test
} // namespace swift

#endif
