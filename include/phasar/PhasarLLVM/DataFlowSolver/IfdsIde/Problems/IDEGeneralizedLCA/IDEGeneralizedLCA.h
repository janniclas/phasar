/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_IDEGENERALIZEDLCA_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDEGENERALIZEDLCA_IDEGENERALIZEDLCA_H

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IDEGeneralizedLCA/EdgeValueSet.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/Printer.h"

namespace psr {
/// \brief An implementation of a linear constant analysis, similar to
/// IDELinearConstantAnalysis, but with an extended edge-Value
/// domain. Instead of using single Values, we use a bounded set of cadidates to
/// increase precision.

struct IDEGeneralizedLCADomain : LLVMAnalysisDomainDefault {
  using l_t = EdgeValueSet;
};

// Forward declare the IDETabulationProblem as we require its toString
// functionality.
template <typename AnalysisDomainTy, typename Container>
class IDETabulationProblem;

class IDEGeneralizedLCA : public IDETabulationProblem<IDEGeneralizedLCADomain> {
  size_t MaxSetSize;

public:
  using d_t = typename IDEGeneralizedLCADomain::d_t;
  using f_t = typename IDEGeneralizedLCADomain::f_t;
  using i_t = typename IDEGeneralizedLCADomain::i_t;
  using l_t = typename IDEGeneralizedLCADomain::l_t;
  using n_t = typename IDEGeneralizedLCADomain::n_t;
  using t_t = typename IDEGeneralizedLCADomain::t_t;
  using v_t = typename IDEGeneralizedLCADomain::v_t;

  struct LCAResult {
    LCAResult() = default;
    unsigned LineNo = 0;
    std::string SrcNode;
    std::map<std::string, l_t> VariableToValue;
    std::vector<n_t> IRTrace;
    void print(llvm::raw_ostream &OS);
  };

  using lca_results_t = std::map<std::string, std::map<unsigned, LCAResult>>;

  IDEGeneralizedLCA(
      const ProjectIRDB *IRDB,
      const TypeHierarchy<const llvm::StructType *, const llvm::Function *> *TH,
      const LLVMBasedICFG *ICF,
      PointsToInfo<const llvm::Value *, const llvm::Instruction *> *PT,
      std::set<std::string> EntryPoints, size_t MaxSetSize);

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t Curr,
                                                           n_t Succ) override;

  std::shared_ptr<FlowFunction<d_t>> getCallFlowFunction(n_t CallStmt,
                                                         f_t DestMthd) override;

  std::shared_ptr<FlowFunction<d_t>> getRetFlowFunction(n_t CallSite,
                                                        f_t CalleeMthd,
                                                        n_t ExitStmt,
                                                        n_t RetSite) override;

  std::shared_ptr<FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           std::set<f_t> Callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t CallStmt, f_t DestMthd) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const override;

  [[nodiscard]] bool isZeroValue(d_t Fact) const override;

  // in addition provide specifications for the IDE parts

  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                        d_t SuccNode) override;

  std::shared_ptr<EdgeFunction<l_t>> getCallEdgeFunction(n_t CallStmt,
                                                         d_t SrcNode,
                                                         f_t DestinationMethod,
                                                         d_t DestNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t CallSite, f_t CalleeMethod, n_t ExitStmt,
                        d_t ExitNode, n_t RetSite, d_t RetNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode, std::set<f_t> Callees) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t CallStmt, d_t CallNode, n_t RetSite,
                         d_t RetSiteNode) override;

  l_t topElement() override;

  l_t bottomElement() override;

  l_t join(l_t Lhs, l_t Rhs) override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

  void printNode(llvm::raw_ostream &OS, n_t Stmt) const override;

  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;

  void printFunction(llvm::raw_ostream &OS, f_t Func) const override;

  void printEdgeFact(llvm::raw_ostream &OS, l_t L) const override;

  // void printIDEReport(llvm::raw_ostream &OS,
  // SolverResults<n_t, d_t, l_t> &SR) override;
  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      llvm::raw_ostream &OS) override;

  lca_results_t getLCAResults(SolverResults<n_t, d_t, l_t> SR);

private:
  void stripBottomResults(std::unordered_map<d_t, l_t> &Res);
  [[nodiscard]] bool isEntryPoint(const std::string &Name) const;
  template <typename V> std::string VtoString(V Val); // NOLINT
  bool isStringConstructor(const llvm::Function *Func);
};

} // namespace psr

#endif
