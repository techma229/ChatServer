# ChatServer
基于muduo库开发的聊天服务器，提供服务器和客户端和源码，可工作在nginx负载均衡环境下，基于redis的订阅、发布队列可进行跨服务器聊天。

编译方式：  
cd ChatServer
cmake ..
make

如何使用：  
  编译后在src/server中生成服务器文件，在src/client中生成客户端文件。  
执行示例：  
  ./ChatServer ip 端口  
  ./ChatClient ip 端口  
  在nginx配置文件中添加服务器的ip和端口，即可实现多服务器负载均衡。项目已使用基于订阅发布的redis实现客户端之间跨服务器通信。
