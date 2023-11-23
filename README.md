# WCDB使用(这儿使用的是2.0.4版本)

```
/*
 * 这儿对WCDB的操作打开了监听日志的，可留意控制台打印信息
 * 对WCDB的任何操作都依赖于模型绑定，模型首先要遵循TableCodable协议，需要存储的值需要在enum CodingKeys: String, CodingTableKey{}里面case 记录，static let objectRelationalMapping = TableBinding(CodingKeys.self) {}方法里设置你对数据库存储值的设定
 * WCDB数据库一般情况下不需要开发者手动调用开关。操作时自动打开，当没有指向 Database 所共享的 Core 时，数据库会自动关闭，并回收内存
 * 具体更多信息自己阅读WCDB文档
 */

//MARK: - 增
private func addMenuItems() -> UIMenu {
    let addAction1 = UIAction.init(title: "insert") { _ in
        let object = Sample()
        object.identifier = 1
        object.description = "abc"
        //纯插入操作，由于设置了identifier为主键，所以identifier必须唯一，不然插入必失败并打印错误
        try? DBManager.shared.db.insert(object, intoTable: "\(Sample.self)")
    }
    let addAction2 = UIAction.init(title: "insertOrReplace") { _ in
        let object = Sample()
        object.identifier = 1
        object.description = "abcd"
        //当主键重复时即为更新，不重复时即直接插入数据
        try? DBManager.shared.db.insertOrReplace(object, intoTable: "\(Sample.self)")
    }
    let addAction3 = UIAction.init(title: "insertOrIgnore") { _ in
        let object = Sample()
        object.identifier = 1
        object.description = "abcdefg"
        //当出现主键重复时，直接忽略此操作，不重复时才插入
        try? DBManager.shared.db.insertOrIgnore(object, intoTable: "\(Sample.self)")
    }
    let addAction4 = UIAction.init(title: "带自定义对象insert") { _ in
        let object = Sample()
        //由于设置了主键identifier为自增，这儿identifier会视数据库中的最大值自增
        object.description = "自定义对象存储"
        guard let data = try? JSONEncoder().encode(["variable1": "1","variable2": "2"]) else {return}
        let cuModel = Customer(with: Value(data))
        object.myClass = cuModel
        //这儿的Customer自定义对象我们使用二进制存储，存储自定义对象只需要遵循ColumnCodable协议，且实现init?(with Value)和archivedValue方法，这两个方法相当于是解归档
        try? DBManager.shared.db.insert(object, intoTable: "\(Sample.self)")
    }
    let addAction5 = UIAction.init(title: "Sample设置了description不为空而传入空值") { _ in
        let object = Sample()
        object.identifier = 10
        //Sample设置了description不可为空，这样插入会失败且报错
        try? DBManager.shared.db.insert(object, intoTable: "\(Sample.self)")
    }
    return UIMenu.init(children: [addAction1,addAction2,addAction3,addAction4,addAction5])
}

//MARK: - 删
private func deleteMenuItems() -> UIMenu {
    let deleteAction1 = UIAction.init(title: "delete") { _ in
        //delete(fromTable table: String,where condition: Condition? = nil,orderBy orderList: [OrderBy]? = nil,limit: Limit? = nil,offset: Offset? = nil)，这五个组合起来可以理解为：将 table 表内，满足 condition 的数据，按照 orderList 的方式进行排序，然后从头开始第 offset 行数据后的 limit 行数据删除，各参数代表的含义：table：表名,condition：条件，这儿可以多个条件合并，orderList：排序规则，这儿是数组，可以有多个列的排序规则，limit：无offset时理解为前 limit 行，有offset时理解为后 limit 行，offset：前 offset 行
        try? DBManager.shared.db.delete(fromTable: "\(Sample.self)",where: Sample.Properties.identifier > 2 && Sample.Properties.description == "abc",orderBy: [Sample.Properties.identifier.asOrder().order(.descending)],limit: 2,offset: 3)
    }
    
    return UIMenu.init(children: [deleteAction1])
}

//MARK: - 改
private func changeMenuItems() -> UIMenu {
    //propertyConvertibleList、condition、limit 和 offset 前面介绍过了，两个更新不同的在with参数，on后的参数代表需要更新的列
    let changeAction1 = UIAction.init(title: "基于对象的更新(update with object)") { _ in
        let object = Sample()
        object.description = "byObject"
        
        try? DBManager.shared.db.update(table: "\(Sample.self)", on: [Sample.Properties.description], with: object,where: Sample.Properties.identifier > 1)
    }
    let changeAction2 = UIAction.init(title: "基于值的更新(update with row)") { _ in
        let row: [ColumnCodable] = ["byRow"]
        //当on后的参数包含myClass而未设定myClass的值时，会置空
        try? DBManager.shared.db.update(table: "\(Sample.self)", on: [Sample.Properties.description,Sample.Properties.myClass], with: row, where: Sample.Properties.identifier == 10000)
    }
    
    return UIMenu.init(children: [changeAction1,changeAction2])
}

//MARK: - 查

/*
 * 若是部分列查询只获取了 identifier 字段，而没有获取 description 的值。这就可能与 Swift 本身存在冲突。 Swift 规定了对象创建时，必须初始化所有成员变量。而进行对象部分查询时，则可能出现某部分变量没有变查询，因此无法初始化的情况。因此，对于可能不被查询的成员变量，应将其类型定义为可选值。 对于 Sample 类中，"getObjects" 接口虽然没有获取 description 的值，但由于 description 是 String? 类型，因此不会出错。 而将`var description: String?` 改为 `var description: String`就会出错,这儿使用部分查询时模型绑定里的值一定要都设置为可选值
 * 只有getObjects和getObject为对象查询，其余都是值查询
 * "getRows" 接口获取整个矩阵的所有内容，即返回值为二维数组。
 * "getRow" 接口获取某一横行的数据，即返回值为一维数组。
 * "getColumn" 接口获取某一纵列的数据，即返回值为一维数组。
 * "getDistinctColumn" 与 "getColumn" 类似，但它会过滤掉重复的值。
 * "getValue" 接口获取矩阵中某一个格的内容。
 * "getDistinctValue" 与 "getValue" 类似，但它会过滤掉重复的值。
 */

private func checkMenuItems() -> UIMenu {
    let checkAction1 = UIAction.init(title: "getObjects(全部列的查询)") { _ in
        do {
            let allObjects: [Sample] = try DBManager.shared.db.getObjects(on: Sample.Properties.all, fromTable: "\(Sample.self)")
            print("getObjects(全部列的查询)",allObjects)
        } catch {

        }
    }
    let checkAction2 = UIAction.init(title: "getObject(全部列的查询)") { _ in
        do {
            //"getObject" 等价于 limit: 1 时的 "getObjects" 接口
            let object: Sample? = try DBManager.shared.db.getObject(on: Sample.Properties.all, fromTable: "\(Sample.self)")
            if let obj = object {
                print("getObject(全部列的查询)",obj.identifier,obj.description,obj.myClass)
            } else {
                print("getObject(全部列的查询)obj为空")
            }
        } catch {

        }
        
    }
    let checkAction3 = UIAction.init(title: "getObjects(部分列的查询)") { _ in
        do {
            let allObjects: [Sample] = try DBManager.shared.db.getObjects(on: [Sample.Properties.identifier,Sample.Properties.myClass], fromTable: "\(Sample.self)")
            print("getObjects(部分列的查询)",allObjects)
        } catch {

        }
    }
    let checkAction4 = UIAction.init(title: "getRows(全部列的查询)") { _ in
        do {
            let allRows = try DBManager.shared.db.getRows(on: Sample.Properties.all, fromTable: "\(Sample.self)")
            print("getRows(全部列的查询)",allRows)
            //row column 代表查询到的值里面第 row 行 第 column 列
            print("getRows(全部列的查询)",allRows[row: 0, column: 0].int64Value)
        } catch {
            
        }
    }
    let checkAction5 = UIAction.init(title: "getRow(全部列的查询)") { _ in
        do {
            //若是带 offset 参数表示查询第 offset+1 行
            let row = try DBManager.shared.db.getRow(on: Sample.Properties.all, fromTable: "\(Sample.self)",offset: 1)
            print("getRow(全部列的查询)",row)
            print("getRow(第二行第1列值的查询)",row[1].stringValue)
        } catch {
            
        }
    }
    let checkAction6 = UIAction.init(title: "getColumn(部分列的查询)") { _ in
        do {
            //获取 description 列
            let descriptionColumn = try DBManager.shared.db.getColumn(on: Sample.Properties.description, fromTable: "\(Sample.self)")
            print("getColumn(description列的查询)",descriptionColumn)
            print("getColumn(description列第0个的查询)",descriptionColumn[0].stringValue)
        } catch {
            
        }
    }
    let checkAction7 = UIAction.init(title: "getDistinctColumn(部分列的查询)") { _ in
        do {
            //获取 description 列
            let distinctDescriptionColumn = try DBManager.shared.db.getDistinctColumn(on: Sample.Properties.description, fromTable: "\(Sample.self)")
            print("getDistinctColumn(description列的查询)",distinctDescriptionColumn)
            print("getDistinctColumn(description列第0个的查询)",distinctDescriptionColumn[0].stringValue)
        } catch {
            
        }
    }
    let checkAction8 = UIAction.init(title: "getValue(部分列的查询)") { _ in
        do {
            //获取 description 列，无offset参数代表第一行，有即 offset+1行
            let value = try DBManager.shared.db.getValue(on: Sample.Properties.description, fromTable: "\(Sample.self)")
            print("getValue(description列第一行的查询)",value)
            print("getValue(description列第一行值的查询)",value.stringValue)
        } catch {
            
        }
    }
    let checkAction9 = UIAction.init(title: "getValue(identifier最大值的查询)") { _ in
        do {
            //获取 identifier最大值
            let value = try DBManager.shared.db.getValue(on: Sample.Properties.identifier.max(), fromTable: "\(Sample.self)")
            print("getValue(identifier最大值的查询)",value)
            print("getValue(identifier最大值的查询)",value.int64Value)
        } catch {
            
        }
    }
    let checkAction10 = UIAction.init(title: "getDistinctValue(不重复的description查询)") { _ in
        do {
            //获取不重复的description的值
            let value = try DBManager.shared.db.getDistinctValue(on: Sample.Properties.description, fromTable: "\(Sample.self)")
            print("getDistinctValue(不重复的description查询)",value)
            print("getDistinctValue(不重复的description查询)",value.stringValue)
        } catch {
            
        }
    }
    
    return UIMenu.init(children: [checkAction1,checkAction2,checkAction3,checkAction4,checkAction5,checkAction6,checkAction7,checkAction8,checkAction9,checkAction10 ])
}

//MARK: - 表

/* 表相当于指定了表名和模型绑定类的 Database，其实质只是后者的简化版。增删查改中提到的所有接口Table都具备，而且这些接口调用时都不需要再传表名和 ORM 类型，因为执行数据读写时Table使用起来比Database更加简洁，而且也有利于以表为单位来管理数据读写逻辑，所以WCDB推荐尽量使用Table来进行数据读写。
 */

private func tableMenuItems() -> UIMenu {
    //下面增删改查分别写个示例
    let table = DBManager.shared.db.getTable(named: "\(Sample.self)",of: Sample.self)
    let tableAction1 = UIAction.init(title: "增") { _ in
        let object = Sample()
        object.identifier = 55
        object.description = "增"
        try? table.insert(object)
    }
    let tableAction2 = UIAction.init(title: "删") { _ in
        try? table.delete(limit: 1)
    }
    let tableAction3 = UIAction.init(title: "改") { _ in
        let object = Sample()
        object.description = "改"
        try? table.update(on: Sample.Properties.description, with: object,where: Sample.Properties.identifier > 55)
    }
    let tableAction4 = UIAction.init(title: "查") { _ in
        do {
            let objects:[Sample] = try table.getObjects(on: Sample.Properties.all)
            print("表查询",objects)
        } catch {
            
        }
    }
    
    return UIMenu.init(children: [tableAction1,tableAction2,tableAction3,tableAction4])
}

//MARK: - 事务

/*
 * 事务一般用于 提升性能 和 保证数据原子性。Database 和 Table 都能直接发起事务，也可以通过 Transaction 更好地控制事务
 * 事务提升性能的实质是批量处理
 * 在多线程下，删除操作发生的时机是不确定的。倘若它发生在 插入完成之后 和 取出数据之前 的瞬间，则 getObjects() 无法取出刚才插入的数据，且这种多线程低概率的 bug 是很难查的。而事务可以保证一段操作的原子性
 */

private func transactionMenuItems() -> UIMenu {
    let table = DBManager.shared.db.getTable(named: "\(Sample.self)",of: Sample.self)
    let transAction1 = UIAction.init(title: "多个对象单独插入") { _ in
        let object = Sample()
        object.description = "多个对象单独插入"
        let objects = Array(repeating: object, count: 100000)
        
        for object in objects {
            try? table.insert(object)
        }
    }
    let transAction2 = UIAction.init(title: "多个对象事务插入") { _ in
        let object = Sample()
        object.description = "多个对象事务插入"
        let objects = Array(repeating: object, count: 100000)
        
        //insert(objects:) 接口内置了事务，并对批量数据做了针对性的优化，性能更好
        
        try? DBManager.shared.db.run(transaction: { _ in
            for object in objects {
                try? table.insert(object)
            }
        })
    }
    //在多线程下，删除操作发生的时机是不确定的。倘若它发生在 插入完成之后 和 取出数据之前 的瞬间，则 getObjects() 无法取出刚才插入的数据，且这种多线程低概率的 bug 是很难查的。
    let transAction3 = UIAction.init(title: "插入查询不写在事务中") { _ in
        DispatchQueue(label: "other thread").async {
            try? table.delete()
        }
        let object = Sample()
        object.description = "不写在事务中"
        try? table.insert(object)
        do {
            let objects = try table.getObjects(on: Sample.Properties.all)
            print("插入查询不写在事务中",objects.count) // 值不固定说明先后顺序不确定
        } catch {
            
        }
    }
    let transAction4 = UIAction.init(title: "插入查询都写在事务中") { _ in
        DispatchQueue(label: "other thread").async {
            try? table.delete()
        }

        try? DBManager.shared.db.run(transaction: { _ in
            let object = Sample()
            object.description = "都写在事务中"
            try? table.insert(object)
            do {
                let objects = try table.getObjects(on: Sample.Properties.all)
                print("插入查询都写在事务中",objects.count) // 输出1
            } catch {
                
            }
        })
    }
    //WCDB Swift 提供了四种事务，普通事务、可控事务、嵌入事务和可中断事务
    //上面的transAction1和transAction2皆为普通事务
    let transAction5 = UIAction.init(title: "可控事务") { _ in
        let object = Sample()
        object.description = "可控事务"
        let objects = Array(repeating: object, count: 10)
        //由于事务是批量处理，所以i == 3返回 false即回滚了数据，也就一个数据都没插入
        try? DBManager.shared.db.run(controllableTransaction: { _ in
            for i in 0...objects.count-1 {
                let obj = objects[i]
                if i == 3 {
                    return false
                }
                try? table.insert(obj)
            }
            return true
        })
    }
    
    let transAction6 = UIAction.init(title: "嵌入事务") { _ in
        let object = Sample()
        object.description = "嵌入事务"
        let objects = Array(repeating: object, count: 10)
        try? DBManager.shared.db.run(transaction: { _ in
            try? DBManager.shared.db.run(controllableTransaction: { _ in
                for i in 0...objects.count-1 {
                    let obj = objects[i]
                    if i == 3 {
                        return false
                    }
                    try? table.insert(obj)
                }
                return true
            })
        })
    }
    //在需要对数据库进行大量数据更新的场景，我们的开发习惯一般是将这些更新操作统一到子线程处理，这样可以避免阻塞主线程，影响用户体验。为了解决大事务会阻塞主线程的问题，WCDB 才加入了可中断事务。可中断事务把一个流程很长的事务过程看成一个循环逻辑，每次循环执行一次短时间的DB操作。操作之后根据外部传入的参数判断当前事务是否可以结束，如果可以结束的话，就直接Commit Transaction，将事务修改内容写入磁盘。如果事务还不可以结束，再判断主线程是否因为当前事务阻塞，没有的话就回调外部逻辑，继续执行后面的循环，直到外部逻辑处理完毕。如果检测到主线程因为当前事务阻塞，则会立即 Commit Transaction，先将部分修改内容写入磁盘，并唤醒主线程执行DB操作。等到主线程的DB操作执行完成之后，在重新开一个新事务，让外部可以继续执行之前中断的逻辑。
    let transAction7 = UIAction.init(title: "可中断事务") { _ in
        var objects: [Sample] = []
        for i in 0..<100 {
            let obj = Sample()
            obj.identifier = i
            obj.description = "可中断事务\(i)"
            objects.append(obj)
        }

        DispatchQueue(label: "other thread").async {
            do {
                var index = 0
                try DBManager.shared.db.run(pausableTransaction: { handle, stop, isNewTransaction in
                    // isNewTransaction表示第一次执行，或者事务在上次循环结束之后被中断提交了
                    if isNewTransaction {
                        //新事务先建一下表，避免事务被中断之后，表已经被其他逻辑删除
                        try handle.create(table: "sampleTable", of: Sample.self)
                    }
                    //写入一个对象，这里还可以用PreparedStatement来减少SQL解析的耗时
                    try handle.insert(objects[index], intoTable: "sampleTable")
                    
                    index += 1
                    //给stop赋值成true表示事务结束
                    stop = index >= (objects.count-3)
                })
            } catch {
                print("Transaction failed with error: \(error)")
            }
        }
    }
    
    return UIMenu.init(children: [transAction1,transAction2,transAction3,transAction4,transAction5,transAction6,transAction7])
}

//MARK: - 语言集成查询

/*
 * 语言集成查询使得开发者能够通过 Swift 的语法特性去完成 SQL 语句。
 */

private func integratedQueryMenuItems() -> UIMenu {
   
    let integratedQueryAction1 = UIAction.init(title: "增") { _ in
        let statementInsert = StatementInsert().insert(intoTable: "\(Sample.self)").columns(Sample.Properties.identifier).values(99)
        print(statementInsert.description) // 输出 "INSERT INTO Sample(identifier) VALUES(99)"
    }
    let integratedQueryAction2 = UIAction.init(title: "删") { _ in
        let statementInsert = StatementDelete().delete(from: "\(Sample.self)").where(Sample.Properties.identifier > 5)
        print(statementInsert.description) // 输出 "DELETE FROM Sample WHERE id > 5"
    }
    let integratedQueryAction3 = UIAction.init(title: "改") { _ in
        let statementInsert = StatementUpdate().update(table: "\(Sample.self)").where(Sample.Properties.identifier == 1).set(Sample.Properties.description).to("语言集成查询改值")
        print(statementInsert.description) // 输出 "UPDATE Sample SET description = '语言集成查询改值' WHERE id == 1"
    }
    let integratedQueryAction4 = UIAction.init(title: "查") { _ in
        let statementInsert = StatementSelect().select(Sample.Properties.description).from("\(Sample.self)").where(Sample.Properties.identifier > 5)
        print(statementInsert.description) // 输出 "SELECT description FROM Sample WHERE id > 5"
    }
    
    
    return UIMenu.init(children: [integratedQueryAction1,integratedQueryAction2,integratedQueryAction3,integratedQueryAction4])
}

//MARK: - Other

/*
 * 数据库升级
 *
 * WCDB的数据库升级很简单，我们知道不销毁表的情况下，无法对列直接进行删除，所以WCDB做了这样的处理
 * 直接在模型绑定里进行列的是否存储操作，新加列可以在case 里加上，当db.create(table:of:)调用时会自动在表里新增列，'删除'列可在case 里删除,当db.create(table:of:)调用时会自动在表里忽略此列，但是表里此列并没有删除，且以前此列的值不会删除
 */


/*
 * 数据库操作
 *
 * database.purge()即回收 database 数据库中暂不使用的内存
 * Database.purge()即回收所有已创建的数据库中暂不使用的内存
 * 在 iOS 平台上，当内存不足、收到系统警告时，WCDB Swift 会自动调用 Database.purge()  接口以减少内存占用
 * 某些情况下，开发者需要确保数据库完全关闭后才能进行操作，如移动文件操作
 * 可以使用以下方式保证是在WCDB数据库关闭后做的操作
 * try? database.close(onClosed: {
 *     try database.moveFiles(toDirectory: otherDirectory)
 * })
 */


/*
 * 文件与代码模版
 *
 * 模型绑定的大部分都是格式固定的代码，因此，WCDB Swift 提供了文件模版和代码模版两种方式，以简化模型绑定操作。 文件和代码模版都在源代码的 tools/templates 目录下
 * 这儿我们使用文件模版，首先获取 WCDB 的 Github 仓库，在命令行如下操作：
 * cd path-to-your-wcdb-dir/tools/templates        //进入wcdb项目里找到tools->templats目录，拖进去
 * sh install.sh                                   //这儿是把文件工具导入Xcode创建新文件里
 * 文件模版安装完成后，在 Xcode 的菜单 File -> New -> File... 中创建新文件，通用数据模版选择 TableCodable。 在弹出的菜单中输入文件名，并选择 Language 为 Swift 即可。
 * 自定义类型模版选择 ColumnCodable
 */

```
