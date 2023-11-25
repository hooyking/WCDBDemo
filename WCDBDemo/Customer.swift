// 
//  Customer.swift
//  WCDBDemo
//
//  Created by hooyking on 2023/11/15.
//

import Foundation
import WCDBSwift

class Customer: ColumnCodable {
    var variable1: String? = nil
    var variable2: String? = nil
    
    static var columnType: ColumnType {
        return .BLOB
    }

    required init?(with value: Value) {
        let data = value.dataValue
        guard data.count > 0 else {
            return nil
        }
        guard let dictionary = try? JSONDecoder().decode([String: String].self, from: data) else {
            return nil
        }
        variable1 = dictionary["variable1"] ?? ""
        variable2 = dictionary["variable2"] ?? ""
    }

    func archivedValue() -> Value {
        if let data = try? JSONEncoder().encode(["variable1": variable1,"variable2": variable2]) {
            return Value(data)
        }
        return Value(nil)
    }
}
