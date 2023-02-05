
update 20220506:
    
    - 修复用户密码输入错误会导致超时退出出错；
    - 修复所用户登录部分用户无法超时退出；
    - 缩小超时检查时间间隔，减少客户不断输入命令导致无法检测超时的情况；
    - 客户端输出外观优化；
    - token增加过期机制；
    - 修复了gets、puts不更新超时时间；
    - 修复了token过期时间；


update 20220507:
    
    - 修复了客户端token超时再次生成token时没有用到用户名的bug；
    - 修复了服务端秒传判断本地文件表中存在文件之后未判断本地是否真的存在文件，或文件是否不全，影响了断点续传功能的bug；
    - 优化了断点续传功能；
    - 优化了超时断开的判断，用循环队列轮盘法代替原来的轮询；

update 20220511:

    - 调整了登录的验证，改回服务端验证，增加安全性；
    

1. 如何使用：
    
    - 请将客户端和服务端的config文件改为自己电脑的IP；
    - ls：使用ls加目录名（或不加）查看目标目录下的文件；
    - cd：使用cd加目录名移动到指定目录；
    - pwd：显示当前路径，如果同一用户多次连接，某一连接改变路径后可用pwd刷新当前路径；
    - rm：使用rm加文件名或目录名可以删除文件或目录，若删除的是目录，目录下的文件一并被删除；
    - mkdir:在当前目录下创建文件夹，不支持在执行路径下创建；
    - puts filename：将本地文件上传至服务器；
    - gets filename：将服务器的文件下载到本地；
    验证puts和gets功能请自行在相应文件夹中创建文件测试；
    以上功能支持断点续传，支持秒传，但大文件计算md5码需要花费一些时间，如1G的文件；

2. 代码架构：
    
    - 服务端主线程：处理注册、登录、token值验证以及短命令（不涉及到文件上传下载的命令）；
    - 服务端子线程(3个)：处理长命令；
    - 客户端主线程：进行注册、登录以及接收命令，发送短命令的操作，并监听服务端发来的超时退出请求；
    - 客户端子线程：发送token验证，token过期则重新生成，发送长命令，执行长命令；

3. 实现功能：
    
    - 基本命令的执行；
    - 文件断点续传；
    - 服务端已有的文件秒传；
    - 大文件（超过1G）用mmap一次传输；
    - 客户端超过30s没操作自动断开，该功能待优化，目前使用0.1s一次轮询，客户端数量较大时；

4. 所使用数据库表
    
    - 用户注册信息表：记录用户姓名、密文、盐值、当前路径、当前路径id;
    - 本地文件信息表：记录本地文件名(文件名+第一次上传的用户id+第一次上传的路径id，确保唯一性)、文件连接数（虚拟表中的引用次数）、md5值;
    - 虚拟文件信息表：记录父目录id、文件名（虚拟名，用户使用）、所属用户id、文件md5值、文件大小、文件类型（目前只支持目录和普通文件）;

