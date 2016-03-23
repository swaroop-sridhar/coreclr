// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

//===--------- coredistools.h - Dissassembly tools for CoreClr ------------===//
//
//  Core Disassembly Tools API Version 1.0.1-prerelease
//  Disassembly tools required by CoreCLR for utilities like
//  GCStress and SuperPMI
//===----------------------------------------------------------------------===//

#if !defined(_COREDISTOOLS_H_)
#define _COREDISTOOLS_H_

#include <stdint.h>

#if defined(__cplusplus)
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif // defined(__cplusplus)

#if defined(_MSC_VER)
#if defined(DllInterfaceExporter)
#define DllIface EXTERN_C __declspec(dllexport)
#else
#define DllIface EXTERN_C __declspec(dllimport)
#endif // defined(DllInterfaceExporter)
#else
#define DllIface EXTERN_C
#endif // defined(_MSC_VER)

enum TargetArch {
    Target_Host, // Target is the same as host architecture
    Target_X86,
    Target_X64,
    Target_Thumb,
    Target_Arm64
};

struct CorDisasm;
struct CorAsmDiff;

// The custom print functionality to be provide by the
// users of this Library
typedef void(*Printer)(const char *msg, ...);
struct PrintControl {
    const Printer Error;
    const Printer Warning;
    const Printer Log;
    const Printer Dump;
};

// The type of a custom function provided by the user to determine
// if two offsets are considered equivalent wrt diffing code blocks.
// Offset1 and Offset2 are the two offsets to be compared.
// BlockOffset is the offest of the instructions (that contain Offset1
// and Offset2) from the beginning of their respective code blocks.
// InstructionLength is the length of the current instruction being
// compared for equivalency.
typedef bool(*OffsetComparator)(const void *UserData, size_t BlockOffset,
    size_t InstructionLength, uint64_t Offset1,
    uint64_t Offset2);

// The Export/Import definitions for CoreDistools library are defined below.
// A typedef for a pointer to each interface function is also defined
// in order to aid the importer.

// Initialize the disassembler, using default print controls
DllIface CorDisasm *InitDisasm(enum TargetArch Target);
typedef CorDisasm *InitDisasm_Sig(enum TargetArch Target);

// Initialize the disassembler using custom print controls
DllIface CorDisasm *NewDisasm(enum TargetArch Target,
    const PrintControl *PControl);
typedef CorDisasm *NewDisasm_Sig(enum TargetArch Target,
    const PrintControl *PControl);

// Delete the disassembler
DllIface void FinishDisasm(const CorDisasm *Disasm);
typedef void FinishDisasm_Sig(const CorDisasm *Disasm);

// DisasmInstruction -- Disassemble one instruction
// Arguments:
// Disasm -- The Disassembler
// Address -- The address at which the bytes of the instruction
//            are intended to execute
// Bytes -- Pointer to the actual bytes which need to be disassembled
// MaxLength -- Number of bytes available in Bytes buffer
// Returns:
//   -- The Size of the disassembled instruction
//   -- Zero on failure
DllIface size_t DisasmInstruction(const CorDisasm *Disasm,
    const uint8_t *Address, const uint8_t *Bytes,
    size_t Maxlength);
typedef size_t DisasmInstruction_Sig(const CorDisasm *Disasm,
    const uint8_t *Address,
    const uint8_t *Bytes, size_t Maxlength);

// Initialize the Code Differ
DllIface CorAsmDiff *NewDiffer(enum TargetArch Target,
    const PrintControl *PControl,
    const OffsetComparator Comparator);
typedef CorAsmDiff *NewDiffer_Sig(enum TargetArch Target,
    const PrintControl *PControl,
    const OffsetComparator Comparator);

// Delete the Code Differ
DllIface void FinishDiff(const CorAsmDiff *AsmDiff);
typedef void FinishDiff_Sig(const CorAsmDiff *AsmDiff);

// NearDiffCodeBlocks -- Compare two code blocks for semantic
//                       equivalence
// Arguments:
// AsmDiff -- The Asm-differ
// UserData -- Any data the user wishes to pass through into
//             the OffsetComparator
// Address1 -- Address at which first block will execute
// Bytes1 -- Pointer to the actual bytes of the first block
// Size1 -- The size of the first block
// Address2 -- Address at which second block will execute
// Bytes2 -- Pointer to the actual bytes of the second block
// Size2 -- The size of the second block
// Returns:
//   -- true if the two blocks are equivalent, false if not.
DllIface bool NearDiffCodeBlocks(const CorAsmDiff *AsmDiff,
    const void *UserData, const uint8_t *Address1,
    const uint8_t *Bytes1, size_t Size1,
    const uint8_t *Address2, const uint8_t *Bytes2,
    size_t Size2);
typedef bool NearDiffCodeBlocks_Sig(const CorAsmDiff *AsmDiff,
    const void *UserData,
    const uint8_t *Address1,
    const uint8_t *Bytes1, size_t Size1,
    const uint8_t *Address2,
    const uint8_t *Bytes2, size_t Size2);

// Print a code block according to the Disassembler's Print Controls
DllIface void DumpCodeBlock(const CorDisasm *Disasm, const uint8_t *Address,
    const uint8_t *Bytes, size_t Size);
typedef void DumpCodeBlock_Sig(const CorDisasm *Disasm, const uint8_t *Address,
    const uint8_t *Bytes, size_t Size);

// Print the two code blocks being diffed, according to
// AsmDiff's PrintControls.
DllIface void DumpDiffBlocks(const CorAsmDiff *AsmDiff, const uint8_t *Address1,
    const uint8_t *Bytes1, size_t Size1,
    const uint8_t *Address2, const uint8_t *Bytes2,
    size_t Size2);
typedef void DumpDiffBlocks_Sig(const CorAsmDiff *AsmDiff,
    const uint8_t *Address1, const uint8_t *Bytes1,
    size_t Size1, const uint8_t *Address2,
    const uint8_t *Bytes2, size_t Size2);

#endif // !defined(_COREDISTOOLS_H_)
