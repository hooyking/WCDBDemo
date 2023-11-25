//
// Created by qiuwenchen on 2022/3/31.
//

/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "DatabaseBridge.h"
#include "CipherConfig.hpp"
#include "Core.hpp"
#include "CustomConfig.hpp"
#include "DBOperationNotifier.hpp"
#include "InnerDatabase.hpp"
#include "ObjectBridge.hpp"

WCDBDefineNoArgumentSwiftClosureBridgedType(WCDBDatabaseCloseCallback, void)

WCDBDefineMultiArgumentSwiftClosureBridgedType(
WCDBPerformanceTracer, void, long, const char*, uint64_t, const char*, double);

WCDBDefineMultiArgumentSwiftClosureBridgedType(WCDBSQLTracer, void, long, const char*, uint64_t, const char*)

WCDBDefineOneArgumentSwiftClosureBridgedType(WCDBErrorTracer, void, CPPError)

WCDBDefineOneArgumentSwiftClosureBridgedType(WCDBConfig, bool, CPPHandle)

WCDBDefineOneArgumentSwiftClosureBridgedType(WCDBPathEnumerator, void, const char*)

WCDBDefineNoArgumentSwiftClosureBridgedType(WCDBDatabaseCloseCallback, void)

WCDBDefineOneArgumentSwiftClosureBridgedType(WCDBCorruptedNotification, void, CPPDatabase)

WCDBDefineOneArgumentSwiftClosureBridgedType(WCDBBackupFilter, bool, const char*)

WCDBDefineMultiArgumentSwiftClosureBridgedType(WCDBRetrieveProgress, void, void*, double, double)

WCDBDefineMultiArgumentSwiftClosureBridgedType(WCDBDatabaseOperationTracer, void, CPPDatabase, long);

WCDBDefineMultiArgumentSwiftClosureBridgedType(
WCDBDatabaseMigrationFilter, void, const char*, const char*, char**, char**);

WCDBDefineMultiArgumentSwiftClosureBridgedType(WCDBDatabaseMigrationNotification,
                                               void,
                                               CPPDatabase,
                                               const char*,
                                               const char*,
                                               const char*,
                                               const char*);

CPPError WCDBDatabaseGetError(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, CPPError());
    const WCDB::Error& error = cppDatabase->getThreadedError();
    return WCDBCreateUnmanagedCPPObject(CPPError, &error);
}

long WCDBDatabaseGetTag(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, 0);
    return cppDatabase->getTag();
}

void WCDBDatabaseSetTag(CPPDatabase database, long tag)
{
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    cppDatabase->setTag(tag);
}

const char* WCDBDatabaseGetPath(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, nullptr);
    return cppDatabase->getPath().data();
}

void WCDBDatabaseGetPaths(CPPDatabase database, SwiftClosure* _Nonnull enumerator)
{
    if (!enumerator) {
        return;
    }
    WCDBPathEnumerator bridgeEnumerator
    = WCDBCreateSwiftBridgedClosure(WCDBPathEnumerator, enumerator);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    std::list<WCDB::StringView> paths = cppDatabase->getPaths();
    for (auto path : paths) {
        WCDBSwiftClosureCallWithOneArgument(bridgeEnumerator, path.data());
    }
}

CPPHandle WCDBDatabaseGetHandle(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, CPPHandle());
    WCDB::RecyclableHandle cppHandle = cppDatabase->getHandle();
    return WCDBCreateRecylableCPPObject(CPPHandle, cppHandle);
}

bool WCDBDatabaseCanOpen(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->canOpen();
}

bool WCDBDatabaseIsOpened(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->isOpened();
}

bool WCDBDatabaseIsBlockaded(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->isBlockaded();
}

void WCDBDatabaseClose(CPPDatabase database, SwiftClosure* callback)
{
    WCDBDatabaseCloseCallback closeCallback
    = WCDBCreateSwiftBridgedClosure(WCDBDatabaseCloseCallback, callback);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    cppDatabase->close([closeCallback]() {
        if (WCDBGetSwiftClosure(closeCallback) != nullptr) {
            WCDBSwiftClosureCall(closeCallback);
        }
    });
}

void WCDBDatabaseBlockaded(CPPDatabase database)
{
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    cppDatabase->blockade();
}

void WCDBDatabaseUnblockaded(CPPDatabase database)
{
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    cppDatabase->unblockade();
}

void WCDBDatabasePurge(CPPDatabase database)
{
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    cppDatabase->purge();
}

void WCDBDatabaseConfigCipher(CPPDatabase database,
                              const unsigned char* cipherKey,
                              int keyLength,
                              int pageSize,
                              int cipherVersion)
{
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    if (cipherKey) {
        cppDatabase->setConfig(
        WCDB::CipherConfigName,
        std::static_pointer_cast<WCDB::Config>(std::make_shared<WCDB::CipherConfig>(
        WCDB::UnsafeData::immutable(cipherKey, (size_t) keyLength), pageSize, cipherVersion)),
        WCDB::Configs::Priority::Highest);
    } else {
        cppDatabase->removeConfig(WCDB::CipherConfigName);
    }
}

void WCDBDatabaseConfig(CPPDatabase database,
                        const char* name,
                        SwiftClosure* _Nonnull invocation,
                        SwiftClosure* _Nullable uninvocation,
                        int priority)
{
    WCDBConfig bridgeInvocation = WCDBCreateSwiftBridgedClosure(WCDBConfig, invocation);
    WCDBConfig bridgeUninvocation = WCDBCreateSwiftBridgedClosure(WCDBConfig, uninvocation);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    WCDB::CustomConfig::Invocation cppInvocation
    = [bridgeInvocation](WCDB::InnerHandle* handle) -> bool {
        return WCDBSwiftClosureCallWithOneArgument(
        bridgeInvocation, WCDBCreateUnmanagedCPPObject(CPPHandle, handle));
    };
    WCDB::CustomConfig::Invocation cppUninvocation = nullptr;
    if (WCDBGetSwiftClosure(bridgeUninvocation) != nullptr) {
        cppUninvocation = [bridgeUninvocation](WCDB::InnerHandle* handle) -> bool {
            return WCDBSwiftClosureCallWithOneArgument(
            bridgeUninvocation, WCDBCreateUnmanagedCPPObject(CPPHandle, handle));
        };
    }
    cppDatabase->setConfig(
    WCDB::UnsafeStringView(name),
    std::static_pointer_cast<WCDB::Config>(
    std::make_shared<WCDB::CustomConfig>(cppInvocation, cppUninvocation)),
    priority);
}

void WCDBDatabaseGlobalTracePerformance(SwiftClosure* _Nullable tracer)
{
    WCDBPerformanceTracer bridgedTracer
    = WCDBCreateSwiftBridgedClosure(WCDBPerformanceTracer, tracer);
    WCDB::InnerHandle::PerformanceNotification callback = nullptr;
    if (WCDBGetSwiftClosure(bridgedTracer) != nullptr) {
        callback = [bridgedTracer](const WCDB::Tag& tag,
                                   const WCDB::UnsafeStringView& path,
                                   const WCDB::UnsafeStringView& sql,
                                   double cost,
                                   const void* handle) {
            WCDBSwiftClosureCallWithMultiArgument(
            bridgedTracer, tag, path.data(), (uint64_t) handle, sql.data(), cost);
        };
    }
    WCDB::Core::shared().setNotificationWhenPerformanceGlobalTraced(callback);
}

void WCDBDatabaseTracePerformance(CPPDatabase database, SwiftClosure* _Nullable tracer)
{
    WCDBPerformanceTracer bridgedTracer
    = WCDBCreateSwiftBridgedClosure(WCDBPerformanceTracer, tracer);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    if (WCDBGetSwiftClosure(bridgedTracer) != nullptr) {
        WCDB::InnerHandle::PerformanceNotification callback
        = [bridgedTracer](const WCDB::Tag& tag,
                          const WCDB::UnsafeStringView& path,
                          const WCDB::UnsafeStringView& sql,
                          double cost,
                          const void* handle) {
              WCDBSwiftClosureCallWithMultiArgument(
              bridgedTracer, tag, path.data(), (uint64_t) handle, sql.data(), cost);
          };
        cppDatabase->setConfig(WCDB::PerformanceTraceConfigName,
                               std::static_pointer_cast<WCDB::Config>(
                               std::make_shared<WCDB::PerformanceTraceConfig>(callback)),
                               WCDB::Configs::Priority::Highest);
    } else {
        cppDatabase->removeConfig(WCDB::PerformanceTraceConfigName);
    }
}
void WCDBDatabaseGlobalTraceSQL(SwiftClosure* _Nullable tracer)
{
    WCDBSQLTracer bridgedTracer = WCDBCreateSwiftBridgedClosure(WCDBSQLTracer, tracer);
    WCDB::InnerHandle::SQLNotification callback = nullptr;
    if (WCDBGetSwiftClosure(bridgedTracer) != nullptr) {
        callback = [bridgedTracer](const WCDB::Tag& tag,
                                   const WCDB::UnsafeStringView& path,
                                   const WCDB::UnsafeStringView& sql,
                                   const void* handle) {
            WCDBSwiftClosureCallWithMultiArgument(
            bridgedTracer, tag, path.data(), (uint64_t) handle, sql.data());
        };
    }
    WCDB::Core::shared().setNotificationForSQLGLobalTraced(callback);
}

void WCDBDatabaseTraceSQL(CPPDatabase database, SwiftClosure* _Nullable tracer)
{
    WCDBSQLTracer bridgedTracer = WCDBCreateSwiftBridgedClosure(WCDBSQLTracer, tracer);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    if (WCDBGetSwiftClosure(bridgedTracer) != nullptr) {
        WCDB::InnerHandle::SQLNotification callback
        = [bridgedTracer](const WCDB::Tag& tag,
                          const WCDB::UnsafeStringView& path,
                          const WCDB::UnsafeStringView& sql,
                          const void* handle) {
              WCDBSwiftClosureCallWithMultiArgument(
              bridgedTracer, tag, path.data(), (uint64_t) handle, sql.data());
          };
        cppDatabase->setConfig(WCDB::SQLTraceConfigName,
                               std::static_pointer_cast<WCDB::Config>(
                               std::make_shared<WCDB::SQLTraceConfig>(callback)),
                               WCDB::Configs::Priority::Highest);
    } else {
        cppDatabase->removeConfig(WCDB::SQLTraceConfigName);
    }
}

void WCDBDatabaseGlobalTraceError(SwiftClosure* _Nullable tracer)
{
    WCDBErrorTracer bridgedTracer = WCDBCreateSwiftBridgedClosure(WCDBErrorTracer, tracer);
    if (WCDBGetSwiftClosure(bridgedTracer) != nullptr) {
        WCDB::Core::shared().setNotificationWhenErrorTraced(
        [bridgedTracer](const WCDB::Error& error) {
            CPPError cppError = WCDBCreateUnmanagedCPPObject(CPPError, &error);
            WCDBSwiftClosureCallWithOneArgument(bridgedTracer, cppError);
        });
    } else {
        WCDB::Core::shared().setNotificationWhenErrorTraced(nullptr);
    }
}

void WCDBDatabaseTraceError(const char* _Nonnull path, SwiftClosure* _Nullable tracer)
{
    WCDBErrorTracer bridgedTracer = WCDBCreateSwiftBridgedClosure(WCDBErrorTracer, tracer);
    if (WCDBGetSwiftClosure(bridgedTracer) != nullptr) {
        WCDB::Core::shared().setNotificationWhenErrorTraced(
        path, [bridgedTracer](const WCDB::Error& error) {
            CPPError cppError = WCDBCreateUnmanagedCPPObject(CPPError, &error);
            WCDBSwiftClosureCallWithOneArgument(bridgedTracer, cppError);
        });
    } else {
        WCDB::Core::shared().setNotificationWhenErrorTraced(path, nullptr);
    }
}

void WCDBDatabaseGlobalTraceOperation(SwiftClosure* _Nullable tracer)
{
    WCDBDatabaseOperationTracer bridgeTracer
    = WCDBCreateSwiftBridgedClosure(WCDBDatabaseOperationTracer, tracer);
    if (WCDBGetSwiftClosure(bridgeTracer) != nullptr) {
        WCDB::DBOperationNotifier::shared().setNotification(
        [=](WCDB::InnerDatabase* database,
            WCDB::DBOperationNotifier::Operation operation,
            WCDB::StringViewMap<WCDB::Value>) {
            CPPDatabase cppDatabase = WCDBCreateUnmanagedCPPObject(CPPDatabase, database);
            WCDBSwiftClosureCallWithMultiArgument(bridgeTracer, cppDatabase, (long) operation);
        });
    } else {
        WCDB::DBOperationNotifier::shared().setNotification(nullptr);
    }
}

bool WCDBDatabaseRemoveFile(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->removeFiles();
}

bool WCDBDatabaseMoveFile(CPPDatabase database, const char* _Nonnull destination)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->moveFiles(WCDB::UnsafeStringView(destination));
}

OptionalUInt64 WCDBDatabaseGetFileSize(CPPDatabase database)
{
    OptionalUInt64 ret = { false, 0 };
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, ret);
    auto size = cppDatabase->getFilesSize();
    if (size.succeed()) {
        ret.hasValue = true;
        ret.value = size.value();
    }
    return ret;
}

void WCDBDatabaseSetNotificationWhenCorrupted(CPPDatabase database, SwiftClosure* _Nullable onCorrupted)
{
    WCDBCorruptedNotification notification
    = WCDBCreateSwiftBridgedClosure(WCDBCorruptedNotification, onCorrupted);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    WCDB::Core::CorruptedNotification cppNotification = nullptr;
    if (WCDBGetSwiftClosure(notification) != nullptr) {
        cppNotification = [notification](WCDB::InnerDatabase* cppDatabase) {
            CPPDatabase database = WCDBCreateUnmanagedCPPObject(CPPDatabase, cppDatabase);
            WCDBSwiftClosureCallWithOneArgument(notification, database);
        };
    }
    WCDB::Core::shared().setNotificationWhenDatabaseCorrupted(
    cppDatabase->getPath(), cppNotification);
}

bool WCDBDatabaseCheckIfCorrupted(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    cppDatabase->checkIntegrity(false);
    return WCDBDatabaseCheckIsAlreadyCorrupted(database);
}

bool WCDBDatabaseCheckIsAlreadyCorrupted(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return WCDB::Core::shared().isFileObservedCorrupted(cppDatabase->getPath());
}

void WCDBDatabaseEnableAutoBackup(CPPDatabase database, bool enable)
{
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    WCDB::Core::shared().enableAutoBackup(cppDatabase, enable);
}

bool WCDBDatabaseBackup(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->backup(false);
}

void WCDBDatabaseFilterBackup(CPPDatabase database, SwiftClosure* _Nullable tableShouldBeBackedUp)
{
    WCDBBackupFilter backupFilter
    = WCDBCreateSwiftBridgedClosure(WCDBBackupFilter, tableShouldBeBackedUp);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    WCDB::InnerDatabase::BackupFilter filter = nullptr;
    if (WCDBGetSwiftClosure(backupFilter) != nullptr) {
        filter = [backupFilter](const WCDB::UnsafeStringView& tableName) -> bool {
            return WCDBSwiftClosureCallWithOneArgument(backupFilter, tableName.data());
        };
    }
    cppDatabase->filterBackup(filter);
}

bool WCDBDatabaseDeposit(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->deposit();
}

bool WCDBDatabaseRemoveDepositedFiles(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->removeDeposited();
}

bool WCDBDatabaseContainDepositedFiles(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->containsDeposited();
}

double WCDBDatabaseRetrieve(CPPDatabase database, SwiftClosure* _Nullable onProgressUpdated)
{
    WCDBRetrieveProgress progress
    = WCDBCreateSwiftBridgedClosure(WCDBRetrieveProgress, onProgressUpdated);
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    WCDB::InnerDatabase::RetrieveProgressCallback callback = nullptr;
    if (WCDBGetSwiftClosure(progress) != nullptr) {
        callback = [progress](double percentage, double increment) {
            WCDBSwiftClosureCallWithMultiArgument(progress, nullptr, percentage, increment);
        };
    }
    return cppDatabase->retrieve(callback);
}

bool WCDBDatabasePassiveCheckpoint(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->checkpoint(false, WCDB::InnerDatabase::CheckPointMode::Passive);
}

bool WCDBDatabaseTruncateCheckpoint(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->checkpoint(false, WCDB::InnerDatabase::CheckPointMode::Truncate);
}

void WCDBDatabaseFilterMigration(CPPDatabase database, SwiftClosure* _Nullable filter)
{
    WCDBDatabaseMigrationFilter bridgedFilter
    = WCDBCreateSwiftBridgedClosure(WCDBDatabaseMigrationFilter, filter);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    WCDB::InnerDatabase::MigrationFilter cppFilter = nullptr;
    if (WCDBGetSwiftClosure(bridgedFilter) != nullptr) {
        cppFilter = [bridgedFilter](WCDB::MigrationUserInfo& info) {
            char* sourceDatabase = nullptr;
            char* sourceTable = nullptr;
            WCDBSwiftClosureCallWithMultiArgument(bridgedFilter,
                                                  info.getDatabase().data(),
                                                  info.getTable().data(),
                                                  &sourceDatabase,
                                                  &sourceTable);
            if (sourceTable != nullptr) {
                info.setSource(sourceTable, sourceDatabase);
                free(sourceTable);
            }
            if (sourceDatabase != nullptr) {
                free(sourceDatabase);
            }
        };
    }
    cppDatabase->filterMigration(cppFilter);
}

bool WCDBDatabaseStepMigration(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return (cppDatabase->stepMigration(false)).succeed();
}

void WCDBDatabaseEnableAutoMigration(CPPDatabase database, bool flag)
{
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    WCDB::Core::shared().enableAutoMigration(cppDatabase, flag);
}

void WCDBDatabaseSetNotificationWhenMigrated(CPPDatabase database, SwiftClosure* _Nullable onMigrated)
{
    WCDBDatabaseMigrationNotification notification
    = WCDBCreateSwiftBridgedClosure(WCDBDatabaseMigrationNotification, onMigrated);
    WCDBGetObjectOrReturn(database, WCDB::InnerDatabase, cppDatabase);
    WCDB::InnerDatabase::MigratedCallback callback = nullptr;
    if (onMigrated != nullptr) {
        callback = [notification](WCDB::InnerDatabase* cppDatabase,
                                  const WCDB::MigrationBaseInfo* baseInfo) {
            CPPDatabase database = WCDBCreateUnmanagedCPPObject(CPPDatabase, cppDatabase);
            if (baseInfo != nullptr) {
                WCDBSwiftClosureCallWithMultiArgument(
                notification,
                database,
                baseInfo->getDatabase().data(),
                baseInfo->getTable().data(),
                baseInfo->getSourceDatabase().data(),
                baseInfo->getSourceTable().data());
            } else {
                WCDBSwiftClosureCallWithMultiArgument(
                notification, database, nullptr, nullptr, nullptr, nullptr);
            }
        };
    }
    cppDatabase->setNotificationWhenMigrated(callback);
}

bool WCDBDatabaseIsMigrated(CPPDatabase database)
{
    WCDBGetObjectOrReturnValue(database, WCDB::InnerDatabase, cppDatabase, false);
    return cppDatabase->isMigrated();
}
