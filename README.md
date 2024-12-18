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
- 这一个`Lab`感觉是我最有收获的一个`Lab`了,一方面让我对于进程编程中的信号处理机制有了更加深入的理解,让进程编程的知识得到了实践,同时也让我在不断的代码验证不断寻找问题并且解决问题中的过程中获得了成就感,这激发了我对于计算机系统的学习兴趣,同时让我明白了,代码写的不好不是设计思维不行,有时候知识知识点的欠缺
- 本`Lab`的一个最重要的主题就是 "进程同步" (也就是多个进程访问同样一个共享数据的时候产生的安全问题),这也是我本实验中收获最大的一个知识点,之前总是关注线程同步,这里的进程同步也是一样的,需要利用屏蔽信号集的方式进行进程同步
- 加强了我对于进程编程的掌握程度,深入理解了`waitpid`函数的参数还有各种信号的作用,比如(`SIGCONT` , `SIGINT` , `SIGTSTP`等)
- 同时也对于`fork`出来的子进程的信号处理方式还有`exec`系列函数有了一定的认识
- 实验记录: [lab6](lab6/shlab/lab6.md)
## Lab7 MallocLab
- 花了大概一天半多一点的时间做完了这一个`Lab`,收获还是很多的,首先就是学会了有耐心的`Debug`,这里利用`gdb`看不到源代码,所以只可以在程序中使用`printf`把关键的变量的值打印出来进行查看,以便查看还需要画出堆中存储的变量的地址和链表中指针的指向,这比较需要耐心,特别是写显示链表的时候,看似代码写的没有问题,但是最后找出来十几个`BUG`
- 通过这一章的学习,我了解了虚拟内存这一概念,虚拟内存存储在物理内存中,物理内存有时磁盘的一个缓存,`CPU`读取指令或者内存中的数据的时候会发送一条虚拟地址,把这一个虚拟地址发送给`MMU`,`MMU`会解析这一个虚拟地址成物理地址,并且把物理地址发送给内存控制器,内存控制器从内存中取出对应的数据来通过总线传递给`CPU`(会先从高速缓存中查询数据,高速缓存中地址的存储方式是物理地址),如果过高速缓存中没有命中,那么就需要在内存中寻找这一个地址,如果这一个页还没有分配,那么就会触发缺页处理程序处理缺页问题,也就是在物理内存中选取一个牺牲页,处理完成之后`CPU`重新发送指令,这一个过程中还会有`MMU`的`TLB`缓存用于加速查询,并且一个虚拟地址在不同的硬件的眼中的翻译方式不一样,这也联系了之前的讲缓存的那一章节
- 为了提升空间利用率,还引出了多级也表的概念,相当层层套娃,一层中包含了`1K`个子表等
- 本章还从程序运行的视角说明的虚拟内存的重要作用,程序运行是,虚拟内存抽象出程序独占内存的假象,并且依赖于虚拟内存可以实现程序变量到磁盘内存的映射`mmap`,并且实现了读写控制,内核区维护的`task_struct`结构体中的成员`mm_struct`结构体中的各种成员表示了一个程序中不同的部分(栈,堆,`.text`,`.data`,`.bss`,共享库)映射到虚拟内存的位置,并且通过地址表示为实现了权限控制,另外我还学习到了`fork`和`exec`实现子进程调用过程中虚拟内存的变化
- 最后本章讲解了如何实现一个动态分配器(`malloc`),同时还讲解了一些高级语言中实现`gc`的方法,并且在这一个`lab`中,我使用了三种方式实现了`malloc`
- 实验记录: [`lab7`](lab7/malloclab/lab7.md)

## Lab8 ProxyLab
- 在这一个`Lab`中我学习到了一个服务器如何解析`http`请求并且如何构建`http`请求从而完成和客户端的通信,同时学习到了利用预线程化的技术来实现 生产者消费者模型,另外回顾了`LRU`缓存
- 通过后面几个章节的学习,我了解了网络编程,了解到了三种构建并发程序的方式(进程,`IO`多路复用,线程),了解到了`IO`多路复用其实就是可以同时监听不同的时间的发生,但是书中只是简单介绍了`select`,另外的`IO`多路复用模型我会在只会继续学习,另外最后的并发编程中虽然只是简单的介绍了多线程编程中的`API`但是它介绍了 `PV`信号量的操作(让我对于锁又有了更加深入的了解),并且将了如何利用进程图和`PV`操作判断死锁,让我对于死锁的形成有了一定的了解,同时介绍了读者-写者模型和利用预线程化的思想实现`Web`服务器这些都对于我的帮助很大
- 实验记录: [`lab8`](lab8/proxylab/lab8.md)

## 总结
- `9`月`21`日到`10`月`27`日,我终于看完了这一本神书,虽然还是有很多东西不太理解(比如第五章中的程序执行时的关键路径以及指令重排序,以及多处理器章节中的解决冒险和竞争的方式,还有第七章在利用可重定位文件生成可执行文件时如果确定地址等问题,第七章基本没有看太多),但是我感觉之前很多困扰我的东西(比如指针类型,`C`语言的内存结构,`IO`多路复用的含义,`PV`操作,虚拟内存等概念我逐渐有了一个比较清晰的认识),同时这也是我第一次体验 课程(书籍)+`Lab`的学习方式,很多我在书中不注意的知识点(比如`CacheLab`中根据物理内存进行寻址,在`ProxyLab`中`Rio`报中的`IO`函数以及在`BombLab`中对于`gdb`的使用的不理解)都对于我对于`Lab`的完成有了比较大的帮助,同时也让我学习到了很多新的只是(比如`ShellLab`中的进程同步等),让我对于计算机体系结构有了一个大致的认知,同时也让我找到了一种新的学习路径,不再是跟着某马,某硅谷的视频学习各种框架,~~抄各种项目~~,而是在对于课程有一定的理解的情况下自己想解决方案,不断体会各种知识点踩各种坑,失败有什么可怕的,代码写的不好报错,大不了从头再来(实际上我已经做过很多次了),另外我自己的心态也是得到了锻炼,从开头基本上手足无措的`DataLab`到之后可以根据自己理解写出的`MallocLab`和`ProxyLab`我认为就是一种进步,希望之后的自己也可以一直坚持下去!
- 下一步准备学习操作系统: `OSTEP` + `Mit6.s081`