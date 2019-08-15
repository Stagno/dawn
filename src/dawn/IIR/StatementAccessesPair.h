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

#ifndef DAWN_IIR_STATEMENTACCESSESPAIR_H
#define DAWN_IIR_STATEMENTACCESSESPAIR_H

#include "dawn/IIR/AccessToNameMapper.h"
#include "dawn/IIR/Accesses.h"
#include "dawn/IIR/IIRNode.h"
#include "dawn/IIR/StatementAccessesPairIteratorDecl.h"
#include "dawn/SIR/ASTNodeIterator.h"
#include "dawn/SIR/Statement.h"
#include <boost/optional.hpp>
#include <memory>
#include <vector>

namespace dawn {

namespace iir {

class StencilMetaInformation;

/// @brief Statement with corresponding Accesses
///
/// If the statement is an If or Block statement, an iterator range through the sub-statements can
/// be retrieved through getBlockStatements() / getBlockStatementAccessesPairs()
/// @ingroup optimizer
class StatementAccessesPair {

  std::shared_ptr<Statement> statement_;

  // In case of a non function call stmt, the accesses are stored in callerAccesses_, while
  // calleeAccesses_ will be nullptr

  // Accesses of the statement. If the statement is part of a stencil-function, this will store
  // the caller accesses. The caller access will have the initial offset added (e.g if a stencil
  // function is called with `avg(u(i+1))` the initial offset of `u` is `[1, 0, 0]`).
  std::shared_ptr<Accesses> callerAccesses_;

  // If the statement is part of a stencil-function, this will store the callee accesses i.e the
  // accesses without the initial offset of the call
  std::shared_ptr<Accesses> calleeAccesses_;

public:
  static constexpr const char* name = "StatementAccessesPair";

  using ASTStmtToSAPMapType = std::unordered_map<const Stmt*, StatementAccessesPair>;

  explicit StatementAccessesPair(const std::shared_ptr<Statement>& statement);

  StatementAccessesPair(StatementAccessesPair&&) = default;
  StatementAccessesPair(const StatementAccessesPair&);
  StatementAccessesPair& operator=(StatementAccessesPair&&) = default;
  StatementAccessesPair& operator=(const StatementAccessesPair&);

  /// @brief clone the statement accesses pair, returning a smart ptr
  std::unique_ptr<StatementAccessesPair> clone() const;

  /// @brief Get/Set the statement
  std::shared_ptr<Statement> getStatement() const;
  void setStatement(const std::shared_ptr<Statement>& statement);

  /// @brief Get/Set the accesses
  std::shared_ptr<Accesses> getAccesses() const;
  void setAccesses(const std::shared_ptr<Accesses>& accesses);

  /// @brief Get/Set the caller accesses (alias for `getAccesses`)
  std::shared_ptr<Accesses> getCallerAccesses() const;
  void setCallerAccesses(const std::shared_ptr<Accesses>& accesses);

  /// @brief Get/Set the callee accesses (only set for statements inside stencil-functions)
  std::shared_ptr<Accesses> getCalleeAccesses() const;
  void setCalleeAccesses(const std::shared_ptr<Accesses>& accesses);
  bool hasCalleeAccesses();

  /// @brief Get the AST statements inside the block (only one level deep). IfStmt gives
  /// a concatenation of the condition statement, the then and else blocks.
  ASTRange<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT> getBlockStatements() const;
  /// @brief Get the AST StatementAccessesPairs of the statements inside the block (only one level
  /// deep). IfStmt gives a concatenation of the condition statement, the then and else blocks.
  StatementAccessesPairRange<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>
  getBlockStatementAccessesPairs(const ASTStmtToSAPMapType& astStmtToSAPMap) const;
  bool hasBlockStatements() const;

  boost::optional<Extents> computeMaximumExtents(const int accessID,
                                                 const ASTStmtToSAPMapType& astStmtToSAPMap) const;

  /// @brief Convert the StatementAccessesPair of a stencil or stencil-function to string
  /// @{
  std::string toString(const StencilMetaInformation* metadata,
                       const ASTStmtToSAPMapType& astStmtToSAPMap,
                       std::size_t initialIndent = 0) const;
  std::string toString(const StencilFunctionInstantiation* stencilFunc,
                       const ASTStmtToSAPMapType& astStmtToSAPMap,
                       std::size_t initialIndent = 0) const;
  /// @}

  json::json jsonDump(const StencilMetaInformation& metadata) const;
  json::json print(const StencilMetaInformation& metadata,
                   const AccessToNameMapper& accessToNameMapper,
                   const std::unordered_map<int, Extents>& accesses) const;
};

} // namespace iir
} // namespace dawn

#endif
