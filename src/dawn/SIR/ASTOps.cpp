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

#include "ASTOps.h"

namespace dawn {

namespace {
template <typename T>
typename std::vector<T>::const_iterator
arrayRefIteratorToVectorIterator(const std::vector<T>& vector,
                                 typename ArrayRef<T>::iterator arrayRefIterator,
                                 typename ArrayRef<T>::iterator beginArrayRefIterator) {
  typename std::vector<T>::const_iterator vecIt = vector.begin();
  return std::next(vecIt, arrayRefIterator - beginArrayRefIterator);
}

template <typename T>
typename ArrayRef<T>::iterator
vectorIteratorToArrayRefIterator(const std::vector<T>& vector,
                                 typename std::vector<T>::const_iterator vectorIterator) {
  return vector.empty() ? nullptr : vector.data() + (vectorIterator - vector.begin());
}

} // namespace

// Input contract: iterator root must be a BlockStmt
template <>
ASTNodeIterator<Stmt, true> ASTOps::insertAfter(std::shared_ptr<Stmt> stmt,
                                                ASTNodeIterator<Stmt, true>& node) {
  // TODO iir_restructuring ADD root = IFSTMT CASE
  // TODO thoroughly check correctness, warnings, etc.
  DAWN_ASSERT(node.getASTRoot().getKind() == Stmt::SK_BlockStmt);
  DAWN_ASSERT(stmt);
  std::vector<std::shared_ptr<Stmt>>& children =
      dynamic_cast<BlockStmt&>(node.getASTRoot()).getStatements();

  if(node.isVoidIter() || node.isEnd()) {
    // Insert as last child
    children.push_back(stmt);
    // input iterator (node) must be kept at end
    // unless it's void, in which case it must be recreated
    if(node.isVoidIter())
      node = std::move(ASTNodeIterator<Stmt, true>::CreateInstance(node.getASTRoot()));
    // iterator to be returned must point to inserted element
    ASTNodeIterator<Stmt, true> insertedNode(node.clone());
    insertedNode.childrenIterator_ = const_cast<Stmt::StmtRangeType::iterator>(
        vectorIteratorToArrayRefIterator(children, children.end() - 1));
    insertedNode.setupRestIterator();
    return insertedNode;
  }
  if(node.isVisitingRoot()) {
    // Insert as first child
    children.insert(children.begin(), stmt);
    // input iterator (node) must be kept at root
    // iterator to be returned must point to inserted element
    ASTNodeIterator<Stmt, true> insertedNode(node.clone());
    insertedNode.childrenIterator_ = const_cast<Stmt::StmtRangeType::iterator>(
        vectorIteratorToArrayRefIterator(children, children.begin()));
    insertedNode.setupRestIterator();
    return insertedNode;
  }

  std::vector<std::shared_ptr<Stmt>>::const_iterator vecIt = ++arrayRefIteratorToVectorIterator(
      children, node.childrenIterator_,
      node.rootStmt_.getChildren()
          .begin()); // Should point to element that is after the position of insertion
  vecIt = children.insert(vecIt, stmt); // Returns iterator to inserted element

  // Fix iterator in input
  node.childrenIterator_ = const_cast<Stmt::StmtRangeType::iterator>(
      vectorIteratorToArrayRefIterator(children, vecIt - 1));
  node.setupRestIterator();

  ASTNodeIterator<Stmt, true> insertedNode(node.clone()); // Clone the iterator
  // Fix it
  ++(insertedNode.childrenIterator_);
  insertedNode.setupRestIterator();
  return insertedNode;
}

} // namespace dawn
