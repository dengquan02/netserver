all:client echo_server

client:client.cpp
	g++ -g -o client client.cpp
echo_server:echo_server.cpp InetAddress.cpp InetAddress.h Socket.cpp Socket.h Epoll.cpp Epoll.h \
Channel.cpp Channel.h EventLoop.h EventLoop.cpp TcpServer.h TcpServer.cpp Acceptor.h \
Acceptor.cpp Connection.h Connection.cpp Buffer.h Buffer.cpp EchoServer.h EchoServer.cpp \
ThreadPool.h ThreadPool.cpp Timestamp.h Timestamp.cpp
	g++ -g -o echo_server echo_server.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp \
	EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp \
	Timestamp.cpp -lpthread

clean:
	rm -f client echo_server
