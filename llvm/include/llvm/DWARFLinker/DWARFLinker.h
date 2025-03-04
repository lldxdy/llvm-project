//===- DWARFLinker.h --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DWARFLINKER_DWARFLINKER_H
#define LLVM_DWARFLINKER_DWARFLINKER_H

#include "llvm/ADT/AddressRanges.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/CodeGen/AccelTable.h"
#include "llvm/CodeGen/NonRelocatableStringpool.h"
#include "llvm/DWARFLinker/DWARFLinkerCompileUnit.h"
#include "llvm/DebugInfo/DWARF/DWARFContext.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugLine.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugRangeList.h"
#include "llvm/DebugInfo/DWARF/DWARFDie.h"
#include "llvm/DebugInfo/DWARF/DWARFExpression.h"
#include <map>

namespace llvm {
class DWARFExpression;
class DWARFUnit;
class DataExtractor;
class DeclContextTree;
template <typename T> class SmallVectorImpl;

enum class DwarfLinkerClient { Dsymutil, LLD, General };

/// AddressesMap represents information about valid addresses used
/// by debug information. Valid addresses are those which points to
/// live code sections. i.e. relocations for these addresses point
/// into sections which would be/are placed into resulting binary.
class AddressesMap {
public:
  virtual ~AddressesMap();

  /// Checks that there are valid relocations against a .debug_info
  /// section.
  virtual bool hasValidRelocs() = 0;

  /// Checks that the specified DWARF expression operand \p Op references live
  /// code section and returns the relocation adjustment value (to get the
  /// linked address this value might be added to the source expression operand
  /// address).
  /// \returns relocation adjustment value or std::nullopt if there is no
  /// corresponding live address.
  virtual std::optional<int64_t>
  getExprOpAddressRelocAdjustment(DWARFUnit &U,
                                  const DWARFExpression::Operation &Op,
                                  uint64_t StartOffset, uint64_t EndOffset) = 0;

  /// Checks that the specified subprogram \p DIE references the live code
  /// section and returns the relocation adjustment value (to get the linked
  /// address this value might be added to the source subprogram address).
  /// Allowed kinds of input DIE: DW_TAG_subprogram, DW_TAG_label.
  /// \returns relocation adjustment value or std::nullopt if there is no
  /// corresponding live address.
  virtual std::optional<int64_t>
  getSubprogramRelocAdjustment(const DWARFDie &DIE) = 0;

  /// Apply the valid relocations to the buffer \p Data, taking into
  /// account that Data is at \p BaseOffset in the .debug_info section.
  ///
  /// \returns true whether any reloc has been applied.
  virtual bool applyValidRelocs(MutableArrayRef<char> Data, uint64_t BaseOffset,
                                bool IsLittleEndian) = 0;

  /// Erases all data.
  virtual void clear() = 0;
};

using Offset2UnitMap = DenseMap<uint64_t, CompileUnit *>;

/// DwarfEmitter presents interface to generate all debug info tables.
class DwarfEmitter {
public:
  virtual ~DwarfEmitter();

  /// Emit DIE containing warnings.
  virtual void emitPaperTrailWarningsDie(DIE &Die) = 0;

  /// Emit section named SecName with data SecData.
  virtual void emitSectionContents(StringRef SecData, StringRef SecName) = 0;

  /// Emit the abbreviation table \p Abbrevs to the .debug_abbrev section.
  virtual void
  emitAbbrevs(const std::vector<std::unique_ptr<DIEAbbrev>> &Abbrevs,
              unsigned DwarfVersion) = 0;

  /// Emit the string table described by \p Pool into .debug_str table.
  virtual void emitStrings(const NonRelocatableStringpool &Pool) = 0;

  /// Emit the string table described by \p Pool into .debug_line_str table.
  virtual void emitLineStrings(const NonRelocatableStringpool &Pool) = 0;

  /// Emit DWARF debug names.
  virtual void
  emitDebugNames(AccelTable<DWARF5AccelTableStaticData> &Table) = 0;

  /// Emit Apple namespaces accelerator table.
  virtual void
  emitAppleNamespaces(AccelTable<AppleAccelTableStaticOffsetData> &Table) = 0;

  /// Emit Apple names accelerator table.
  virtual void
  emitAppleNames(AccelTable<AppleAccelTableStaticOffsetData> &Table) = 0;

  /// Emit Apple Objective-C accelerator table.
  virtual void
  emitAppleObjc(AccelTable<AppleAccelTableStaticOffsetData> &Table) = 0;

  /// Emit Apple type accelerator table.
  virtual void
  emitAppleTypes(AccelTable<AppleAccelTableStaticTypeData> &Table) = 0;

  /// Emit debug ranges (.debug_ranges, .debug_rnglists) header.
  virtual MCSymbol *emitDwarfDebugRangeListHeader(const CompileUnit &Unit) = 0;

  /// Emit debug ranges (.debug_ranges, .debug_rnglists) fragment.
  virtual void
  emitDwarfDebugRangeListFragment(const CompileUnit &Unit,
                                  const AddressRanges &LinkedRanges,
                                  PatchLocation Patch) = 0;

  /// Emit debug ranges (.debug_ranges, .debug_rnglists) footer.
  virtual void emitDwarfDebugRangeListFooter(const CompileUnit &Unit,
                                             MCSymbol *EndLabel) = 0;

  /// Emit debug locations (.debug_loc, .debug_loclists) header.
  virtual MCSymbol *emitDwarfDebugLocListHeader(const CompileUnit &Unit) = 0;

  /// Emit debug locations (.debug_loc, .debug_loclists) fragment.
  virtual void emitDwarfDebugLocListFragment(
      const CompileUnit &Unit,
      const DWARFLocationExpressionsVector &LinkedLocationExpression,
      PatchLocation Patch) = 0;

  /// Emit debug locations (.debug_loc, .debug_loclists) footer.
  virtual void emitDwarfDebugLocListFooter(const CompileUnit &Unit,
                                           MCSymbol *EndLabel) = 0;

  /// Emit .debug_aranges entries for \p Unit
  virtual void
  emitDwarfDebugArangesTable(const CompileUnit &Unit,
                             const AddressRanges &LinkedRanges) = 0;

  /// Emit specified \p LineTable into .debug_line table.
  virtual void emitLineTableForUnit(const DWARFDebugLine::LineTable &LineTable,
                                    const CompileUnit &Unit,
                                    OffsetsStringPool &DebugStrPool,
                                    OffsetsStringPool &DebugLineStrPool) = 0;

  /// Emit the .debug_pubnames contribution for \p Unit.
  virtual void emitPubNamesForUnit(const CompileUnit &Unit) = 0;

  /// Emit the .debug_pubtypes contribution for \p Unit.
  virtual void emitPubTypesForUnit(const CompileUnit &Unit) = 0;

  /// Emit a CIE.
  virtual void emitCIE(StringRef CIEBytes) = 0;

  /// Emit an FDE with data \p Bytes.
  virtual void emitFDE(uint32_t CIEOffset, uint32_t AddreSize, uint64_t Address,
                       StringRef Bytes) = 0;

  /// Emit the compilation unit header for \p Unit in the
  /// .debug_info section.
  ///
  /// As a side effect, this also switches the current Dwarf version
  /// of the MC layer to the one of U.getOrigUnit().
  virtual void emitCompileUnitHeader(CompileUnit &Unit,
                                     unsigned DwarfVersion) = 0;

  /// Recursively emit the DIE tree rooted at \p Die.
  virtual void emitDIE(DIE &Die) = 0;

  /// Emit all available macro tables(DWARFv4 and DWARFv5).
  /// Use \p UnitMacroMap to get compilation unit by macro table offset.
  /// Side effects: Fill \p StringPool with macro strings, update
  /// DW_AT_macro_info, DW_AT_macros attributes for corresponding compile
  /// units.
  virtual void emitMacroTables(DWARFContext *Context,
                               const Offset2UnitMap &UnitMacroMap,
                               OffsetsStringPool &StringPool) = 0;

  /// Returns size of generated .debug_line section.
  virtual uint64_t getLineSectionSize() const = 0;

  /// Returns size of generated .debug_frame section.
  virtual uint64_t getFrameSectionSize() const = 0;

  /// Returns size of generated .debug_ranges section.
  virtual uint64_t getRangesSectionSize() const = 0;

  /// Returns size of generated .debug_rnglists section.
  virtual uint64_t getRngListsSectionSize() const = 0;

  /// Returns size of generated .debug_info section.
  virtual uint64_t getDebugInfoSectionSize() const = 0;

  /// Returns size of generated .debug_macinfo section.
  virtual uint64_t getDebugMacInfoSectionSize() const = 0;

  /// Returns size of generated .debug_macro section.
  virtual uint64_t getDebugMacroSectionSize() const = 0;

  /// Returns size of generated .debug_loclists section.
  virtual uint64_t getLocListsSectionSize() const = 0;

  /// Dump the file to the disk.
  virtual void finish() = 0;

  /// Emit the swift_ast section stored in \p Buffer.
  virtual void emitSwiftAST(StringRef Buffer) = 0;

  /// Emit the swift reflection section stored in \p Buffer.
  virtual void emitSwiftReflectionSection(
      llvm::binaryformat::Swift5ReflectionSectionKind ReflSectionKind,
      StringRef Buffer, uint32_t Alignment, uint32_t Size) = 0;

  /// Returns underlying AsmPrinter.
  virtual AsmPrinter &getAsmPrinter() const = 0;
};

class DwarfStreamer;
using UnitListTy = std::vector<std::unique_ptr<CompileUnit>>;

/// This class represents DWARF information for source file
/// and its address map.
class DWARFFile {
public:
  DWARFFile(StringRef Name, std::unique_ptr<DWARFContext> Dwarf,
            std::unique_ptr<AddressesMap> Addresses,
            const std::vector<std::string> &Warnings)
      : FileName(Name), Dwarf(std::move(Dwarf)),
        Addresses(std::move(Addresses)), Warnings(Warnings) {}

  /// The object file name.
  StringRef FileName;

  /// The source DWARF information.
  std::unique_ptr<DWARFContext> Dwarf;

  /// Helpful address information(list of valid address ranges, relocations).
  std::unique_ptr<AddressesMap> Addresses;

  /// Warnings for this object file.
  const std::vector<std::string> &Warnings;
};

typedef std::map<std::string, std::string> swiftInterfacesMap;
typedef std::map<std::string, std::string> objectPrefixMap;

typedef function_ref<void(const DWARFUnit &Unit)> CompileUnitHandler;

/// The core of the Dwarf linking logic.
///
/// The generation of the dwarf information from the object files will be
/// driven by the selection of 'root DIEs', which are DIEs that
/// describe variables or functions that resolves to the corresponding
/// code section(and thus have entries in the Addresses map). All the debug
/// information that will be generated(the DIEs, but also the line
/// tables, ranges, ...) is derived from that set of root DIEs.
///
/// The root DIEs are identified because they contain relocations that
/// points to code section(the low_pc for a function, the location for
/// a variable). These relocations are called ValidRelocs in the
/// AddressesInfo and are gathered as a very first step when we start
/// processing a object file.
class DWARFLinker {
public:
  typedef std::function<void(const Twine &Warning, StringRef Context,
                             const DWARFDie *DIE)>
      messageHandler;
  DWARFLinker(messageHandler ErrorHandler, messageHandler WarningHandler,
              std::function<StringRef(StringRef)> StringsTranslator)
      : DwarfLinkerClientID(DwarfLinkerClient::Dsymutil),
        StringsTranslator(StringsTranslator), ErrorHandler(ErrorHandler),
        WarningHandler(WarningHandler) {}

  static std::unique_ptr<DWARFLinker> createLinker(
      messageHandler ErrorHandler, messageHandler WarningHandler,
      std::function<StringRef(StringRef)> StringsTranslator = nullptr) {
    return std::make_unique<DWARFLinker>(ErrorHandler, WarningHandler,
                                         StringsTranslator);
  }

  /// Type of output file.
  enum class OutputFileType {
    Object,
    Assembly,
  };

  /// The kind of accelerator tables we should emit.
  enum class AccelTableKind : uint8_t {
    Apple,     ///< .apple_names, .apple_namespaces, .apple_types, .apple_objc.
    Pub,       ///< .debug_pubnames, .debug_pubtypes
    DebugNames ///< .debug_names.
  };
  typedef std::function<void(const DWARFFile &File)> inputVerificationHandler;
  typedef std::function<ErrorOr<DWARFFile &>(StringRef ContainerName,
                                             StringRef Path)>
      objFileLoader;

  Error createEmitter(const Triple &TheTriple, OutputFileType FileType,
                      raw_pwrite_stream &OutFile);

  DwarfEmitter *getEmitter();

  /// Add object file to be linked. Pre-load compile unit die. Call
  /// \p OnCUDieLoaded for each compile unit die. If specified \p File
  /// has reference to the Clang module then such module would be
  /// pre-loaded by \p Loader for !Update case.
  ///
  /// \pre NoODR, Update options should be set before call to addObjectFile.
  void addObjectFile(
      DWARFFile &File, objFileLoader Loader = nullptr,
      CompileUnitHandler OnCUDieLoaded = [](const DWARFUnit &) {});

  /// Link debug info for added objFiles. Object files are linked all together.
  Error link();

  /// A number of methods setting various linking options:

  /// Allows to generate log of linking process to the standard output.
  void setVerbosity(bool Verbose) { Options.Verbose = Verbose; }

  /// Print statistics to standard output.
  void setStatistics(bool Statistics) { Options.Statistics = Statistics; }

  /// Verify the input DWARF.
  void setVerifyInputDWARF(bool Verify) { Options.VerifyInputDWARF = Verify; }

  /// Do not unique types according to ODR.
  void setNoODR(bool NoODR) { Options.NoODR = NoODR; }

  /// Update index tables only(do not modify rest of DWARF).
  void setUpdateIndexTablesOnly(bool Update) { Options.Update = Update; }

  /// Allow generating valid, but non-deterministic output.
  void setAllowNonDeterministicOutput(bool) { /* Nothing to do. */
  }

  /// Set whether to keep the enclosing function for a static variable.
  void setKeepFunctionForStatic(bool KeepFunctionForStatic) {
    Options.KeepFunctionForStatic = KeepFunctionForStatic;
  }

  /// Use specified number of threads for parallel files linking.
  void setNumThreads(unsigned NumThreads) { Options.Threads = NumThreads; }

  /// Add kind of accelerator tables to be generated.
  void addAccelTableKind(AccelTableKind Kind) {
    assert(!llvm::is_contained(Options.AccelTables, Kind));
    Options.AccelTables.emplace_back(Kind);
  }

  /// Set prepend path for clang modules.
  void setPrependPath(const std::string &Ppath) { Options.PrependPath = Ppath; }

  /// Set estimated objects files amount, for preliminary data allocation.
  void setEstimatedObjfilesAmount(unsigned ObjFilesNum) {
    ObjectContexts.reserve(ObjFilesNum);
  }

  /// Set verification handler which would be used to report verification
  /// errors.
  void setInputVerificationHandler(inputVerificationHandler Handler) {
    Options.InputVerificationHandler = Handler;
  }

  /// Set map for Swift interfaces.
  void setSwiftInterfacesMap(swiftInterfacesMap *Map) {
    Options.ParseableSwiftInterfaces = Map;
  }

  /// Set prefix map for objects.
  void setObjectPrefixMap(objectPrefixMap *Map) {
    Options.ObjectPrefixMap = Map;
  }

  /// Set target DWARF version.
  Error setTargetDWARFVersion(uint16_t TargetDWARFVersion) {
    if ((TargetDWARFVersion < 1) || (TargetDWARFVersion > 5))
      return createStringError(std::errc::invalid_argument,
                               "unsupported DWARF version: %d",
                               TargetDWARFVersion);

    Options.TargetDWARFVersion = TargetDWARFVersion;
    return Error::success();
  }

private:
  /// Flags passed to DwarfLinker::lookForDIEsToKeep
  enum TraversalFlags {
    TF_Keep = 1 << 0,            ///< Mark the traversed DIEs as kept.
    TF_InFunctionScope = 1 << 1, ///< Current scope is a function scope.
    TF_DependencyWalk = 1 << 2,  ///< Walking the dependencies of a kept DIE.
    TF_ParentWalk = 1 << 3,      ///< Walking up the parents of a kept DIE.
    TF_ODR = 1 << 4,             ///< Use the ODR while keeping dependents.
    TF_SkipPC = 1 << 5,          ///< Skip all location attributes.
  };

  /// The  distinct types of work performed by the work loop.
  enum class WorklistItemType {
    /// Given a DIE, look for DIEs to be kept.
    LookForDIEsToKeep,
    /// Given a DIE, look for children of this DIE to be kept.
    LookForChildDIEsToKeep,
    /// Given a DIE, look for DIEs referencing this DIE to be kept.
    LookForRefDIEsToKeep,
    /// Given a DIE, look for parent DIEs to be kept.
    LookForParentDIEsToKeep,
    /// Given a DIE, update its incompleteness based on whether its children are
    /// incomplete.
    UpdateChildIncompleteness,
    /// Given a DIE, update its incompleteness based on whether the DIEs it
    /// references are incomplete.
    UpdateRefIncompleteness,
    /// Given a DIE, mark it as ODR Canonical if applicable.
    MarkODRCanonicalDie,
  };

  /// This class represents an item in the work list. The type defines what kind
  /// of work needs to be performed when processing the current item. The flags
  /// and info fields are optional based on the type.
  struct WorklistItem {
    DWARFDie Die;
    WorklistItemType Type;
    CompileUnit &CU;
    unsigned Flags;
    union {
      const unsigned AncestorIdx;
      CompileUnit::DIEInfo *OtherInfo;
    };

    WorklistItem(DWARFDie Die, CompileUnit &CU, unsigned Flags,
                 WorklistItemType T = WorklistItemType::LookForDIEsToKeep)
        : Die(Die), Type(T), CU(CU), Flags(Flags), AncestorIdx(0) {}

    WorklistItem(DWARFDie Die, CompileUnit &CU, WorklistItemType T,
                 CompileUnit::DIEInfo *OtherInfo = nullptr)
        : Die(Die), Type(T), CU(CU), Flags(0), OtherInfo(OtherInfo) {}

    WorklistItem(unsigned AncestorIdx, CompileUnit &CU, unsigned Flags)
        : Type(WorklistItemType::LookForParentDIEsToKeep), CU(CU), Flags(Flags),
          AncestorIdx(AncestorIdx) {}
  };

  /// Verify the given DWARF file.
  void verifyInput(const DWARFFile &File);

  /// returns true if we need to translate strings.
  bool needToTranslateStrings() { return StringsTranslator != nullptr; }

  void reportWarning(const Twine &Warning, const DWARFFile &File,
                     const DWARFDie *DIE = nullptr) const {
    if (WarningHandler != nullptr)
      WarningHandler(Warning, File.FileName, DIE);
  }

  void reportError(const Twine &Warning, const DWARFFile &File,
                   const DWARFDie *DIE = nullptr) const {
    if (ErrorHandler != nullptr)
      ErrorHandler(Warning, File.FileName, DIE);
  }

  /// Emit warnings as Dwarf compile units to leave a trail after linking.
  bool emitPaperTrailWarnings(const DWARFFile &File,
                              OffsetsStringPool &StringPool);

  void copyInvariantDebugSection(DWARFContext &Dwarf);

  /// Keep information for referenced clang module: already loaded DWARF info
  /// of the clang module and a CompileUnit of the module.
  struct RefModuleUnit {
    RefModuleUnit(DWARFFile &File, std::unique_ptr<CompileUnit> Unit)
        : File(File), Unit(std::move(Unit)) {}
    RefModuleUnit(RefModuleUnit &&Other)
        : File(Other.File), Unit(std::move(Other.Unit)) {}
    RefModuleUnit(const RefModuleUnit &) = delete;

    DWARFFile &File;
    std::unique_ptr<CompileUnit> Unit;
  };
  using ModuleUnitListTy = std::vector<RefModuleUnit>;

  /// Keeps track of data associated with one object during linking.
  struct LinkContext {
    DWARFFile &File;
    UnitListTy CompileUnits;
    ModuleUnitListTy ModuleUnits;
    bool Skip = false;

    LinkContext(DWARFFile &File) : File(File) {}

    /// Clear part of the context that's no longer needed when we're done with
    /// the debug object.
    void clear() {
      CompileUnits.clear();
      File.Addresses->clear();
    }
  };

  /// Called before emitting object data
  void cleanupAuxiliarryData(LinkContext &Context);

  /// Look at the parent of the given DIE and decide whether they should be
  /// kept.
  void lookForParentDIEsToKeep(unsigned AncestorIdx, CompileUnit &CU,
                               unsigned Flags,
                               SmallVectorImpl<WorklistItem> &Worklist);

  /// Look at the children of the given DIE and decide whether they should be
  /// kept.
  void lookForChildDIEsToKeep(const DWARFDie &Die, CompileUnit &CU,
                              unsigned Flags,
                              SmallVectorImpl<WorklistItem> &Worklist);

  /// Look at DIEs referenced by the given DIE and decide whether they should be
  /// kept. All DIEs referenced though attributes should be kept.
  void lookForRefDIEsToKeep(const DWARFDie &Die, CompileUnit &CU,
                            unsigned Flags, const UnitListTy &Units,
                            const DWARFFile &File,
                            SmallVectorImpl<WorklistItem> &Worklist);

  /// Mark context corresponding to the specified \p Die as having canonical
  /// die, if applicable.
  void markODRCanonicalDie(const DWARFDie &Die, CompileUnit &CU);

  /// \defgroup FindRootDIEs Find DIEs corresponding to Address map entries.
  ///
  /// @{
  /// Recursively walk the \p DIE tree and look for DIEs to
  /// keep. Store that information in \p CU's DIEInfo.
  ///
  /// The return value indicates whether the DIE is incomplete.
  void lookForDIEsToKeep(AddressesMap &RelocMgr, const UnitListTy &Units,
                         const DWARFDie &DIE, const DWARFFile &File,
                         CompileUnit &CU, unsigned Flags);

  /// Check whether specified \p CUDie is a Clang module reference.
  /// if \p Quiet is false then display error messages.
  /// \return first == true if CUDie is a Clang module reference.
  ///         second == true if module is already loaded.
  std::pair<bool, bool> isClangModuleRef(const DWARFDie &CUDie,
                                         std::string &PCMFile,
                                         LinkContext &Context, unsigned Indent,
                                         bool Quiet);

  /// If this compile unit is really a skeleton CU that points to a
  /// clang module, register it in ClangModules and return true.
  ///
  /// A skeleton CU is a CU without children, a DW_AT_gnu_dwo_name
  /// pointing to the module, and a DW_AT_gnu_dwo_id with the module
  /// hash.
  bool registerModuleReference(const DWARFDie &CUDie, LinkContext &Context,
                               objFileLoader Loader,
                               CompileUnitHandler OnCUDieLoaded,
                               unsigned Indent = 0);

  /// Recursively add the debug info in this clang module .pcm
  /// file (and all the modules imported by it in a bottom-up fashion)
  /// to ModuleUnits.
  Error loadClangModule(objFileLoader Loader, const DWARFDie &CUDie,
                        const std::string &PCMFile, LinkContext &Context,
                        CompileUnitHandler OnCUDieLoaded, unsigned Indent = 0);

  /// Clone specified Clang module unit \p Unit.
  Error cloneModuleUnit(LinkContext &Context, RefModuleUnit &Unit,
                        DeclContextTree &ODRContexts,
                        OffsetsStringPool &DebugStrPool,
                        OffsetsStringPool &DebugLineStrPool,
                        unsigned Indent = 0);

  unsigned shouldKeepDIE(AddressesMap &RelocMgr, const DWARFDie &DIE,
                         const DWARFFile &File, CompileUnit &Unit,
                         CompileUnit::DIEInfo &MyInfo, unsigned Flags);

  /// This function checks whether variable has DWARF expression containing
  /// operation referencing live address(f.e. DW_OP_addr, DW_OP_addrx...).
  /// \returns first is true if the expression has an operation referencing an
  /// address.
  ///          second is the relocation adjustment value if the live address is
  ///          referenced.
  std::pair<bool, std::optional<int64_t>>
  getVariableRelocAdjustment(AddressesMap &RelocMgr, const DWARFDie &DIE);

  /// Check if a variable describing DIE should be kept.
  /// \returns updated TraversalFlags.
  unsigned shouldKeepVariableDIE(AddressesMap &RelocMgr, const DWARFDie &DIE,
                                 CompileUnit::DIEInfo &MyInfo, unsigned Flags);

  unsigned shouldKeepSubprogramDIE(AddressesMap &RelocMgr, const DWARFDie &DIE,
                                   const DWARFFile &File, CompileUnit &Unit,
                                   CompileUnit::DIEInfo &MyInfo,
                                   unsigned Flags);

  /// Resolve the DIE attribute reference that has been extracted in \p
  /// RefValue. The resulting DIE might be in another CompileUnit which is
  /// stored into \p ReferencedCU. \returns null if resolving fails for any
  /// reason.
  DWARFDie resolveDIEReference(const DWARFFile &File, const UnitListTy &Units,
                               const DWARFFormValue &RefValue,
                               const DWARFDie &DIE, CompileUnit *&RefCU);

  /// @}

  /// \defgroup Methods used to link the debug information
  ///
  /// @{

  struct DWARFLinkerOptions;

  class DIECloner {
    DWARFLinker &Linker;
    DwarfEmitter *Emitter;
    DWARFFile &ObjFile;
    OffsetsStringPool &DebugStrPool;
    OffsetsStringPool &DebugLineStrPool;

    /// Allocator used for all the DIEValue objects.
    BumpPtrAllocator &DIEAlloc;

    std::vector<std::unique_ptr<CompileUnit>> &CompileUnits;

    /// Keeps mapping from offset of the macro table to corresponding
    /// compile unit.
    Offset2UnitMap UnitMacroMap;

    bool Update;

  public:
    DIECloner(DWARFLinker &Linker, DwarfEmitter *Emitter, DWARFFile &ObjFile,
              BumpPtrAllocator &DIEAlloc,
              std::vector<std::unique_ptr<CompileUnit>> &CompileUnits,
              bool Update, OffsetsStringPool &DebugStrPool,
              OffsetsStringPool &DebugLineStrPool)
        : Linker(Linker), Emitter(Emitter), ObjFile(ObjFile),
          DebugStrPool(DebugStrPool), DebugLineStrPool(DebugLineStrPool),
          DIEAlloc(DIEAlloc), CompileUnits(CompileUnits), Update(Update) {}

    /// Recursively clone \p InputDIE into an tree of DIE objects
    /// where useless (as decided by lookForDIEsToKeep()) bits have been
    /// stripped out and addresses have been rewritten according to the
    /// address map.
    ///
    /// \param OutOffset is the offset the cloned DIE in the output
    /// compile unit.
    /// \param PCOffset (while cloning a function scope) is the offset
    /// applied to the entry point of the function to get the linked address.
    /// \param Die the output DIE to use, pass NULL to create one.
    /// \returns the root of the cloned tree or null if nothing was selected.
    DIE *cloneDIE(const DWARFDie &InputDIE, const DWARFFile &File,
                  CompileUnit &U, int64_t PCOffset, uint32_t OutOffset,
                  unsigned Flags, bool IsLittleEndian, DIE *Die = nullptr);

    /// Construct the output DIE tree by cloning the DIEs we
    /// chose to keep above. If there are no valid relocs, then there's
    /// nothing to clone/emit.
    uint64_t cloneAllCompileUnits(DWARFContext &DwarfContext,
                                  const DWARFFile &File, bool IsLittleEndian);

  private:
    using AttributeSpec = DWARFAbbreviationDeclaration::AttributeSpec;

    /// Information gathered and exchanged between the various
    /// clone*Attributes helpers about the attributes of a particular DIE.
    struct AttributesInfo {
      /// Names.
      DwarfStringPoolEntryRef Name, MangledName, NameWithoutTemplate;

      /// Offsets in the string pool.
      uint32_t NameOffset = 0;
      uint32_t MangledNameOffset = 0;

      /// Offset to apply to PC addresses inside a function.
      int64_t PCOffset = 0;

      /// Does the DIE have a low_pc attribute?
      bool HasLowPc = false;

      /// Does the DIE have a ranges attribute?
      bool HasRanges = false;

      /// Is this DIE only a declaration?
      bool IsDeclaration = false;

      AttributesInfo() = default;
    };

    /// Helper for cloneDIE.
    unsigned cloneAttribute(DIE &Die, const DWARFDie &InputDIE,
                            const DWARFFile &File, CompileUnit &U,
                            const DWARFFormValue &Val,
                            const AttributeSpec AttrSpec, unsigned AttrSize,
                            AttributesInfo &AttrInfo, bool IsLittleEndian);

    /// Clone a string attribute described by \p AttrSpec and add
    /// it to \p Die.
    /// \returns the size of the new attribute.
    unsigned cloneStringAttribute(DIE &Die, AttributeSpec AttrSpec,
                                  const DWARFFormValue &Val, const DWARFUnit &U,
                                  AttributesInfo &Info);

    /// Clone an attribute referencing another DIE and add
    /// it to \p Die.
    /// \returns the size of the new attribute.
    unsigned cloneDieReferenceAttribute(DIE &Die, const DWARFDie &InputDIE,
                                        AttributeSpec AttrSpec,
                                        unsigned AttrSize,
                                        const DWARFFormValue &Val,
                                        const DWARFFile &File,
                                        CompileUnit &Unit);

    /// Clone a DWARF expression that may be referencing another DIE.
    void cloneExpression(DataExtractor &Data, DWARFExpression Expression,
                         const DWARFFile &File, CompileUnit &Unit,
                         SmallVectorImpl<uint8_t> &OutputBuffer,
                         int64_t AddrRelocAdjustment, bool IsLittleEndian);

    /// Clone an attribute referencing another DIE and add
    /// it to \p Die.
    /// \returns the size of the new attribute.
    unsigned cloneBlockAttribute(DIE &Die, const DWARFDie &InputDIE,
                                 const DWARFFile &File, CompileUnit &Unit,
                                 AttributeSpec AttrSpec,
                                 const DWARFFormValue &Val,
                                 bool IsLittleEndian);

    /// Clone an attribute referencing another DIE and add
    /// it to \p Die.
    /// \returns the size of the new attribute.
    unsigned cloneAddressAttribute(DIE &Die, const DWARFDie &InputDIE,
                                   AttributeSpec AttrSpec, unsigned AttrSize,
                                   const DWARFFormValue &Val,
                                   const CompileUnit &Unit,
                                   AttributesInfo &Info);

    /// Clone a scalar attribute  and add it to \p Die.
    /// \returns the size of the new attribute.
    unsigned cloneScalarAttribute(DIE &Die, const DWARFDie &InputDIE,
                                  const DWARFFile &File, CompileUnit &U,
                                  AttributeSpec AttrSpec,
                                  const DWARFFormValue &Val, unsigned AttrSize,
                                  AttributesInfo &Info);

    /// Get the potential name and mangled name for the entity
    /// described by \p Die and store them in \Info if they are not
    /// already there.
    /// \returns is a name was found.
    bool getDIENames(const DWARFDie &Die, AttributesInfo &Info,
                     OffsetsStringPool &StringPool, bool StripTemplate = false);

    uint32_t hashFullyQualifiedName(DWARFDie DIE, CompileUnit &U,
                                    const DWARFFile &File,
                                    int RecurseDepth = 0);

    /// Helper for cloneDIE.
    void addObjCAccelerator(CompileUnit &Unit, const DIE *Die,
                            DwarfStringPoolEntryRef Name,
                            OffsetsStringPool &StringPool, bool SkipPubSection);

    void rememberUnitForMacroOffset(CompileUnit &Unit);

    /// Clone and emit the line table for the specified \p Unit.
    /// Translate directories and file names if necessary.
    /// Relocate address ranges.
    void generateLineTableForUnit(CompileUnit &Unit);
  };

  /// Assign an abbreviation number to \p Abbrev
  void assignAbbrev(DIEAbbrev &Abbrev);

  /// Compute and emit debug ranges(.debug_aranges, .debug_ranges,
  /// .debug_rnglists) for \p Unit, patch the attributes referencing it.
  void generateUnitRanges(CompileUnit &Unit, const DWARFFile &File) const;

  using ExpressionHandlerRef =
      function_ref<void(SmallVectorImpl<uint8_t> &, SmallVectorImpl<uint8_t> &,
                        int64_t AddrRelocAdjustment)>;

  /// Compute and emit debug locations (.debug_loc, .debug_loclists)
  /// for \p Unit, patch the attributes referencing it.
  void generateUnitLocations(CompileUnit &Unit, const DWARFFile &File,
                             ExpressionHandlerRef ExprHandler) const;

  /// Emit the accelerator entries for \p Unit.
  void emitAcceleratorEntriesForUnit(CompileUnit &Unit);

  /// Patch the frame info for an object file and emit it.
  void patchFrameInfoForObject(LinkContext &Context);

  /// FoldingSet that uniques the abbreviations.
  FoldingSet<DIEAbbrev> AbbreviationsSet;

  /// Storage for the unique Abbreviations.
  /// This is passed to AsmPrinter::emitDwarfAbbrevs(), thus it cannot be
  /// changed to a vector of unique_ptrs.
  std::vector<std::unique_ptr<DIEAbbrev>> Abbreviations;

  /// DIELoc objects that need to be destructed (but not freed!).
  std::vector<DIELoc *> DIELocs;

  /// DIEBlock objects that need to be destructed (but not freed!).
  std::vector<DIEBlock *> DIEBlocks;

  /// Allocator used for all the DIEValue objects.
  BumpPtrAllocator DIEAlloc;
  /// @}

  std::unique_ptr<DwarfStreamer> TheDwarfEmitter;
  std::vector<LinkContext> ObjectContexts;

  /// The CIEs that have been emitted in the output section. The actual CIE
  /// data serves a the key to this StringMap, this takes care of comparing the
  /// semantics of CIEs defined in different object files.
  StringMap<uint32_t> EmittedCIEs;

  /// Offset of the last CIE that has been emitted in the output
  /// .debug_frame section.
  uint32_t LastCIEOffset = 0;

  /// Apple accelerator tables.
  AccelTable<DWARF5AccelTableStaticData> DebugNames;
  AccelTable<AppleAccelTableStaticOffsetData> AppleNames;
  AccelTable<AppleAccelTableStaticOffsetData> AppleNamespaces;
  AccelTable<AppleAccelTableStaticOffsetData> AppleObjc;
  AccelTable<AppleAccelTableStaticTypeData> AppleTypes;

  /// Mapping the PCM filename to the DwoId.
  StringMap<uint64_t> ClangModules;

  DwarfLinkerClient DwarfLinkerClientID;

  std::function<StringRef(StringRef)> StringsTranslator = nullptr;

  /// A unique ID that identifies each compile unit.
  unsigned UniqueUnitID = 0;

  // error handler
  messageHandler ErrorHandler = nullptr;

  // warning handler
  messageHandler WarningHandler = nullptr;

  /// linking options
  struct DWARFLinkerOptions {
    /// DWARF version for the output.
    uint16_t TargetDWARFVersion = 0;

    /// Generate processing log to the standard output.
    bool Verbose = false;

    /// Print statistics.
    bool Statistics = false;

    /// Verify the input DWARF.
    bool VerifyInputDWARF = false;

    /// Do not unique types according to ODR
    bool NoODR = false;

    /// Update
    bool Update = false;

    /// Whether we want a static variable to force us to keep its enclosing
    /// function.
    bool KeepFunctionForStatic = false;

    /// Number of threads.
    unsigned Threads = 1;

    /// The accelerator table kinds
    SmallVector<AccelTableKind, 1> AccelTables;

    /// Prepend path for the clang modules.
    std::string PrependPath;

    // input verification handler
    inputVerificationHandler InputVerificationHandler = nullptr;

    /// A list of all .swiftinterface files referenced by the debug
    /// info, mapping Module name to path on disk. The entries need to
    /// be uniqued and sorted and there are only few entries expected
    /// per compile unit, which is why this is a std::map.
    /// this is dsymutil specific fag.
    swiftInterfacesMap *ParseableSwiftInterfaces = nullptr;

    /// A list of remappings to apply to file paths.
    objectPrefixMap *ObjectPrefixMap = nullptr;
  } Options;
};

} // end namespace llvm

#endif // LLVM_DWARFLINKER_DWARFLINKER_H
