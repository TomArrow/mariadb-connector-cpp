/************************************************************************************
   Copyright (C) 2020,2023 MariaDB Corporation AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/


#include "Results.h"

#include "ExceptionFactory.h"
#include "ServerSidePreparedStatement.h"
#include "com/CmdInformation.h"
#include "com/CmdInformationSingle.h"
#include "com/CmdInformationBatch.h"
#include "com/CmdInformationMultiple.h"

namespace sql
{
namespace mariadb
{
  /**
   * Single Text query. /! use internally, because autoincrement value is not right for
   * multi-queries !/
   */
  Results::Results()
  {
  }

  /**
   * Default constructor.
   *
   * @param statement current statement
   * @param fetchSize fetch size
   * @param batch select result possible
   * @param expectedSize expected size
   * @param binaryFormat use binary protocol
   * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
   *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
   *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
   * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
   *     or <code>ResultSet.CONCUR_UPDATABLE</code>
   * @param autoGeneratedKeys a flag indicating whether auto-generated keys should be returned; one
   *     of <code>Statement.RETURN_GENERATED_KEYS</code> or <code>Statement.NO_GENERATED_KEYS</code>
   * @param autoIncrement Connection auto-increment value
   * @param sql sql command
   * @param parameters parameters
   */
  Results::Results(
      Statement* _statement,
      int32_t fetchSize,
      bool batch,
      std::size_t expectedSize,
      bool binaryFormat,
      int32_t resultSetScrollType,
      int32_t resultSetConcurrency,
      int32_t autoGeneratedKeys,
      int32_t autoIncrement,
      const SQLString& _sql,
      std::vector<Shared::ParameterHolder>& _parameters)
    :
      fetchSize(fetchSize)
    , batch(batch)
    , expectedSize(expectedSize)
    , binaryFormat(binaryFormat)
    , resultSetScrollType(resultSetScrollType)
    , resultSetConcurrency(resultSetConcurrency)
    , autoGeneratedKeys(autoGeneratedKeys)
    , maxFieldSize(_statement->getMaxFieldSize())
    , autoIncrement(autoIncrement)
    , sql(_sql)
    , parameters(_parameters)
  {
    ServerSidePreparedStatement* ssps = dynamic_cast<ServerSidePreparedStatement*>(_statement);
    if (ssps != nullptr) {
      serverPrepResult= ssps->getPrepareResult();
      statement= static_cast<MariaDbStatement*>(*ssps);
    }
    else {
      statement= dynamic_cast<MariaDbStatement*>(_statement);
    }
  }

  Results::~Results()
  {
    if (statement && statement->getProtocol()) {
      loadFully(true, statement->getProtocol());
    }
  }

  /**
   * Add execution statistics.
   *
   * @param updateCount number of updated rows
   * @param insertId primary key
   * @param moreResultAvailable is there additional packet
   */
  void Results::addStats(int64_t updateCount,int64_t insertId,bool moreResultAvailable) {

    if (haveResultInWire && !moreResultAvailable && fetchSize == 0) {
      statement->getProtocol()->removeActiveStreamingResult();
    }
    haveResultInWire= moreResultAvailable;
    if (!cmdInformation){
      if (batch){
        cmdInformation.reset(new CmdInformationBatch(expectedSize, autoIncrement));
      }else if (moreResultAvailable){
        cmdInformation.reset(new CmdInformationMultiple(expectedSize, autoIncrement));
      }else {
        cmdInformation.reset(new CmdInformationSingle(insertId, updateCount, autoIncrement));
        return;
      }
    }
    cmdInformation->addSuccessStat(updateCount,insertId);
  }

  /**
   * Indicate that result is an Error, to set appropriate results.
   *
   * @param moreResultAvailable indicate if other results (ResultSet or updateCount) are available.
   */
  void Results::addStatsError(bool moreResultAvailable) {

    if (haveResultInWire && !moreResultAvailable && fetchSize == 0) {
      statement->getProtocol()->removeActiveStreamingResult();
    }
    haveResultInWire= moreResultAvailable;
    if (!cmdInformation){
      if (batch){
        cmdInformation.reset(new CmdInformationBatch(expectedSize, autoIncrement));
      }else if (moreResultAvailable){
        cmdInformation.reset(new CmdInformationMultiple(expectedSize, autoIncrement));
      }else {
        cmdInformation.reset(new CmdInformationSingle(0, Statement::EXECUTE_FAILED, autoIncrement));
        return;
      }
    }
    cmdInformation->addErrorStat();
  }

  int32_t Results::getCurrentStatNumber(){
    return (!cmdInformation)? 0 : cmdInformation->getCurrentStatNumber();
  }

  /**
   * Add resultSet to results.
   *
   * @param resultSet new resultSet.
   * @param moreResultAvailable indicate if other results (ResultSet or updateCount) are available.
   */
  void Results::addResultSet(SelectResultSet* resultSet, bool moreResultAvailable) {

    if (haveResultInWire && !moreResultAvailable && fetchSize == 0) {
      statement->getProtocol()->removeActiveStreamingResult();
    }
    haveResultInWire= moreResultAvailable;
    if (resultSet->isCallableResult()){
      callableResultSet.reset(resultSet);
      return;
    }
    if (executionResults.size() == 0){
     /* executionResults= new ArrayDeque<>()*/;
    }
    executionResults.emplace_back(resultSet);

    if (!cmdInformation) {
      if (batch) {
        cmdInformation.reset(new CmdInformationBatch(expectedSize, autoIncrement));
      }
      else if (moreResultAvailable) {
        cmdInformation.reset(new CmdInformationMultiple(expectedSize, autoIncrement));
      }
      else {
        cmdInformation.reset(new CmdInformationSingle(0, -1, autoIncrement));
        return;
      }
    }
    cmdInformation->addResultSetStat();
  }

  Shared::CmdInformation Results::getCmdInformation(){
    return cmdInformation;
  }

  void Results::setCmdInformation(CmdInformation* _cmdInformation){
    cmdInformation.reset(_cmdInformation);
  }

  /**
   * Indicate that command / batch is finished, so set current resultSet if needed.
   *
   * @return true id has cmdInformation
   */
  bool Results::commandEnd(){
    if (cmdInformation)
    {
      if (executionResults.size() > 0 && !cmdInformation->isCurrentUpdateCount())
      {
        resultSet.reset(executionResults.begin()->release());
        executionResults.pop_front();
      }else {
        resultSet.reset(nullptr);
      }
      cmdInformation->setRewrite(rewritten);
      return true;
    }
    else {
      resultSet.reset(nullptr);
    }
    return false;
  }


  SelectResultSet* Results::getResultSet(){
    return resultSet.get();
  }


  SelectResultSet* Results::releaseResultSet() {
    return resultSet.release();
  }


  SelectResultSet* Results::getCallableResultSet(){
    return callableResultSet.get();
  }

  /**
   * Load fully current results.
   *
   * <p><i>Lock must be set before using this method</i>
   *
   * @param skip must result be available afterwhile
   * @param protocol current protocol
   * @throws SQLException if any connection error occur
   */
  void Results::loadFully(bool skip, Protocol* protocol) {

    if (fetchSize != 0) {

      fetchSize= 0;
      if (resultSet) {

        if (skip) {
          resultSet->close();
        }
        else {
          resultSet->fetchRemaining();
        }
      }
      else {

        const auto& it= executionResults.begin();

        if (it != executionResults.end())
        {
          if (skip) {
            it->get()->close();
          }
          else {
            it->get()->fetchRemaining();
          }
        }
      }
    }
    if (haveResultInWire){
      while (protocol->hasMoreResults()) {
        protocol->moveToNextResult(this, serverPrepResult);
        if (!skip) {
          protocol->getResult(this);
        }
      }
      haveResultInWire= false;
    }
  }


  /**
   * Connection.abort() has been called, abort remaining active result-set
   *
   * @throws SQLException exception
   */
  void Results::abort()
  {
    if (fetchSize != 0){
      fetchSize= 0;
      if (resultSet)
      {
        resultSet->abort();
      }
      else
      {
        auto firstResult= executionResults.begin();
        if (firstResult != executionResults.end())
        {
          (*firstResult)->abort();
        }
      }
    }
  }

  /**
   * Indicate if result contain result-set that is still streaming from server.
   *
   * @param protocol current protocol
   * @return true if streaming is finished
   */
  bool Results::isFullyLoaded(Protocol* protocol){
    if (fetchSize == 0 || !resultSet){
      return true;
    }
    return resultSet->isFullyLoaded() && executionResults.empty() && !protocol->hasMoreResults();
  }

  /**
   * Position to next resultSet.
   *
   * @param current one of the following <code>Statement</code> constants indicating what should
   *     happen to current <code>ResultSet</code> objects obtained using the method <code>
   *     getResultSet</code>: <code>Statement.CLOSE_CURRENT_RESULT</code>, <code>
   *     Statement.KEEP_CURRENT_RESULT</code>, or <code>Statement.CLOSE_ALL_RESULTS</code>
   * @param protocol current protocol
   * @return true if other resultSet exists.
   * @throws SQLException if any connection error occur.
   */
  bool Results::getMoreResults(int32_t current, Protocol* protocol) {

    std::lock_guard<std::mutex> localScopeLock(*(protocol->getLock()));
    if (fetchSize != 0 && resultSet) {
      
      try {

        if (current == Statement::CLOSE_CURRENT_RESULT && resultSet) {
          resultSet->close();
        }
        else {
          resultSet->fetchRemaining();
        }
      }
      catch (SQLException& e) {
        ExceptionFactory::INSTANCE.create(e).Throw();
      }
    }

    if (haveResultInWire) {
      protocol->moveToNextResult(this, serverPrepResult);
      protocol->getResult(this, serverPrepResult);
    }

    if (cmdInformation->moreResults() && !batch){

      if (current == Statement::CLOSE_CURRENT_RESULT && resultSet){
        resultSet->close();
      }
      if (executionResults.empty() != true){
        resultSet.reset(executionResults.begin()->release());
        executionResults.pop_front();
      }
      return resultSet ? true : false;
    }
    else {
      if (current == Statement::CLOSE_CURRENT_RESULT && resultSet){
        resultSet->close();
      }
      resultSet.reset(nullptr);

      if (cmdInformation->getUpdateCount() == -1 && haveResultInWire) {
        haveResultInWire= false;
        protocol->removeActiveStreamingResult();
      }
      return false;
    }
  }

  int32_t Results::getFetchSize(){
    return fetchSize;
  }

  MariaDbStatement* Results::getStatement(){
    return statement;
  }

  bool Results::isBatch(){
    return batch;
  }

  std::size_t Results::getExpectedSize(){
    return expectedSize;
  }

  bool Results::isBinaryFormat(){
    return binaryFormat;
  }

  void Results::removeFetchSize(){
    fetchSize= 0;
  }

  int32_t Results::getResultSetScrollType(){
    return resultSetScrollType;
  }

  const SQLString& Results::getSql(){
    return sql;
  }

  std::vector<Shared::ParameterHolder>& Results::getParameters(){
    return parameters;
  }

  /**
   * Send a resultSet that contain auto generated keys. 2 differences :
   *
   * <ol>
   *   <li>Batch will list all insert ids.
   *   <li>in case of multi-query is set, resultSet will be per query.
   * </ol>
   *
   * <p>example "INSERT INTO myTable values ('a'),('b');INSERT INTO myTable values
   * ('c'),('d'),('e')" will have a resultSet of 2 values, and when Statement.getMoreResults() will
   * be called, a Statement.getGeneratedKeys will return a resultset with 3 ids.
   *
   * @param protocol current protocol
   * @return a ResultSet containing generated ids.
   * @throws SQLException if autoGeneratedKeys was not set to Statement.RETURN_GENERATED_KEYS
   */
  ResultSet* Results::getGeneratedKeys(Protocol* protocol) {
    if (autoGeneratedKeys != Statement::RETURN_GENERATED_KEYS){
      throw SQLException(
          "Cannot return generated keys : query was not set with Statement::RETURN_GENERATED_KEYS");
    }
    if (cmdInformation)
    {
      if (batch){
        return cmdInformation->getBatchGeneratedKeys(protocol);
      }
      return cmdInformation->getGeneratedKeys(protocol,sql);
    }
    return SelectResultSet::createEmptyResultSet();
  }

  void Results::close(){
    statement= NULL;
    fetchSize= 0;
  }

  int32_t Results::getMaxFieldSize(){
    return maxFieldSize;
  }

  void Results::setAutoIncrement(int32_t autoIncrement){
    this->autoIncrement= autoIncrement;
  }

  int32_t Results::getResultSetConcurrency(){
    return resultSetConcurrency;
  }

  int32_t Results::getAutoGeneratedKeys(){
    return autoGeneratedKeys;
  }

  bool Results::isRewritten(){
    return rewritten;
  }

  void Results::setRewritten(bool rewritten){
    this->rewritten= rewritten;
  }

}
}
