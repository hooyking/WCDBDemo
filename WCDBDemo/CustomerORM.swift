// 
//  CustomerORM.swift
//  WCDBDemo
//
//  Created by hooyking on 2023/11/15.
//

import Foundation
import WCDBSwift

class CustomerORM: ColumnCodable {
    static var columnType: ColumnType {
        return .BLOB
    }

    required init?(with value: Value) {
		
    }

    func archivedValue() -> Value {
        return Value(nil)
    }
}
