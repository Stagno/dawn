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

#ifndef DAWN_IIR_SAPITERATOR_H
#define DAWN_IIR_SAPITERATOR_H

#include "dawn/IIR/IIRNode.h"
#include "dawn/IIR/IIRNodeIterator.h"
#include "dawn/IIR/StatementAccessesPair.h"
#include "dawn/SIR/ASTNodeIterator.h"
#include "dawn/Support/IteratorRange.h"
#include <unordered_map>

namespace dawn {

/// @brief Iterator through StatementAccessesPairs of statements of an AST
///
/// Built on top of an ASTNodeIterator that iterates through statements, requires a map from ASTStmt
/// to StatementAccessesPair. Can either visit the first level of the AST or the whole AST.
template <ASTNodeIteratorVisitKind onlyFirstLevel>
class StatementAccessesPairIterator {
public:
  using WrappedIterator = ASTNodeIterator<onlyFirstLevel>;
  using ASTStmtToSAPMapType = std::unordered_map<const Stmt*, iir::StatementAccessesPair>;

  StatementAccessesPairIterator(const std::shared_ptr<Stmt>& root,
                                const ASTStmtToSAPMapType& ASTStmtToSAPMap)
      : current_(WrappedIterator::CreateInstance(root)),
        ASTStmtToSAPMap_(const_cast<ASTStmtToSAPMapType&>(ASTStmtToSAPMap)) {}
  StatementAccessesPairIterator(const WrappedIterator& it,
                                const ASTStmtToSAPMapType& ASTStmtToSAPMap)
      : current_(it.clone()), ASTStmtToSAPMap_(const_cast<ASTStmtToSAPMapType&>(ASTStmtToSAPMap)) {}
  StatementAccessesPairIterator(StatementAccessesPairIterator&& it) = default;
  StatementAccessesPairIterator(const StatementAccessesPairIterator&) = delete;
  StatementAccessesPairIterator& operator=(StatementAccessesPairIterator&& o) {
    *this = std::move(o);
  }
  StatementAccessesPairIterator& operator=(const StatementAccessesPairIterator&) = delete;

  StatementAccessesPairIterator clone() const {
    return StatementAccessesPairIterator(current_, ASTStmtToSAPMap_);
  }
  /// @brief returns a clone of the underlying ASTNodeIterator
  WrappedIterator base() const { return current_.clone(); }
  inline iir::StatementAccessesPair& operator*() const { return ASTStmtToSAPMap_.at(&*current_); }
  StatementAccessesPairIterator& operator++() {
    ++current_;
    return *this;
  }
  inline bool operator==(const StatementAccessesPairIterator& other) const {
    return current_ == other.current_;
  }

  inline bool operator!=(const StatementAccessesPairIterator& other) const {
    return current_ != other.current_;
  }
  inline bool isEnd() const { return current_.isEnd(); }
  inline StatementAccessesPairIterator& setToEnd() {
    current_.setToEnd();
    return *this;
  }
  /// @brief returns a new StatementAccessesPairIterator pointing to the same node of the first
  /// level of the AST but of switched visit type (only first-level or full).
  inline StatementAccessesPairIterator<
      static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(onlyFirstLevel))>
  toggleOnlyFirstLevelVisiting() const {
    return StatementAccessesPairIterator<static_cast<ASTNodeIteratorVisitKind>(!static_cast<bool>(
        onlyFirstLevel))>(current_.toggleOnlyFirstLevelVisiting(), ASTStmtToSAPMap_);
  }

private:
  WrappedIterator current_;
  ASTStmtToSAPMapType& ASTStmtToSAPMap_;
};

template <ASTNodeIteratorVisitKind onlyFirstLevel>
class StatementAccessesPairRange
    : public IteratorRange<StatementAccessesPairIterator<onlyFirstLevel>> {
public:
  using Iterator = StatementAccessesPairIterator<onlyFirstLevel>;

  StatementAccessesPairRange(const std::shared_ptr<Stmt>& root,
                             const typename Iterator::ASTStmtToSAPMapType& ASTStmtToSAPMap)
      : IteratorRange<StatementAccessesPairIterator<onlyFirstLevel>>(
            std::move(Iterator(root, ASTStmtToSAPMap)),
            std::move(Iterator(root, ASTStmtToSAPMap).setToEnd())) {}
  StatementAccessesPairRange(const Iterator& singleton)
      : IteratorRange<StatementAccessesPairIterator<onlyFirstLevel>>(
            std::move(Iterator(singleton.clone())), std::move(++Iterator(singleton.clone()))) {}

  StatementAccessesPairRange(StatementAccessesPairRange&&) = default;
  StatementAccessesPairRange(const StatementAccessesPairRange&) = delete;
  StatementAccessesPairRange& operator=(StatementAccessesPairRange&& o) { *this = std::move(o); }
  StatementAccessesPairRange& operator=(const StatementAccessesPairRange&) = delete;
};

template <ASTNodeIteratorVisitKind onlyFirstLevel>
StatementAccessesPairRange<onlyFirstLevel> iterateStatementAccessesPairOver(
    const std::shared_ptr<Stmt>& root,
    typename StatementAccessesPairRange<onlyFirstLevel>::Iterator::ASTStmtToSAPMapType&
        ASTStmtToSAPMap) {
  return StatementAccessesPairRange<onlyFirstLevel>(root, ASTStmtToSAPMap);
}

} // namespace dawn

namespace std {
template <dawn::ASTNodeIteratorVisitKind onlyFirstLevel>
struct iterator_traits<dawn::StatementAccessesPairIterator<onlyFirstLevel>> {
  using difference_type = typename iterator_traits<typename dawn::StatementAccessesPairIterator<
      onlyFirstLevel>::WrappedIterator>::difference_type;
  using value_type = dawn::iir::StatementAccessesPair;
  using pointer = dawn::iir::StatementAccessesPair*;
  using reference = dawn::iir::StatementAccessesPair&;
  using iterator_category = std::input_iterator_tag;
};

} // namespace std

#endif
