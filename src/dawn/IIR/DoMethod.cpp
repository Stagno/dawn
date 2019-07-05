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

#include "dawn/IIR/DoMethod.h"
#include "dawn/IIR/Accesses.h"
#include "dawn/IIR/DependencyGraphAccesses.h"
#include "dawn/IIR/IIR.h"
#include "dawn/IIR/IIRNodeIterator.h"
#include "dawn/IIR/MultiStage.h"
#include "dawn/IIR/Stage.h"
#include "dawn/IIR/StatementAccessesPair.h"
#include "dawn/IIR/Stencil.h"
#include "dawn/IIR/StencilMetaInformation.h"
#include "dawn/Optimizer/AccessComputation.h"
#include "dawn/Optimizer/AccessUtils.h"
#include "dawn/Optimizer/OptimizerContext.h"
#include "dawn/Optimizer/StatementMapper.h"
#include "dawn/SIR/ASTOps.h"
#include "dawn/SIR/Statement.h"
#include "dawn/Support/IndexGenerator.h"
#include "dawn/Support/Logging.h"
#include <boost/optional.hpp>

namespace dawn {
namespace iir {

DoMethod::DoMethod(Interval interval, const StencilMetaInformation& metaData)
    : interval_(interval), id_(IndexGenerator::Instance().getIndex()), metaData_(metaData),
      ASTStmtToSAPMap_() {}

DoMethod::DoMethod(Interval interval, const StencilMetaInformation& metaData,
                   const std::shared_ptr<AST> ast)
    : interval_(interval), id_(IndexGenerator::Instance().getIndex()), metaData_(metaData),
      ast_(ast), ASTStmtToSAPMap_() {}

DoMethod::DoMethod(Interval interval, const StencilMetaInformation& metaData,
                   const std::shared_ptr<AST> ast, const std::shared_ptr<SIR>& fullSIR,
                   iir::StencilInstantiation* instantiation,
                   const std::shared_ptr<std::vector<sir::StencilCall*>>& stackTrace,
                   const std::unordered_map<std::string, int>& localFieldnameToAccessIDMap)
    : DoMethod(interval, metaData) {
  fillWithAST(ast, fullSIR, instantiation, stackTrace, localFieldnameToAccessIDMap);
}

// TODO check correctness of clone
std::unique_ptr<DoMethod> DoMethod::clone() const {

  auto cloneMS = make_unique<DoMethod>(interval_, metaData_, ast_->clone());

  cloneMS->setID(id_);
  cloneMS->setDependencyGraph(derivedInfo_.dependencyGraph_->clone());
  cloneMS->derivedInfo_.fields_ = derivedInfo_.fields_;

  cloneMS->ASTStmtToSAPMap_ = ASTStmtToSAPMapType(ASTStmtToSAPMap_);
  return cloneMS;
}

Interval& DoMethod::getInterval() { return interval_; }

const Interval& DoMethod::getInterval() const { return interval_; }

void DoMethod::setDependencyGraph(const std::shared_ptr<DependencyGraphAccesses>& DG) {
  derivedInfo_.dependencyGraph_ = DG;
}

boost::optional<Extents> DoMethod::computeMaximumExtents(const int accessID) const {
  boost::optional<Extents> extents;

  for(auto& stmtAccess : sapRange()) {
    auto extents_ = stmtAccess.computeMaximumExtents(accessID, ASTStmtToSAPMap_);
    if(!extents_.is_initialized())
      continue;

    if(extents.is_initialized()) {
      extents->merge(*extents_);
    } else {
      extents = extents_;
    }
  }
  return extents;
}

boost::optional<Interval>
DoMethod::computeEnclosingAccessInterval(const int accessID, const bool mergeWithDoInterval) const {
  boost::optional<Interval> interval;

  boost::optional<Extents>&& extents = computeMaximumExtents(accessID);

  if(extents.is_initialized()) {
    if(mergeWithDoInterval)
      extents->addCenter(2);
    return boost::make_optional(getInterval())->extendInterval(*extents);
  }
  return interval;
}

void DoMethod::setInterval(const Interval& interval) { interval_ = interval; }

void DoMethod::fillWithAST(
    const std::shared_ptr<AST> ast, const std::shared_ptr<SIR>& fullSIR,
    iir::StencilInstantiation* instantiation,
    const std::shared_ptr<std::vector<sir::StencilCall*>>& stackTrace,
    const std::unordered_map<std::string, int>& localFieldnameToAccessIDMap) {

  ast_->setRoot(ast->getRoot());

  // Here we convert the AST of the vertical region to a flat list of statements of the stage.
  // Further, we instantiate all referenced stencil functions.
  DAWN_LOG(INFO) << "Inserting statements ... ";
  StatementMapper statementMapper(fullSIR, instantiation, stackTrace, *this, interval_,
                                  localFieldnameToAccessIDMap, nullptr);
  ast_->accept(statementMapper);
  DAWN_LOG(INFO) << "Inserted " << getStmts().size() << " statements";
  if(instantiation->getOptimizerContext()->getDiagnostics().hasErrors())
    return;
  // Here we compute the *actual* access of each statement and associate access to the AccessIDs
  // we set previously.
  DAWN_LOG(INFO) << "Filling accesses ...";
  computeAccesses(metaData_, ASTStmtToSAPMap_, sapRange());
}

DoMethod::StmtsConstIterator
DoMethod::insertStatementAfterImpl(SAPConstIterator otherSapIt,
                                   StmtsConstIterator& insertionPointIt) {
  auto insertedOnlyFirstLevelIt =
      ASTOps::insertAfter((*otherSapIt).getStatement()->ASTStmt->clone(), insertionPointIt);

  auto insertedFullASTIt = insertedOnlyFirstLevelIt.toggleOnlyFirstLevelVisiting();
  auto otherSapFullAstIt = otherSapIt.toggleOnlyFirstLevelVisiting();

  for(; !otherSapFullAstIt.isEnd(); ++otherSapFullAstIt) {
    ASTStmtToSAPMap_.emplace(&*insertedFullASTIt, std::move(*((*otherSapFullAstIt).clone())));
    ++insertedFullASTIt;
  }

  return insertedOnlyFirstLevelIt;
}

void DoMethod::appendStatement(SAPConstIterator otherSapIt) {
  DAWN_ASSERT(!otherSapIt.isEnd());
  auto itEnd = stmtsEnd().clone();
  insertStatementAfterImpl(std::move(otherSapIt), itEnd);

  computeAccesses(metaData_, ASTStmtToSAPMap_, sapRange());
}

void DoMethod::appendStatements(IteratorRange<SAPConstIterator> otherSaps) {
  auto insertionPointIt = stmtsEnd().clone();
  for(SAPConstIterator otherSapIt = otherSaps.begin(); otherSapIt != otherSaps.end();
      ++otherSapIt) {

    auto insertedOnlyFirstLevelIt = insertStatementAfterImpl(otherSapIt.clone(), insertionPointIt);

    insertionPointIt = std::move(insertedOnlyFirstLevelIt);
  }
  computeAccesses(metaData_, ASTStmtToSAPMap_, sapRange());
}

const std::shared_ptr<DependencyGraphAccesses>& DoMethod::getDependencyGraph() const {
  return derivedInfo_.dependencyGraph_;
}

void DoMethod::DerivedInfo::clear() { fields_.clear(); }

void DoMethod::clearDerivedInfo() { derivedInfo_.clear(); }

json::json DoMethod::jsonDump(const StencilMetaInformation& metaData) const {
  json::json node;
  node["ID"] = id_;
  std::stringstream ss;
  ss << interval_;
  node["interval"] = ss.str();

  json::json fieldsJson;
  for(const auto& field : derivedInfo_.fields_) {
    fieldsJson[metaData.getNameFromAccessID(field.first)] = field.second.jsonDump();
  }
  node["Fields"] = fieldsJson;

  json::json stmtsJson;
  for(const auto& sap : sapRange()) {
    stmtsJson.push_back(sap.jsonDump(metaData));
  }
  node["Stmts"] = stmtsJson;
  return node;
}

void DoMethod::updateLevel() {

  // Compute the fields and their intended usage. Fields can be in one of three states: `Output`,
  // `InputOutput` or `Input` which implements the following state machine:
  //
  //    +-------+                               +--------+
  //    | Input |                               | Output |
  //    +-------+                               +--------+
  //        |                                       |
  //        |            +-------------+            |
  //        +----------> | InputOutput | <----------+
  //                     +-------------+
  //
  std::unordered_map<int, Field> inputOutputFields;
  std::unordered_map<int, Field> inputFields;
  std::unordered_map<int, Field> outputFields;

  for(const auto& statementAccessesPair : sapRange()) {
    const auto& access = statementAccessesPair.getAccesses();
    DAWN_ASSERT(access);

    for(const auto& accessPair : access->getWriteAccesses()) {
      int AccessID = accessPair.first;
      Extents const& extents = accessPair.second;

      // Does this AccessID correspond to a field access?
      if(!metaData_.isAccessType(FieldAccessType::FAT_Field, AccessID)) {
        continue;
      }
      AccessUtils::recordWriteAccess(inputOutputFields, inputFields, outputFields, AccessID,
                                     extents, getInterval());
    }

    for(const auto& accessPair : access->getReadAccesses()) {
      int AccessID = accessPair.first;
      Extents const& extents = accessPair.second;

      // Does this AccessID correspond to a field access?
      if(!metaData_.isAccessType(FieldAccessType::FAT_Field, AccessID)) {
        continue;
      }

      AccessUtils::recordReadAccess(inputOutputFields, inputFields, outputFields, AccessID, extents,
                                    getInterval());
    }
  }

  // Merge inputFields, outputFields and fields
  derivedInfo_.fields_.insert(outputFields.begin(), outputFields.end());
  derivedInfo_.fields_.insert(inputOutputFields.begin(), inputOutputFields.end());
  derivedInfo_.fields_.insert(inputFields.begin(), inputFields.end());

  if(derivedInfo_.fields_.empty()) {
    DAWN_LOG(WARNING) << "no fields referenced in stage";
    return;
  }

  // Compute the extents of each field by accumulating the extents of each access to field in the
  // stage
  for(const auto& statementAccessesPair : sapRange()) {
    const auto& access = statementAccessesPair.getAccesses();

    // first => AccessID, second => Extent
    for(auto& accessPair : access->getWriteAccesses()) {
      if(!metaData_.isAccessType(FieldAccessType::FAT_Field, accessPair.first))
        continue;

      derivedInfo_.fields_.at(accessPair.first).mergeWriteExtents(accessPair.second);
    }

    for(const auto& accessPair : access->getReadAccesses()) {
      if(!metaData_.isAccessType(FieldAccessType::FAT_Field, accessPair.first))
        continue;

      derivedInfo_.fields_.at(accessPair.first).mergeReadExtents(accessPair.second);
    }
  }
}

class CheckNonNullStatementVisitor : public ASTVisitorForwarding, public NonCopyable {
private:
  bool result_ = false;

public:
  CheckNonNullStatementVisitor() {}
  virtual ~CheckNonNullStatementVisitor() override {}

  bool getResult() const { return result_; }

  virtual void visit(const std::shared_ptr<ExprStmt>& expr) override {
    if(!isa<NOPExpr>(expr->getExpr().get()))
      result_ = true;
    else {
      ASTVisitorForwarding::visit(expr);
    }
  }
};

bool DoMethod::isEmptyOrNullStmt() const {
  for(const std::shared_ptr<Stmt>& root : getStmts()) {
    CheckNonNullStatementVisitor checker;
    root->accept(checker);

    if(checker.getResult()) {
      return false;
    }
  }
  return true;
}

} // namespace iir
} // namespace dawn
