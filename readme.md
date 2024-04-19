30_是为了处理30中的bug而创建的。

    /*
     * bug描述：
     * 在使用工作线程的情况下，若运算后的message长度>=16，SendInLoop执行时发现data指向的内存异常。
     *
     * 原因解析： 
     * string的内存机制：
     *      字符串长度<16时，只使用栈内存，string.data()指向&string的后一个八字节；
     *      字符串长度>=16时，string.data()指向堆内存。
     * 因此，若运算后的message长度>=16，data()指向堆内存；
     * Send()返回后，EchoServer::OnMessage立马结束，那么EchoServer::OnMessage中的message对象就会释放堆内存；
     * 此时放在IO线程任务队列中的SendInLoop被取出开始执行，其中的字符指针data指向原来message对象的堆内存（但已经被释放了）
     * 
     * 解决方法：
     * 既然难以阻止EchoServer::OnMessage中的message对象释放，那就在调用SendInLoop时传入string副本。
     */
