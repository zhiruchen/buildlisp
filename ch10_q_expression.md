# ch10 QExpression
## 添加语言新特性的四个典型步骤
```
1. Syntax(语法)
为这个特性添加新的语法规则
2. Representation(表示)
添加代表这特性的新的数据类型变化
3. Parsing(解析)
添加从抽象语法树中读取新特性的函数
4. Semantics
添加对这个新特性进行求值和操作的函数
```
## 添加检测参数数量不正确的Macro
## 添加检测被空列表调用的Macro
## cons, len, init
```
cons:
len: 返回QExpr的元素数量
init: 返回QExpr中所有(除了最后一个)的元素
```
