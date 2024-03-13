# my-sylar
1.日志模块
==
支持流式日志风格写日志和格式化风格写日志，支持日志格式自定义，日志级别，多日志分离等等功能。<br>
流式日志使用：<br>
　　SYLAR_LOG_INFO(g_logger) << "this is a log";<br>
格式化日志使用：<br>
　　SYLAR_LOG_FMT_INFO(g_logger, "%s", "this is a log"); <br>
目前的日志支持自由配置日期时间，累计运行毫秒数，线程id，线程名称，协程id，日志线别，日志名称，文件名，行号。<br>
相关类：<br>
　　LogEvent:日志事件，用于保存日志现场，比如所属日志器的名称，日期时间，线程/协程id，文件名/行号等，以及日志消息内容。<br>
　　LogFormatter: 日志格式器，构造时可指定一个格式模板字符串，根据该模板字符串定义的模板项对一个日志事件进行格式化，提供format方法对LogEvent对象进行格式化并返回对应的字符串或流。<br> 
　　Logger: 日志控制器，用于写日志，包含名称，日志级别两个属性，以及数个LogAppender成员对象． Logger有自己的日志格式，可以按自己的格式进行输出．一个日志事件经过判断高于日志器自身的日志级别时即会启动Appender进行输出。日志器默认不带Appender，需要用户进行手动添加。<br>
　　LogAppender：日志输出器，用于将日志输出到不同的目的地，比如终端和文件等。LogAppender内部包含一个LogFormatter成员，提供log方法对LogEvent对象进行输出到不同的目的地。这是一个虚类，通过继承的方式可派生出不同的Appender，目前默认提供了StdoutAppender和FileAppender两个类，用于输出到终端和文件。<br>
　　LoggerManager：日志器管理类，单例模式，包含全部的日志器集合，并且提供工厂方法，用于创建或获取日志器。LoggerManager初始化时自带一个root日志器，这为日志模块提供一个初始可用的日志器。<br>
  
２．配置模块
===
采用约定优于配置的思想。定义即可使用。支持变更通知功能。使用YAML文件做为配置内容，配置名称大小写不敏感。支持级别格式的数据类型，支持STL容器(vector,list,set,map等等),支持自定义类型的支持（需要实现序列化和反序列化方法)。使用方式如下：<br>
　　static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =<br>
　　　　sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");<br>
定义了一个tcp连接超时参数g_tcp_connect_timeout，可以直接使用其getValue()方法获取参数值。当配置修改重新加载，该值自动更新。
上述配置格式如下：<br>
　　tcp:<br>
　　　　connect:<br>
　　　　　　timeout: 10000<br>
配置模块相关类：<br>
　　ConfigVarBase:配置参数的基类，有名字和描述两个属性，提供toString和fromString纯虚方法，配置参数的类型和值由继承类实现。<br>
　　ConfigVar：配置参数模板类，继承自ConfigVarBase。有三个模板参数：类型T,FromStr和ToStr。这两个模板参数是仿函数，共中FromStr用于将字符串转类型T，ToStr将类型T转字符串。这两个仿函数通过一个类型转换模板类和不同的片特化类来实现不同类型的序列化与反序列化。<br>
　　ConfigVar类包含了一个T类型的配置参数值成员和一个变更回调函数数组，以及toString/fromString函数实现。toString/fromString用到了模板参数FromStr和ToStr。此外，ConfigVar还提供了setValue/getValue方法用于设置/修改值，并触发回调函数数组，以及addListener/delListener方法用于添加或删除回调函数。<br>
　　Config：配置ConfigVar的管理类，管理所有ConfigVar对象。提供Lookup方法，根据参数名称查询配置参数。如果调用Lookup查询时同时提供了默认值和配置描述信息，那么在未找到对应的配置时，会创建一个新的配置项，这样就保证了配置模块定义即可用的特性。除此外，Config类还提供了LoadFromYaml方法，用于从YAML结点中加载配置。Config的全部成员变量和方法都是static类型，保证了全局只有一个实例。<br>
  
3.线程模块
===
线程模块封装了互斥量与锁，封装了pthread里面的一些常用功能，Thread,Semaphore,Mutex,RWMutex,Spinlock等对象。<br>
为什么不使用c++11里面的thread？<br>
　　不使用thread，是因为thread其实也是基于pthread实现的。并且C++11里面没有提供读写互斥量，RWMutex，Spinlock等，在高并发场景，这些对象是经常需要用到的。所以选择了自己封装pthread。<br>
线程同步类：　　　所有的锁均采用局部锁模板，在构造时获得锁，在析构时释放锁。
　　Semaphore：计数信号量，基于sem_t实现。<br>
　　Mutex:普通互斥锁，基于pthread_mutex_t实现。<br>
　　RWMutex:读写锁，基于pthread_rwlock_t实现。<br>
　　SpinLock：自旋锁，基于pthread_spinlock_t实现。<br>
　　CASLock: 原子锁，基于std::atomic_flag实现。<br>
Thread:线程类，构造函数传入线程入口函数和线程名称，线程入口函数类型为void()，如果带参数，则需要用std::bind进行绑定。线程类构造之后线程即开始运行，构造函数在线程真正开始运行之后返回。<br>

4.协程模块
===
协程：用户态的线程，相当于线程中的线程，更轻量级。目前该协程是基于ucontext_t来实现的。
Fiber：协程类，有INIT、HOLD、EXEC、TERM、READY、EXCEPT六种状态。有id、协程栈大小、协程状态、协程上下文、协程运行指针、协程运行函数等属性。有swapIn、swapOut方法，用于将协程切换到运行状态/后台，一个无参/有参构造函数，无参构造为每个线程第一个协程的构造，带参数的构造函数用于构造子协程，初始化子协程的ucontext_t上下文和栈空间，要求必须传入协程的入口函数，以及可选的协程栈大小。<br>

5.协程调度模块
===
协程调度器，管理协程的调度，内部实现为一个线程池，支持协程在多线程中切换，也可以指定协程在固定的线程中执行。是一个N-M的协程调度模型，N个线程，M个协程。重复利用每一个线程。<br>
start方法创建线程池，并将每个创建的线程绑定到run方法。<br>
run方法中，先把调度器设置到当前线程。<br>
　　1.查看任务队列中是否有任务<br>
　　2.若有协程任务，swapIn()到任务协程执行任务，执行过后将ft reset;<br>
　　2.若有回调任务，新创建一个协程执行。<br>
　　3.无任务，执行idle协程，idle协程让出当前协程，循环执行run方法，存在的问题：调度器在idle情况下会疯狂占用CPU。<br>

6.IO协程调度模块
===
继承自协程调度器,支持读写事件。封装了一个FdContext结构体，有fd句柄、EventContext类型的读写事件、已注册的事件等属性，内部封装了一个EventContext类型的结构体，有调度器scheduler、事件协程、事件回调函数等属性，写了一个triggerEvent方法，可以将该fd对应的读写事件通过调度器scheduler中的schedule方法触发。<br>

IOManager：IO协程调度类，有一个epoll句柄、一对管道（用来通知调度协程有新任务）、一个存放FdContext的数组等属性。<br>
　　addEvent方法：1.若要添加的事件已存在，则不添加；2.若不存在，则将要添加的事件加入epoll_wait，使用epoll_event的私有指针存储FdContext的位置，将其添加到监听树上用epoll_wait监听。3.修改fd_ctx,更新events和其中的event_ctx中的cb/fiber和scheduler。<br>
　　delEvent方法：1.判断fd是否在数组m_fdContexts中，若在，在数组中取出该fd对应信息，得到对应结构体信息为fd_ctx。2.对比fd_ctx的事件与要删除的事件，如果已存事件和要删除事件没有交集，返回false，若有交集，则说明交集事件就是要删除的事件。那么创建一个epoll_event，将fd，fd_ctx,event放入到epoll_event中，将其添加到监听树上用epoll_wait监听。3.修改fd_ctx,将对应删除事件的event重置。<br>
　　cancelEvent方法：1、2同delEvent一样3.触发一次事件，执行对应事件的triggerEvent<br>
　　cancelAll方法：删除fd_ctx中的所有事件，并执行所有事件(读写)triggerEvent<br>
　　tickle：判断是否有空余线程，若没有则返回，若有，则向m_tickleFd[1]写一个字符“T”,写过以后，会触发m_tickleFd[0]的读事件<br>
　　idle：使用epoll_wait实现，是一个epoll反应堆<br>

7.定时器模块
===
在IO协程调度器之上再增加定时器调度功能，也就是在指定超时时间结束之后执行回调函数。定时的实现机制是idle协程的epoll_wait超时，大体思路是创建定时器时指定超时时间和回调函数，然后以当前时间加上超时时间计算出超时的绝对时间点，然后所有的定时器按这个超时时间点排序，从最早的时间点开始取出超时时间作为idle协程的epoll_wait超时时间，epoll_wait超时结束时把所有已超时的定时器收集起来，执行它们的回调函数。支持一次性定时器，循环定时器，条件定时器等功能。<br>
　　定时器以gettimeofday()来获取绝对时间点并判断超时，所以依赖系统时间，如果系统进行了校时，比如NTP时间同步，那这套定时机制就失效了。sylar的解决办法是设置一个较小的超时步长，比如3秒钟，也就是epoll_wait最多3秒超时，如果最近一个定时器的超时时间是10秒以后，那epoll_wait需要超时3次才会触发。每次超时之后除了要检查有没有要触发的定时器，还顺便检查一下系统时间有没有被往回调。如果系统时间往回调了1个小时以上，那就触发全部定时器。<br>

8.Hook模块
===
hook系统底层和socket相关的API，socket io相关的API，以及sleep系列的API。hook的开启控制是线程粒度的。可以自由选择。通过hook模块，可以使一些不具异步功能的API，展现出异步的性能。hook实际就是把系统提供的api再进行一层封装，以便于在执行真正的系统调用之前进行一些操作。hook的目的是把socket io相关的api都转成异步，以便于提高性能。hook和io调度是密切相关的，如果不使用IO协程调度器，那hook没有任何意义。考虑IOManager要调度以下协程：<br>
协程1：sleep(2) 睡眠两秒后返回<br>
协程2：在scoket fd1上send 100k数据<br>
协程3：在socket fd2上recv直到数据接收成功<br>
在未hook的情况下，IOManager要调度上面的协程，流程是下面这样的：<br>
　　1.调度协程1，协程阻塞在sleep上，等2秒后返回，这两秒内调度器是被协程1占用的，其他协程无法被调度。<br>
　　2.调度协徎2，协程阻塞send 100k数据上，这个操作一般问题不大，因为send数据无论如何都要占用时间，但如果fd迟迟不可写，那send会阻塞直到套接字可写。<br>
　　3.调度协程3，协程阻塞在recv上，这个操作要直到recv超时或是有数据时才返回，期间调度器也无法调度其他协程。<br>
总而言之，协程只能按顺序调度一旦有一个协程阻塞住了，那整个调度器也就阻塞住了，其他的协程都无法执行。Hook后，IOManager的执行流程将变成下面的方式：<br>
　　1.调度协程1，检测到协程sleep，添加一个2秒的定时器，定时器的回调函数是在调度器上继续调度本协程，接着协程yield，等定时器超时。<br>
　　2.因为上一步协程1已经yield了，所以协徎2并不需要等2秒后才可以执行，而是立刻可以执行。同样，调度器检测到协程send，由于不知道fd是不是马上可写，所以先在IOManager上给fd注册一个写事件，回调函数是让当前协程resume并执行实际的send操作，然后当前协程yield，等可写事件发生。<br>
　　3.上一步协徎2也yield了，可以马上调度协程3。协程3与协程2类似，也是给fd注册一个读事件，回调函数是让当前协程resume并继续recv，然后本协程yield，等事件发生。<br>
　　4．等2秒超时后，执行定时器回调函数，将协程1 resume以便继续执行。<br>
　　5.等协程2的fd可写，一旦可写，调用写事件回调函数将协程2 resume以便继续执行send。<br>
　　6.等协程3的fd可读，一旦可读，调用回调函数将协程3 resume以便继续执行recv。<br>
上面的4、5、6步都是异步的，系统并不会阻塞，IOManager仍然可以调度其他的任务，只在相关的事件发生后，再继续执行对应的任务即可。并且，由于hook的函数对调用方是不可知的，调用方也不需要知道hook的细节，所以对调用方也很方便，只需要以同步的方式编写代码，实现的效果却是异常执行的，效率很高。<br>
sylar对以下函数进行了hook，并且只对socket fd进行了hook，如果操作的不是socket fd，那会直接调用系统原本的api，而不是hook之后的api：<br>
`sleep`<br>
`usleep`<br>
`nanosleep`<br>
`socket`<br>
`connect`  <br>
`accept` <br> 
`read `  <br>
`readv` <br> 
`recv` <br> 
`recvfrom`  <br>
`recvmsg`  <br>
`write`  <br>
`writev`  <br>
`send` <br>
`sendto`  <br>
`sendmsg`  <br>
`close`  <br>
`fcntl`  <br>
`ioctl`  <br>
`getsockopt`  <br>
`setsockopt`<br>

9.Socket模块
==
封装了一个Address地址基类，IPAddress/UnixAddress类继承自Address，IPv4/6Address继承自IPAddress，其他Address统一归为UnknowAddress类。<br>
封装了Socket类，提供所有socket API功能，支持创建TCP/UDP/Unix socket,对accept、bind、connect、listen、close、send、write等方法重新进行封装。<br>

10.ByteArray序列化模块
==
ByteArray二进制序列化模块，提供对二进制数据的常用操作。读写入基础类型int8_t,int16_t,int32_t,int64_t等，支持Varint,std::string的读写支持,支持字节序转化,支持序列化到文件，以及从文件反序列化等功能。<br>
ByteArray的底层存储是固定大小的块，以链表形式组织。每次写入数据时，将数据写入到链表最后一个块中，如果最后一个块不足以容纳数据，则分配一个新的块并添加到链表结尾，再写入数据。<br>ByteArray会记录当前的操作位置，每次写入数据时，该操作位置按写入大小往后偏移，如果要读取数据，则必须调用setPosition重新设置当前的操作位置。<br>
ByteArray支持基础类型的序列化与反序列化功能，并且支持将序列化的结果写入文件，以及从文件中读取内容进行反序列化。ByteArray支持以下类型的序列化与反序列化：<br>
　　1.固定长度的有符号/无符号8位、16位、32位、64位整数<br>
　　2.不固定长度的有符号/无符号32位、64位整数<br>
　　3.float、double类型<br>
　　4.字符串，包含字符串长度，长度范围支持16位、32位、64位。<br>
　　5.字符串，不包含长度。<br>
以上所有的类型都支持读写。<br>

ByteArray还支持设置序列化时的大小端顺序。<br>
ByteArray在序列化不固定长度的有符号/无符号32位、64位整数时使用了zigzag算法。<br>
ByteArray在序列化字符串时使用TLV编码结构中的Length和Value。<br>

11.Stream模块
==
流结构，提供字节流读写接口。<br>
所有的流结构都继承自抽象类Stream，Stream类规定了一个流必须具备read/write接口和readFixSize/writeFixSize接口，继承自Stream的类必须实现这些接口。使用的时候，采用统一的风格操作。基于统一的风格，可以提供更灵活的扩展。目前实现了SocketStream。<br>

12.TcpServer模块
==
TCP服务器封装。<br>
基于Socket类，封装了一个通用的TcpServer的服务器类，提供简单的API，使用便捷，可以快速绑定一个或多个地址，启动服务，监听端口，accept连接，处理socket连接等功能。具体业务功能的服务器实现，只需要继承该类就可以快速实现，后面实现了HttpServer。<br>

13.HTTP模块
==
提供HTTP服务，主要包含以下几个模块：<br>

1.HTTP常量定义，包括HTTP方法HttpMethod与HTTP状态HttpStatus。<br>
2.HTTP请求与响应结构，对应HttpRequest和HttpResponse。<br>
3.HTTP解析器，包含HTTP请求解析器与HTTP响应解析器，对应HttpRequestParser和HttpResponseParser。<br>
4.HTTP会话结构，对应HttpSession。<br>
5.HTTP服务器。<br>
6.HTTP Servlet。<br>
7.uri模块<br>
8.HTTP客户端HttpConnection，用于发起GET/POST等请求，支持连接池。<br>
9.HTTP连接池。<br>
HTTP模块依赖nodejs/http-parser提供的HTTP解析器，并且直接复用了nodejs/http-parser中定义的HTTP方法与状态枚举。<br>

1.HTTP常量定义<br>
包括HttpMethod和HttpStatus两个定义，如下：<br>
/* Request Methods */<br>
#define HTTP_METHOD_MAP(XX)         \ <br>
　XX(0,  DELETE,      DELETE)       \ <br>
　XX(1,  GET,         GET)          \ <br>
　XX(2,  HEAD,        HEAD)         \ <br>
　XX(3,  POST,        POST)         \ <br>
　XX(4,  PUT,         PUT)          \ <br>
... <br>
 
/* Status Codes */ <br>
#define HTTP_STATUS_MAP(XX)                                                 \ <br>
　XX(100, CONTINUE,                        Continue)                        \ <br>
　XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \ <br>
　XX(102, PROCESSING,                      Processing)                      \ <br>
　XX(200, OK,                              OK)                              \ <br>
　XX(201, CREATED,                         Created)                         \ <br>
　XX(202, ACCEPTED,                        Accepted)                        \ <br>
　XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \ <br>
... <br>

2.请求与响应结构<br>
包括HttpRequest和HttpResponse两个结构，用于封装HTTP请求与响应。对于HTTP请求，需要关注HTTP方法，请求路径和参数，HTTP版本，HTTP头部的key-value结构，Cookies，以及HTTP Body内容。<br>
对于HTTP响应，需要关注HTTP版本，响应状态码，响应字符串，响应头部的key-value结构，以及响应的Body内容。<br>

3.HTTP解析器<br>
输入字节流，解析HTTP消息，包括HttpRequestParser和HttpResponseParser两个结构。<br>
HTTP解析器基于nodejs/http-parser实现，通过套接字读到HTTP消息后将消息内容传递给解析器，解析器通过回调的形式通知调用方HTTP解析的内容。<br>

4.HTTP会话结构HttpSession<br>
继承自SocketStream，实现了在套接字流上读取HTTP请求与发送HTTP响应的功能，在读取HTTP请求时需要借助HTTP解析器，以便于将套接字流上的内容解析成HTTP请求。<br>

5.HTTP服务器<br>
继承自TcpServer，重载handleClient方法，将accept后得到的客户端socket封装成HttpSession结构，以便于接收和发送HTTP消息。<br>

6.HTTP Servlet<br>
提供HTTP请求路径到处理类的映射，用于规范化的HTTP消息处理流程。<br>
HTTP Servlet包括两部分，第一部分是Servlet对象，每个Servlet对象表示一种处理HTTP消息的方法，第二部分是ServletDispatch，它包含一个请求路径到Servlet对象的映射，用于指定一个请求路径该用哪个Servlet来处理。<br>

7.uri模块<br>
有scheme/用户信息/host/路径/查询参数/fragmen/端口等属性，提供create方法将字符串转化为uri对象，提供各个属性的获取和设置方法。<br>

8.HTTP客户端HttpConnection<br>
继承自SocketStream，用于发起GET/POST等请求并获取响应，支持设置超时，keep-alive，支持连接池。将客户端的socket包装成HttpConnection,用来和HttpSession进行通信。<br>
封装了一系列DoRequest方法，提供了很方便的接口。用户只需要传入请求的方法，uri/url和超时时长，就会自动得到server的回应数据并保存到HttpResult结构体中。否则，用户需要先创建一个server的地址，再创建一个socket连接该地址，然后用此socket创建一个connection。再然后创建一个请求结构体设置请求，然后调用conn的sendRequest方法发送给server，然后用一个接收结构体接收server的回应数据。<br>

9.HTTP连接池HttpConnectionPool<br>
连接池，是指提前预备好一系列已接建立连接的socket，这样，在发起请求时，可以直接从中选择一个进行通信，而不用重复创建套接字→ 发起connect→ 发起请求的流程。<br>
一条连接有创建时间、最大存在时间、最大处理请求数等属性。

14.守护进程
==
将进程与终端解绑，转到后台运行，除此外，还实现了双进程唤醒功能，父进程作为守护进程的同时会检测子进程是否退出，如果子进程退出，则会定时重新拉起子进程。<br>
守护进程的实现步骤：<br>
　　1.调用daemon(1, 0)将当前进程以守护进程的形式运行；<br>
　　2.守护进程fork子进程，在子进程运行主业务；<br>
　　3.父进程通过waitpid()检测子进程是否退出，如果子进程退出，则重新拉起子进程；<br>
