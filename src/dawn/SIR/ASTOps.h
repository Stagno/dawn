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

#ifndef DAWN_SIR_ASTASTOPS_H
#define DAWN_SIR_ASTASTOPS_H

#include "ASTNodeIterator.h"

namespace dawn {
class ASTOps {
  enum InsertionKind { BEFORE_INPUT_NODE, AFTER_INPUT_NODE };

public:
  /// @brief Inserts stmt after node pointed by iterator 'node'. Keeps the inout 'node' iterator
  /// pointing to the same stmt (whether it pointed to a node or to end()), fixing it to be valid
  /// after the operation. Returns a valid iterator to the inserted node.
  /// Input contract: 'node'
  /// iterator must be a top-level iterator and its root must be a BlockStmt.
  template <ASTNodeIteratorVisitKind onlyFirstLevel>
  static ASTNodeIterator<onlyFirstLevel> insertAfter(std::shared_ptr<Stmt> stmt,
                                                     ASTNodeIterator<onlyFirstLevel>& node);
  /// @brief Inserts stmt before node pointed by iterator 'node'. Keeps the inout 'node' iterator
  /// pointing to the same stmt (whether it pointed to a node or to end()), fixing it to be valid
  /// after the operation. Returns a valid iterator to the inserted node.
  /// Input contract: 'node'
  /// iterator must be a top-level iterator and its root must be a BlockStmt.
  template <ASTNodeIteratorVisitKind onlyFirstLevel>
  static ASTNodeIterator<onlyFirstLevel> insertBefore(std::shared_ptr<Stmt> stmt,
                                                      ASTNodeIterator<onlyFirstLevel>& node);
  /// @brief Removes (from the tree) stmt pointed by 'node' iterator and returns it. 'node' iterator
  /// is fixed to be valid and to point to the node that followed the pruned one (or to end()).
  /// Input contract: 'node' iterator must be a top-level iterator, its root must be a BlockStmt and
  /// it must point to a stmt.
  template <ASTNodeIteratorVisitKind onlyFirstLevel>
  static std::shared_ptr<Stmt> prune(ASTNodeIterator<onlyFirstLevel>& node);
  /// @brief Moves stmt pointed by iterator 'from' before node pointed by iterator 'to' and returns
  /// a pointer to the destination position where the stmt has been moved to. Iterator 'from' is
  /// fixed to be valid and to point to the node that followed the source position. Iterator 'to' is
  /// fixed to be valid and to point to the same stmt as before the operation.
  /// Input contract:
  /// iterators must be top-level iterators and their roots must be two different BlockStmts. 'from'
  /// iterator must point to a stmt.
  template <ASTNodeIteratorVisitKind onlyFirstLevel>
  static ASTNodeIterator<onlyFirstLevel> moveBefore(ASTNodeIterator<onlyFirstLevel>& from,
                                                    ASTNodeIterator<onlyFirstLevel>& to);

private:
  static ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>
  insertOnlyFirstLevelImpl(std::shared_ptr<Stmt> stmt,
                           ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>& node,
                           InsertionKind insertionKind);
};
} // namespace dawn

#endif
