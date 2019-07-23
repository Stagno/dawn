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
typename MutableArrayRef<T>::iterator
vectorIteratorToArrayRefIterator(const std::vector<T>& vector,
                                 typename std::vector<T>::const_iterator vectorIterator) {
  return vector.empty() ? nullptr
                        : const_cast<typename MutableArrayRef<T>::iterator>(
                              vector.data() + (vectorIterator - vector.begin()));
}

} // namespace

// TODO iir_restructuring ADD root = IFSTMT CASE
// Input contract: (node) iterator must be a top-level iterator and its root must be a BlockStmt
template <>
ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>
ASTOps::insertAfter(std::shared_ptr<Stmt> stmt,
                    ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>& node) {

  DAWN_ASSERT(node.getASTRoot()->getKind() == Stmt::SK_BlockStmt);
  DAWN_ASSERT(node.isTop());
  DAWN_ASSERT(stmt);
  std::vector<std::shared_ptr<Stmt>>& children =
      std::dynamic_pointer_cast<BlockStmt>(node.getASTRoot())->getStatements();

  if(node.isVoidIter() || node.isEnd()) {
    // Insert as last child
    children.push_back(stmt);

    // if iterator (node) is void it must be recreated as non-void
    if(node.isVoidIter())
      node = std::move(
          ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>::CreateInstance(
              node.getASTRoot()));

    // input iterator (node) must be kept at end
    node.setToEnd();

    // iterator to be returned must point to inserted element
    ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT> insertedNode(
        std::move(node.clone()));
    SubtreeIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>* insertedImpl =
        dynamic_cast<SubtreeIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>*>(
            insertedNode.impl_.get());
    insertedImpl->restIterator_ =
        std::unique_ptr<VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>>(
            new VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>(
                vectorIteratorToArrayRefIterator(children, children.end() - 1), false));

    return insertedNode;
  }
  // skipping case isVisitingRoot() = true, since it's a first-level only visit iterator
  SubtreeIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>* impl =
      dynamic_cast<SubtreeIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>*>(
          node.impl_.get());

  std::vector<std::shared_ptr<Stmt>>::const_iterator vecIt = ++arrayRefIteratorToVectorIterator(
      children, impl->restIterator_->rootStmtIt_,
      node.getASTRoot()
          ->getChildren()
          .begin()); // Should point to element that is after the position of insertion

  vecIt = children.insert(vecIt, stmt); // Returns iterator to inserted element

  // Fix iterator in input
  impl->restIterator_ =
      std::unique_ptr<VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>>(
          new VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>(
              vectorIteratorToArrayRefIterator(children, vecIt - 1), false));

  // iterator to be returned must point to inserted element
  ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT> insertedNode(node.clone());
  ++insertedNode;
  return insertedNode;
}

} // namespace dawn
