# csapp
## Lab1 dataLab
- 学习到了位运算的相关知识,以及`IEEE`标准下的浮点表示,以及`C`语言中有符号整数和无符号整数的二进制表示方式
## Lab2 bombLab
- [Lab2](lab2/lab2.md)
- 虽然有两个`phase`没有做出来(`phase_6`和隐藏关卡),但是学习到了`gdb`调试器的使用和各种不同的数据结构在内存中的存储方式,`x86`汇编语言以及寻址方式,理解了`mov`和`lea`指令的作用
- 通关截图:
![alt text](./img/image.png)
## Lab3 attackLab
- [Lab3](lab3/attacklab/lab3.md)
- 虽然本`Lab`基本是 ~~抄~~ 的,但是还是有很多收获:
- 深入理解了`ret`,`pushq`,`popq`指令的用途,注意后面的两个指令的作用,`pushq`的作用也就是同时减少`%rsp`并且把后面一个寄存器的值放到栈底元素的位置,`popq`保存的就是原来的栈底的值,他们都可以把,`ret`指令可以跳转到 `%rsp` 指向的地址指示的指令位置执行这一个指令并且把`%rsp`减少的操作和栈空间无关(`subq,addq`)
- `Code-injection-attack`: 由于`ret`指令总是找到`%rsp`指令并且取出`%rsp`指令的内容(也就是指令的地址),所以如果通过操作`%rsp`指针的方式来存放指令,那么就可以让程序执行我们书写的指令而不去执行原来需要执行的指令,这一个技术的实现依赖于栈溢出异常
- `Return-orient-attack`: 由于栈空间是不断变化的,所以插入已经知道的指令的方式是难以实现的,所以只用利用已经存在的代码的方式进行攻击,比如如果一个指令的后面带有`ret`指令那么就可以依赖于`ret`指令完成跳转,这一种攻击方法依赖于程序中的指令的各个字节的分布是连续的,可以取得各个字节的空间(注意内存和栈空间的区别),程序在运行时可能产生栈空间,但是指令存储于内存中,所以指令的地址一般是不变化的,所以可以利用指令进行空间
- 以上的两种方法都依赖于栈溢出异常,所以在书写程序是需要避免这一个异常,`CPU`其实就是一个不断取得指令并且执行指令的机器,并不会智能的判断是否执行到正确的指令
- 比如`phase_5`和`phase_3`中一定需要注意把数据存储到指定的位置,如果返回一个值,那么此时是利用`mov`指令进行值赋值,所以可以保留原来的值,但是利用地址传递的方式,对应地址的栈空间会被释放,所以不可取(一定需要把数据包存在安全的栈空间的位置)
- 学会使用`cgdb`工具(基本和`gdb`一样)
## Lab4 archlab
- 实验的具体内容: [lab4](lab4/archlab/lab4.md)
- 这一个实验让我对于处理器的体系结构有了更加深刻的认识,我的收获如下:
1. 对于各种指令(虽然书中利用自己模拟的`Y86-64`指令进行讲解,但是还是和`X86`中的指令类似)有比较深刻的认识,了解了每一个指令在处理器执行的某一个阶段的某一些作用,比如`ret,pushq,popq`指令,对于程序的结构组成有一个比较清晰的认识(比如`PartA`)中栈的结构其实在代码片段之下,另外还认识到了从汇编指令到机器指令之间的变化,明确的指令中各种字段的提取规则
2. 对于处理器处理指令的各个阶段有了一定的认知(取指,译码,写回,执行,访存,更新 `PC`),同时对于顺序执行的结构和五级流水线模型有一定的了解
3. 对于程序对于各种异常情况的处理,比如利用转发处理数据冒险(比如利用没有写入内存或者寄存器文件的数据进行计算操作),分支预测错误之后的处理方式(气泡),明确了气泡和暂停之间的区别,了解了利用两种方式来解决各种异常情况的方法
4. 明白了其实对于指令集内部的各种指令的实现不用依赖于各种复杂的判断和逻辑,只需要指定各种指令中信号的来源,并且明确指令操作的数据类型和指令本身的类型,明确指令的去向等信息即可,利用硬件描述语言进行编程的时候,也只是需要注意各种信号在各种组件之间的传递(处理器,寄存器文件,高速缓存(指令高速缓存和数据高速缓存))即可,特别注意`dstE`和`dstM`之前的区别
5. 明确了影响程序运行效率的因素大多数事件是由于数据冒险导致的内存读写的顺序性,了解了各种优化方式(比如循环展开,减少内存引用以及调整运算顺序的方式的作用机理),同时了解了各种指令对于程序性能的影响,了解了利用数据流图分析程序的方法
## Lab5 cacheLab
- 通过这一章的学习,我对于缓存的存储器的体系结构的人年时更加清晰了,了解了各种缓存用的存储器(包含`SRAM`,`DRAM`,`ROM`,闪存等)
- 编写程序时需要遵循局部性原理,考虑时间局部性和空间局部性,确保变量在高速缓存中可以被多次使用并且缓存时间最新
- 了解了`LRU`缓存,和实现`LRU`缓存的方法,在`PartA`中实现了`LRU`缓存并且进行测试
- 了解了优化缓存命中率的几种方法,包含分块等方法
- 了解了如何利用`CPU`传过来的地址进行高速缓存的寻址操作(并且这一种利用地址进行寻址操作的方法需要在实践中模拟,比如`64 * 64`矩阵中的冲突情况就是这样分析出来的)
- ~~真正开始写程序才发现自己有多🥬~~,另外注意仔细看文档,~~不要应为没有看到`64`为地址而重写程序~~
- 实验记录: [lab5](lab5/cachelab/lab5.md)
## Lab6 shellLab
- 这一个`Lab`感觉是我最有收获的一个`Lab`了,一方面让我对于进程编程中的信号处理机制的`fork`机制有了更加深入的理解,让进程编程的知识得到了实践,同时也让我在不断的代码验证不断寻找问题并且解决问题中的过程中获得了成就感,这激发了我对于计算机系统的学习兴趣,同时让我明白了,代码写的不好不是设计思维不行,有时候知识知识点的欠缺
- 本`Lab`的一个最重要的主题就是 "进程同步" (也就是多个进程访问同样一个共享数据的时候产生的安全问题),这也是我本实验中收获最大的一个知识点,之前总是关注线程同步,这里的进程同步也是一样的,需要利用屏蔽信号集的方式进行进程同步
- 加强了我对于进程编程的掌握程度,深入理解了`waitpid`函数的参数还有各种信号的作用,比如(`SIGCONT` , `SIGINT` , `SIGTSTP`等)
- 同时也对于`fork`出来的子进程的信号处理方式还有`exec`系列函数有了一定的认识