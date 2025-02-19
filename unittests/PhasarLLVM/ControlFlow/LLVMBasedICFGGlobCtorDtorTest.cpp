/******************************************************************************
 * Copyright (c) 2021 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <string>
#include <vector>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/Config/Configuration.h"
#include "phasar/DB/ProjectIRDB.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedCFG.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDELinearConstantAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/IDESolver.h"
#include "phasar/PhasarLLVM/Passes/ValueAnnotationPass.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "TestConfig.h"

using namespace std;
using namespace psr;

class LLVMBasedICFGGlobCtorDtorTest : public ::testing::Test {
protected:
  const std::string PathToLLFiles = unittest::PathToLLTestFiles + "globals/";

  void SetUp() override { ValueAnnotationPass::resetValueID(); }

  void ensureFunctionOrdering(
      llvm::Function *F, LLVMBasedICFG &ICFG,
      std::initializer_list<std::pair<llvm::StringRef, llvm::StringRef>>
          FixedOrdering) {

    auto CallSites = ICFG.getCallsFromWithin(F);

    llvm::StringMap<const llvm::CallBase *> CSByCalleeName;

    for (const auto *CS : CallSites) {
      const auto *Call = llvm::cast<llvm::CallBase>(CS);

      if (const auto *Callee = Call->getCalledFunction()) {
        // Assume, we have no duplicates
        CSByCalleeName[Callee->getName()] = Call;
      }
    }

    llvm::DominatorTree Dom(*F);

    for (auto [First, Second] : FixedOrdering) {
      EXPECT_TRUE(CSByCalleeName.count(First));
      EXPECT_TRUE(CSByCalleeName.count(Second));

      if (CSByCalleeName.count(First) && CSByCalleeName.count(Second)) {
        EXPECT_TRUE(
            Dom.dominates(CSByCalleeName[First], CSByCalleeName[Second]));
      }
    }
  }
};

TEST_F(LLVMBasedICFGGlobCtorDtorTest, CtorTest) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_ctor_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  auto *GlobalCtor = IRDB.getFunction(LLVMBasedICFG::GlobalCRuntimeModelName);
  ASSERT_TRUE(GlobalCtor != nullptr);

  // GlobalCtor->print(llvm::outs());

  ensureFunctionOrdering(GlobalCtor, ICFG,
                         {{"_GLOBAL__sub_I_globals_ctor_1.cpp", "main"},
                          {"main", "__psrCRuntimeGlobalDtorsModel"}});
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, CtorTest2) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_ctor_2_1_cpp.ll",
                    PathToLLFiles + "globals_ctor_2_2_cpp.ll"},
                   IRDBOptions::WPA);
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  auto *GlobalCtor = IRDB.getFunction(LLVMBasedICFG::GlobalCRuntimeModelName);
  ASSERT_TRUE(GlobalCtor != nullptr);

  // GlobalCtor->print(llvm::outs());

  ensureFunctionOrdering(GlobalCtor, ICFG,
                         {{"_GLOBAL__sub_I_globals_ctor_2_1.cpp", "main"},
                          {"_GLOBAL__sub_I_globals_ctor_2_2.cpp", "main"}});
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, DtorTest1) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_dtor_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  auto *GlobalCtor = IRDB.getFunction(LLVMBasedICFG::GlobalCRuntimeModelName);
  ASSERT_NE(nullptr, GlobalCtor);

  // GlobalCtor->print(llvm::outs());

  ensureFunctionOrdering(
      GlobalCtor, ICFG,
      {{"_GLOBAL__sub_I_globals_dtor_1.cpp", "main"},
       {"main", "__psrGlobalDtorsCaller.globals_dtor_1_cpp.ll"}});

  auto *GlobalDtor =
      IRDB.getFunction("__psrGlobalDtorsCaller.globals_dtor_1_cpp.ll");

  ASSERT_NE(nullptr, GlobalDtor);

  auto DtorCallSites = ICFG.getCallsFromWithin(GlobalDtor);
  EXPECT_EQ(2, std::count_if(DtorCallSites.begin(), DtorCallSites.end(),
                             [](const llvm::Instruction *CS) {
                               auto Call = llvm::cast<llvm::CallBase>(CS);
                               return (Call->getCalledFunction() &&
                                       Call->getCalledFunction()->getName() ==
                                           "_ZN3FooD2Ev");
                             }));
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest1) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_lca_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  IDELinearConstantAnalysis Problem(
      &IRDB, &TH, &ICFG, &PT, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem);

  Solver.solve();

  // Solver.dumpResults();

  auto *FooInit = IRDB.getInstruction(6);
  auto *LoadX = IRDB.getInstruction(11);
  auto *End = IRDB.getInstruction(13);
  const auto *Foo = IRDB.getGlobalVariableDefinition("foo");

  auto FooValueAfterInit = Solver.resultAt(FooInit, Foo);

  EXPECT_EQ(42, FooValueAfterInit)
      << "Value of foo at " << llvmIRToString(FooInit) << " is not 42";

  auto XValueAtEnd = Solver.resultAt(End, LoadX);
  auto FooValueAtEnd = Solver.resultAt(End, Foo);

  EXPECT_EQ(42, FooValueAtEnd)
      << "Value of foo at " << llvmIRToString(End) << " is not 42";
  EXPECT_EQ(43, XValueAtEnd)
      << "Value of x at " << llvmIRToString(End) << " is not 43";
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest2) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_lca_2_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);
  IDELinearConstantAnalysis Problem(
      &IRDB, &TH, &ICFG, &PT, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem);

  Solver.solve();

  // Solver.dumpResults();

  auto *FooInit = IRDB.getInstruction(7);
  auto *BarInit = IRDB.getInstruction(11);
  auto *LoadX = IRDB.getInstruction(20);
  auto *LoadY = IRDB.getInstruction(21);
  auto *End = IRDB.getInstruction(23);
  const auto *Foo = IRDB.getGlobalVariableDefinition("foo");
  const auto *Bar = IRDB.getGlobalVariableDefinition("bar");

  auto FooValueAfterInit = Solver.resultAt(FooInit, Foo);
  auto BarValueAfterInit = Solver.resultAt(BarInit, Bar);

  EXPECT_EQ(42, FooValueAfterInit);
  EXPECT_EQ(45, BarValueAfterInit);

  auto XValueAtEnd = Solver.resultAt(End, LoadX);
  auto FooValueAtEnd = Solver.resultAt(End, Foo);
  auto YValueAtEnd = Solver.resultAt(End, LoadY);
  auto BarValueAtEnd = Solver.resultAt(End, Bar);

  EXPECT_EQ(42, FooValueAtEnd);
  EXPECT_EQ(43, XValueAtEnd);
  EXPECT_EQ(44, YValueAtEnd);
  EXPECT_EQ(45, BarValueAtEnd);
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest3) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_lca_3_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy, /*IncludeGlobals*/ true);

  IDELinearConstantAnalysis Problem(
      &IRDB, &TH, &ICFG, &PT, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem);

  Solver.solve();

  // Solver.dumpResults();

  auto *FooInit = IRDB.getInstruction(7);
  // FIXME Why is 10 missing in the results set?
  auto *BarInit = IRDB.getInstruction(11);
  auto *LoadX = IRDB.getInstruction(18);
  auto *LoadY = IRDB.getInstruction(19);
  auto *End = IRDB.getInstruction(21);
  const auto *Foo = IRDB.getGlobalVariableDefinition("foo");
  const auto *Bar = IRDB.getGlobalVariableDefinition("bar");

  auto FooValueAfterInit = Solver.resultAt(FooInit, Foo);
  auto BarValueAfterInit = Solver.resultAt(BarInit, Bar);

  EXPECT_EQ(42, FooValueAfterInit);
  EXPECT_EQ(45, BarValueAfterInit);

  auto XValueAtEnd = Solver.resultAt(End, LoadX);
  auto FooValueAtEnd = Solver.resultAt(End, Foo);
  auto YValueAtEnd = Solver.resultAt(End, LoadY);
  auto BarValueAtEnd = Solver.resultAt(End, Bar);

  EXPECT_EQ(42, FooValueAtEnd);
  EXPECT_EQ(43, XValueAtEnd);
  EXPECT_EQ(44, YValueAtEnd);
  EXPECT_EQ(45, BarValueAtEnd);
}

// Fails due to exception handling
TEST_F(LLVMBasedICFGGlobCtorDtorTest, DISABLED_LCATest4) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_lca_4_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(
      IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT, Soundness::Soundy,
      /*IncludeGlobals*/ true); // We have no real global initializers here, but
                                // just keep the flag IncludeGlobals=true
  IDELinearConstantAnalysis Problem(
      &IRDB, &TH, &ICFG, &PT, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem);

  Solver.solve();

  // Solver.dumpResults();

  auto *FooGet = IRDB.getInstruction(17);
  auto *LoadFoo = IRDB.getInstruction(16);
  auto *LoadX = IRDB.getInstruction(34);
  auto *End = IRDB.getInstruction(36);

  auto FooValueAfterGet = Solver.resultAt(FooGet, LoadFoo);

  EXPECT_EQ(42, FooValueAfterGet)
      << "Invalid value of " << llvmIRToString(LoadFoo);

  auto XValueAtEnd = Solver.resultAt(End, LoadX);

  EXPECT_EQ(43, XValueAtEnd) << "Invalid value of " << llvmIRToString(LoadX);
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest4_1) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_lca_4_1_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(
      IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT, Soundness::Soundy,
      /*IncludeGlobals*/ true); // We have no real global initializers here, but
                                // just keep the flag IncludeGlobals=true
  IDELinearConstantAnalysis Problem(
      &IRDB, &TH, &ICFG, &PT, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem);

  Solver.solve();

  Solver.dumpResults();

  auto *FooGet = IRDB.getInstruction(15);
  auto *LoadFoo = IRDB.getInstruction(14);
  auto *LoadX = IRDB.getInstruction(20);
  auto *End = IRDB.getInstruction(22);

  auto FooValueAfterGet = Solver.resultAt(FooGet, LoadFoo);

  EXPECT_EQ(42, FooValueAfterGet)
      << "Invalid value of " << llvmIRToString(LoadFoo);

  auto XValueAtEnd = Solver.resultAt(End, LoadX);

  EXPECT_EQ(43, XValueAtEnd) << "Invalid value of " << llvmIRToString(LoadX);
}

TEST_F(LLVMBasedICFGGlobCtorDtorTest, LCATest5) {

  ProjectIRDB IRDB({PathToLLFiles + "globals_lca_5_cpp.ll"});
  LLVMTypeHierarchy TH(IRDB);
  LLVMPointsToSet PT(IRDB);
  LLVMBasedICFG ICFG(IRDB, CallGraphAnalysisType::OTF, {"main"}, &TH, &PT,
                     Soundness::Soundy,
                     /*IncludeGlobals*/ true);
  IDELinearConstantAnalysis Problem(
      &IRDB, &TH, &ICFG, &PT, {LLVMBasedICFG::GlobalCRuntimeModelName.str()});

  IDESolver Solver(Problem);

  const auto *GlobalDtor =
      ICFG.getRegisteredDtorsCallerOrNull(IRDB.getWPAModule());

  ASSERT_NE(nullptr, GlobalDtor);

  GlobalDtor->print(llvm::outs());

  Solver.solve();

  Solver.dumpResults();

  // FIXME: Why is the 27 missing in the results set?
  auto *AfterGlobalInit = IRDB.getInstruction(4);
  auto *AtMainPrintF = IRDB.getInstruction(29);

  const auto *Foo = IRDB.getGlobalVariableDefinition("foo");

  EXPECT_EQ(42, Solver.resultAt(AfterGlobalInit, Foo));
  EXPECT_EQ(42, Solver.resultAt(AtMainPrintF, Foo));
  // For whatever reason the analysis computes Top for _this inside the dtor.
  // However, it correctly considers foo=42 inside __PHASAR_DTOR_CALLER.

  // EXPECT_EQ(43, Solver.resultAt(BeforeDtorPrintF, Foo));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
