//===--------------------------------------------------------------------------------*- C++ -*-===//
//                          _
//                         | |
//                       __| | __ ___      ___ ___
//                      / _` |/ _` \ \ /\ / / '_  |
//                     | (_| | (_| |\ V  V /| | | |
//                      \__,_|\__,_| \_/\_/ |_| |_| - Compiler Toolchain
//
//
//  This file is distributed under the MIT License (MIT).
//  See LICENSE.txt for details.
//
//===------------------------------------------------------------------------------------------===//

#ifndef DAWN_IIR_DOMETHOD_H
#define DAWN_IIR_DOMETHOD_H

#include "dawn/IIR/Field.h"
#include "dawn/IIR/IIRNode.h"
#include "dawn/IIR/Interval.h"
#include "dawn/IIR/StatementAccessesPair.h"
#include "dawn/IIR/StatementAccessesPairIterator.h"
#include "dawn/SIR/ASTNodeIterator.h"
#include "dawn/Support/IteratorRange.h"
#include <boost/optional.hpp>
#include <memory>
#include <vector>

namespace dawn {
namespace iir {

class Stage;
class DependencyGraphAccesses;
class StencilMetaInformation;

/// @brief A Do-method is a collection of Statements with corresponding Accesses of a specific
/// vertical region
///
/// @ingroup optimizer
class DoMethod : public IIRNode<Stage, DoMethod, void> {

public:
  static constexpr const char* name = "DoMethod";

  using ASTStmtToSAPMapType = std::unordered_map<const Stmt*, StatementAccessesPair>;
  using FullASTIterator = ASTNodeIterator<ASTNodeIteratorVisitKind::FULL_AST_VISIT>;
  using FullASTConstIterator = ASTNodeConstIterator<ASTNodeIteratorVisitKind::FULL_AST_VISIT>;
  using FullASTRange = ASTRange<ASTNodeIteratorVisitKind::FULL_AST_VISIT>;
  using FullASTConstRange = ASTConstRange<ASTNodeIteratorVisitKind::FULL_AST_VISIT>;
  using StmtsIterator = ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>;
  using StmtsConstIterator = ASTNodeConstIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>;
  using StmtsRange = ASTRange<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>;
  using StmtsConstRange = ASTConstRange<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>;
  using SAPIterator =
      StatementAccessesPairIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>;
  using SAPConstIterator =
      StatementAccessesPairConstIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>;
  using SAPRange = StatementAccessesPairRange<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>;
  using SAPConstRange =
      StatementAccessesPairConstRange<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>;

  /// @name Constructors and Assignment
  /// @{

  /// @brief Constructor for empty DoMethod
  DoMethod(Interval interval, StencilMetaInformation& metaData);

  // TODO iir_restructuring whether the domethod is of a function or of a stencil it should be
  // transparent here. Also this constructor leaves the DoMethod in an inconsistent state! Need
  // refactoring of StatementMapper!
  /// @brief Constructor for function's DoMethod
  DoMethod(Interval interval, StencilMetaInformation& metaData, const std::shared_ptr<AST> ast);

  /// @brief Constructor for stencil's DoMethod filled with statements (in the iterator range) from
  /// another DoMethod.
  DoMethod(Interval interval, StencilMetaInformation& metaData, IteratorRange<SAPIterator> saps);
  /// @brief Constructor for stencil's DoMethod with AST (also creates necessary AccessIDs and
  /// computes accesses)
  DoMethod(Interval interval, const std::shared_ptr<AST> ast, const std::shared_ptr<SIR>& fullSIR,
           const std::shared_ptr<iir::StencilInstantiation>& instantiation,
           const std::shared_ptr<std::vector<sir::StencilCall*>>& stackTrace,
           const std::unordered_map<std::string, int>& localFieldnameToAccessIDMap);
  DoMethod(DoMethod&&) = default;
  DoMethod& operator=(DoMethod&&) = default;
  /// @}

  ~DoMethod();

  json::json jsonDump(const StencilMetaInformation& metaData) const;

  /// @brief clone the object creating and returning a new unique_ptr
  std::unique_ptr<DoMethod> clone() const;

  /// @name Iterators
  /// /// @{
  /// @brief returns a depth-first pre-order iterator through the whole AST of the DoMethod
  /// pointing to root
  inline FullASTIterator astBegin() { return FullASTRange(ast_->getRoot()).begin(); }
  inline FullASTConstIterator astConstBegin() const {
    return FullASTConstRange(ast_->getRoot()).begin();
  }
  /// @brief returns a depth-first pre-order iterator through the whole AST of the DoMethod
  /// pointing to the end (after the last statement of the tree)
  inline FullASTIterator astEnd() { return FullASTRange(ast_->getRoot()).end(); }
  inline FullASTConstIterator astConstEnd() const {
    return FullASTConstRange(ast_->getRoot()).end();
  }
  /// @brief returns a depth-first pre-order iterator range through the whole AST of the DoMethod
  inline FullASTRange astRange() { return FullASTRange(ast_->getRoot()); }
  inline FullASTConstRange astConstRange() const { return FullASTConstRange(ast_->getRoot()); }
  /// @brief returns a iterator through the statements of the first level of the AST of
  /// the DoMethod pointing to the first one
  inline StmtsIterator stmtsBegin() { return StmtsRange(ast_->getRoot()).begin(); }
  inline StmtsConstIterator stmtsConstBegin() const {
    return StmtsConstRange(ast_->getRoot()).begin();
  }
  /// @brief returns a iterator through the statements of the first level of the AST of
  /// the DoMethod pointing to the end (after the last statement)
  inline StmtsIterator stmtsEnd() { return StmtsRange(ast_->getRoot()).end(); }
  inline StmtsConstIterator stmtsConstEnd() const { return StmtsConstRange(ast_->getRoot()).end(); }
  /// @brief returns an iterator range through the statements of the first level of the AST of the
  /// DoMethod
  inline StmtsRange stmtsRange() { return StmtsRange(ast_->getRoot()); }
  inline StmtsConstRange stmtsConstRange() const { return StmtsConstRange(ast_->getRoot()); }
  /// @brief returns an iterator through the StatementAccessesPairs of the statements of the
  /// first level of the AST of the DoMethod pointing to the first one
  inline SAPIterator sapBegin() { return SAPRange(ast_->getRoot(), ASTStmtToSAPMap_).begin(); }
  inline SAPConstIterator sapConstBegin() const {
    return SAPConstRange(ast_->getRoot(), ASTStmtToSAPMap_).begin();
  }
  /// @brief returns an iterator through the StatementAccessesPairs of the statements of the
  /// first level of the AST of the DoMethod pointing to the end (after the last statement)
  inline SAPIterator sapEnd() { return SAPRange(ast_->getRoot(), ASTStmtToSAPMap_).end(); }
  inline SAPConstIterator sapConstEnd() const {
    return SAPConstRange(ast_->getRoot(), ASTStmtToSAPMap_).end();
  }
  /// @brief returns an iterator range through the StatementAccessesPairs of the statements of the
  /// first level of the AST of the DoMethod
  inline SAPRange sapRange() { return SAPRange(ast_->getRoot(), ASTStmtToSAPMap_); }
  inline SAPConstRange sapConstRange() const {
    return SAPConstRange(ast_->getRoot(), ASTStmtToSAPMap_);
  }
  /// @}

  /// @name Getters
  /// @{
  Interval& getInterval();
  const Interval& getInterval() const;
  inline unsigned long int getID() const { return id_; }
  const std::shared_ptr<DependencyGraphAccesses>& getDependencyGraph() const;
  inline const std::vector<std::shared_ptr<Stmt>>& getStmts() const {
    return ast_->getRoot()->getStatements();
  }
  /// @brief returns a reference to the internal map from ASTStmt to StatementAccessesPair
  inline const ASTStmtToSAPMapType& getASTStmtToSAPMap() const { return ASTStmtToSAPMap_; }

  inline const iir::StatementAccessesPair&
  getStatementAccessesPairFromStmt(const Stmt* stmt) const {
    return ASTStmtToSAPMap_.at(stmt);
  }
  /// @}

  /// @name Setters
  /// @{

  void setInterval(Interval const& interval);
  void setID(const long unsigned int id) { id_ = id; }
  void setDependencyGraph(const std::shared_ptr<DependencyGraphAccesses>& DG);
  /// @}
  ///  @brief Fills the DoMethod with the provided AST (also creates necessary AccessIDs and
  ///  computes accesses)
  void fillWithAST(const std::shared_ptr<AST> ast, const std::shared_ptr<SIR>& fullSIR,
                   const std::shared_ptr<iir::StencilInstantiation>& instantiation,
                   const std::shared_ptr<std::vector<sir::StencilCall*>>& stackTrace,
                   const std::unordered_map<std::string, int>& localFieldnameToAccessIDMap);
  // TODO iir_restructuring append AST
  // TODO iir_restructuring implement
  // insertStatements (in any position) with iterators
  /// @brief inserts a statement (pointed by the input iterator) and its accesses info at the end of
  /// the method
  void appendStatement(SAPIterator sap);
  /// @brief inserts statements (pointed by the iterator range) and their accesses info at the end
  /// of the method
  void appendStatements(IteratorRange<SAPIterator> saps);

  /// @brief moves n consecutive statements (starting from iterator 'from') and their accesses info
  /// before statement pointed by 'to'. Iterators 'from' and 'to' should be rooted at the roots of
  /// DoMethods (respectively) 'originDoMethod' and 'destinationDoMethod', which should not be the
  /// same.
  static void moveStatementsBefore(StmtsIterator from, DoMethod& originDoMethod, StmtsIterator to,
                                   DoMethod& destinationDoMethod, int n);

  virtual void clearDerivedInfo() override;

  /// @brief computes the maximum extent among all the accesses of accessID
  boost::optional<Extents> computeMaximumExtents(const int accessID) const;

  /// @brief true if it is empty
  bool isEmptyOrNullStmt() const;

  /// @param accessID accessID for which the enclosing interval is computed
  /// @param mergeWidhDoInterval determines if the extent of the access is merged with the interval
  /// of the do method.
  /// Example:
  ///    do(kstart+2,kend) return u[k+1]
  /// will return Interval{3,kend+1} if mergeWithDoInterval is false
  /// will return Interval{2,kend+1} (which is Interval{3,kend+1}.merge(Interval{2,kend})) if
  /// mergeWithDoInterval is true
  boost::optional<Interval> computeEnclosingAccessInterval(const int accessID,
                                                           const bool mergeWithDoInterval) const;

  /// @brief Get fields of this stage sorted according their Intend: `Output` -> `IntputOutput` ->
  /// `Input`
  ///
  /// The fields are computed during `DoMethod::update`.
  const std::unordered_map<int, Field>& getFields() const { return derivedInfo_.fields_; }

  bool hasField(int accessID) const { return derivedInfo_.fields_.count(accessID); }

  /// @brief field getter
  const Field& getField(const int accessID) const {
    DAWN_ASSERT(derivedInfo_.fields_.count(accessID));
    return derivedInfo_.fields_.at(accessID);
  }

  /// @brief Update the fields and global variables
  ///
  /// This recomputes the fields referenced in this Do-Method and computes
  /// the @b accumulated extent of each field
  virtual void updateLevel() override;

private:
  StmtsIterator insertStatementAfterImpl(SAPIterator otherSapIt, StmtsIterator& insertionPointIt);
  static SAPIterator moveStatementBeforeImpl(StmtsIterator& from, DoMethod& originDoMethod,
                                             StmtsIterator& to, DoMethod& destinationDoMethod);

  Interval interval_;
  long unsigned int id_;

  struct DerivedInfo {
    DerivedInfo() : dependencyGraph_(nullptr) {}
    DerivedInfo(DerivedInfo&&) = default;
    DerivedInfo(const DerivedInfo&) = default;
    DerivedInfo& operator=(DerivedInfo&&) = default;
    DerivedInfo& operator=(const DerivedInfo&) = default;

    void clear();

    /// Declaration of the fields of this doMethod
    std::unordered_map<int, Field> fields_;
    std::shared_ptr<DependencyGraphAccesses> dependencyGraph_;
  };

  StencilMetaInformation& metaData_;
  const std::shared_ptr<AST> ast_;
  ASTStmtToSAPMapType ASTStmtToSAPMap_;
  DerivedInfo derivedInfo_;
};

} // namespace iir
} // namespace dawn

#endif
