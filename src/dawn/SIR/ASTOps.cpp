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
template <>
ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>
ASTOps::insertAfter(std::shared_ptr<Stmt> stmt,
                    ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>& node) {
  return insertOnlyFirstLevelImpl(stmt, node, InsertionKind::AFTER_INPUT_NODE);
}

// TODO iir_restructuring ADD root = IFSTMT CASE
template <>
ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>
ASTOps::insertBefore(std::shared_ptr<Stmt> stmt,
                     ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>& node) {
  return insertOnlyFirstLevelImpl(stmt, node, InsertionKind::BEFORE_INPUT_NODE);
}

ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT> ASTOps::insertOnlyFirstLevelImpl(
    std::shared_ptr<Stmt> stmt,
    ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>& node,
    InsertionKind insertionKind) {
  if(node.getASTRoot()->getKind() != Stmt::SK_BlockStmt)
    throw std::runtime_error(
        std::string("ASTOps::") +
        (insertionKind == InsertionKind::AFTER_INPUT_NODE ? "insertAfter" : "insertBefore") +
        " called with 'node' iterator whose root is not a 'BlockStmt'");
  if(!node.isTop())
    throw std::runtime_error(
        std::string("ASTOps::") +
        (insertionKind == InsertionKind::AFTER_INPUT_NODE ? "insertAfter" : "insertBefore") +
        " called with 'node' iterator which is not top-level");
  DAWN_ASSERT(stmt);
  std::vector<std::shared_ptr<Stmt>>& children =
      std::dynamic_pointer_cast<BlockStmt>(node.getASTRoot())->getStatements();

  if(node.isVoidIter() || node.isEnd()) {
    // Insert as last child
    children.push_back(stmt);

    // if iterator (node) is void it must be recreated as non-void
    if(node.isVoidIter())
      node = std::move(
          makeASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>(node.getASTRoot()));

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

  // vecIt should point to element that is after the position of insertion
  std::vector<std::shared_ptr<Stmt>>::const_iterator vecIt = arrayRefIteratorToVectorIterator(
      children, impl->restIterator_->rootStmtIt_, node.getASTRoot()->getChildren().begin());

  if(insertionKind == InsertionKind::AFTER_INPUT_NODE)
    ++vecIt;

  // Insert among the children
  vecIt = children.insert(vecIt, stmt); // Returns iterator to inserted element

  // Set iterator in input to point to: the inserted node for BEFORE_INPUT_NODE insertion kind, the
  // node before the inserted one for AFTER_INPUT_NODE insertion kind.
  impl->restIterator_ =
      std::unique_ptr<VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>>(
          new VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>(
              vectorIteratorToArrayRefIterator(
                  children, (insertionKind == InsertionKind::AFTER_INPUT_NODE ? vecIt - 1 : vecIt)),
              false));

  ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT> insertedNode(node.clone());

  if(insertionKind == InsertionKind::AFTER_INPUT_NODE)
    ++insertedNode; // Iterator to be returned must point to inserted node
  else
    ++node; // Fix iterator in input to point to the node after the inserted one
  return insertedNode;
}

// TODO iir_restructuring ADD root = IFSTMT CASE
template <>
std::shared_ptr<Stmt>
ASTOps::prune(ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>& node) {
  if(node.getASTRoot()->getKind() != Stmt::SK_BlockStmt)
    throw std::runtime_error("ASTOps::prune called with iterator whose root is not a 'BlockStmt'");
  if(!node.isTop())
    throw std::runtime_error("ASTOps::prune called with iterator which is not top-level");
  if(node.isVoidIter() || node.isEnd())
    throw std::runtime_error(
        "ASTOps::prune called with iterator which doesn't point to a statement");

  std::vector<std::shared_ptr<Stmt>>& children =
      std::dynamic_pointer_cast<BlockStmt>(node.getASTRoot())->getStatements();

  SubtreeIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>* impl =
      dynamic_cast<SubtreeIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>*>(
          node.impl_.get());
  // vecIt should point to the element to be pruned
  std::vector<std::shared_ptr<Stmt>>::const_iterator vecIt = arrayRefIteratorToVectorIterator(
      children, impl->restIterator_->rootStmtIt_, node.getASTRoot()->getChildren().begin());
  // save the element to be pruned to return it later
  std::shared_ptr<Stmt> prunedStmt = *vecIt;
  // remove the element from the children
  vecIt = children.erase(vecIt);
  // set 'node' iterator to point to the element after the removed one
  impl->restIterator_ =
      std::unique_ptr<VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>>(
          new VoidIteratorImpl<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>(
              vectorIteratorToArrayRefIterator(children, vecIt), false));

  return prunedStmt;
}

// TODO iir_restructuring ADD root = IFSTMT CASE
template <>
ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>
ASTOps::moveBefore(ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>& from,
                   ASTNodeIterator<ASTNodeIteratorVisitKind::ONLY_FIRST_LEVEL_VISIT>& to) {
  if(from.getASTRoot()->getKind() != Stmt::SK_BlockStmt ||
     to.getASTRoot()->getKind() != Stmt::SK_BlockStmt)
    throw std::runtime_error(
        "ASTOps::moveBefore called with iterator whose root is not a 'BlockStmt'");
  if(!from.isTop() || !to.isTop())
    throw std::runtime_error("ASTOps::moveBefore called with iterator which is not top-level");
  if(from.isVoidIter() || from.isEnd())
    throw std::runtime_error(
        "ASTOps::moveBefore called with 'from' iterator which doesn't point to a statement");
  if(*(from.getASTRoot()) == *(to.getASTRoot()))
    throw std::runtime_error("ASTOps::moveBefore called with iterators through the same tree");

  std::shared_ptr<Stmt> prunedStmt = prune(from);
  return insertBefore(prunedStmt, to);
}

} // namespace dawn
