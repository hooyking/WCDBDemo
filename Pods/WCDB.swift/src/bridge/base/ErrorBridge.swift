//
// Created by qiuwenchen on 2022/4/2.
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

import Foundation
import WCDB_Private

internal final class ErrorBridge {
    static func getErrorFrom(cppError error: CPPError) -> WCDBError {
        let recycleError = ObjectBridge.createRecyclableCPPObject(error)
        let level = WCDBError.Level(rawValue: Int(WCDBErrorGetLevel(recycleError.raw)))
        let code = WCDBError.Code(rawValue: WCDBErrorGetCode(recycleError.raw))
        let message = String(cString: WCDBErrorGetMsg(recycleError.raw))
        var infos = WCDBError.Infos()
        var extInfos = WCDBError.ExtInfos()
        infos[WCDBError.Key.message] = ErrorValue(message)
        let enumerator: @convention(block) (UnsafePointer<Int8>, WCDBErrorValueType, Int, Double, UnsafePointer<Int8>) -> Void = {
            (ckey, valueType, intValue, doubleValue, stringValue) in
            let stringKey = String(cString: ckey)
            var value: ErrorValue
            switch valueType {
            case WCDBErrorValueTypeInterger:
                value = ErrorValue(intValue)
            case WCDBErrorValueTypeFloat:
                value = ErrorValue(doubleValue)
            case WCDBErrorValueTypeString:
                value = ErrorValue(String(cString: stringValue))
            default:
                return
            }
            let key = WCDBError.Key(stringKey: stringKey)
            if key != .invalidKey {
                infos[key] = value
            } else {
                extInfos[stringKey] = value
            }
        }
        let imp = imp_implementationWithBlock(enumerator)
        WCDBErrorEnumerateAllInfo(recycleError.raw, imp)
        return WCDBError(level: level ?? .Error, code: code ?? .Error, infos: infos, extInfos: extInfos)
    }

    @discardableResult
    static internal func report(level: WCDBError.Level, code: WCDBError.Code, infos: WCDBError.Infos) -> WCDBError {
        var cppLevel: WCDBErrorLevel = WCDBErrorLevel_Ignore
        switch level {
        case .Ignore:
            cppLevel = WCDBErrorLevel_Ignore
        case .Debug:
            cppLevel = WCDBErrorLevel_Debug
        case .Warning:
            cppLevel = WCDBErrorLevel_Warming
        case .Notice:
            cppLevel = WCDBErrorLevel_Notice
        case .Error:
            cppLevel = WCDBErrorLevel_Error
        case .Fatal:
            cppLevel = WCDBErrorLevel_Fatal
        }
        let error = WCDBError(level: level, code: code, infos: infos)
        WCDBErrorReport(cppLevel, error.code.rawValue, error.message, error.path, error.tag ?? 0)
        return error
    }
}
