# Pointer
**指针是一个c语言中非常难以理解的概念。**
****
## 为什么在C中我们需要指针
**c语言中函数调用的参数是通过复制值的方式来传递的。**
**当我们传递的参数是很大的含有许多子结构体结构体时候，仅仅因为函数调用而需要复制大量的数据。**
**为了解决这个问题，c语言的创造者将内存想象成一个巨大的字节列表。在这个列表中每一个字节都有一个全局索引，在这个情况下，所有计算机中的数据都在这个巨大的列表中有一个开始的索引(位置)。在传递参数的时候，我们可以将代表这个数据的数字(在字节列表中的起始位置)传递给被调用的函数。被调用函数就可以查看任意大小的数据根据这个起始位置。**
**计算机的内存大小是固定的，代表一个地址的字节数也是固定的；但是地址指向的字节数却可以增长和减少。这意味我们可以创建一个变量大小的数据结构，仍将它传递给一个函数，而这个函数可以检查和修改它**
****
# `所以指针就是数字，一个代表内存中数据的起始索引的数字, 指针类型暗示我们和编译器 这个位置处可访问的数据的类型`
*****
## 获取数据的指针，获取指针指向的数据
```
int val = 100;
int* x = &val;
```
****
```
char ch = 'c';
char* chp = &ch;
```
# S 表达式
```
S 表达式就是括号括起来的多个表达式， '(' <expr>* ')'
而表达式可以是:  数字/操作符/其他s表达式
```
