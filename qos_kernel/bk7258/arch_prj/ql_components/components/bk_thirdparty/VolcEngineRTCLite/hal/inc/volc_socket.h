/*
 * Copyright (c) 2024 The VolcEngineRTCLite project authors. All Rights Reserved.
 * @brief VolcEngineRTCLite Interface Lite
 */

#ifndef __HAL_VOLC_SOCKET_H__
#define __HAL_VOLC_SOCKET_H__

#include <stdint.h>
#include <stdio.h>
#include "volc_network.h"
#include <sys/types.h> 
#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#if defined(__BUILDING_BYTE_RTC_SDK__)
#define __byte_rtc_api__ __declspec(dllexport)
#else
#define __byte_rtc_api__ __declspec(dllimport)
#endif
#else
#define __byte_rtc_api__ __attribute__((visibility("default")))
#endif

// #ifndef socklen_t
// #define socklen_t int
// #endif

typedef enum {
    VOLC_OP_CONNECT = 0,
    VOLC_OP_READ    = 1,
    VOLC_OP_WRITE   = 2,
    VOLC_OP_TIMER   = 3,
    VOLC_OP_ASYNC   = 4,
} volc_ev_op_type_e;

// 定义事件类型
typedef enum {
    VOLC_EVLOOP_POLLIN  = 0x0001,
    VOLC_EVLOOP_POLLOUT = 0x0004,
    VOLC_EVLOOP_POLLERR = 0x0008,
    VOLC_EVLOOP_POLLHUP = 0x0010,
} volc_ev_loop_poll_type_e;


typedef enum {
    VOLC_SOCK_STREAM = 1,	
    VOLC_SOCK_DGRAM  = 2,		
} volc_socket_type_e;

struct volc_pollfd {
    int   fd;         /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
};

/**
 * @brief 创建一个新的套接字
 * 
 * 此函数用于创建一个新的套接字，类似于标准的 socket 函数。
 * 它接受三个参数来指定套接字的域、类型和协议。
 * 
 * @param domain 套接字的协议族，例如 AF_INET 用于 IPv4 或 AF_INET6 用于 IPv6。
 * @param type 套接字的类型，例如 SOCK_STREAM 用于 TCP 或 SOCK_DGRAM 用于 UDP。
 * @param protocol 套接字使用的协议，通常为 0 以使用默认协议。
 * @return int 如果成功，返回新创建的套接字描述符；如果失败，返回 -1。
 */
int volc_socket(int domain, int type, int protocol);

/**
 * @brief 将套接字绑定到指定地址
 * 
 * 此函数用于将一个套接字描述符 `sockfd` 绑定到一个特定的地址 `addr`，地址长度由 `addrlen` 指定。
 * 类似于标准的 `bind` 函数，它允许应用程序指定套接字使用的本地地址和端口。
 * 
 * @param sockfd 要绑定的套接字描述符。
 * @param addr 指向 `struct sockaddr_in` 结构体的指针，包含要绑定的地址信息。
 * @return int 如果绑定成功，返回 0；如果失败，返回 -1。
 */
int volc_bind(int sockfd, const volc_ip_addr_t* addr);

/**
 * @brief 标记套接字为被动套接字，用于接受传入的连接请求
 * 
 * 此函数将指定的套接字 `sockfd` 标记为被动套接字，使其能够接受传入的连接请求。
 * 它类似于标准的 `listen` 函数，允许应用程序指定在队列中等待处理的最大连接数。
 * 
 * @param sockfd 要标记为被动套接字的描述符。
 * @param backlog 在队列中等待处理的最大连接数。
 * @return int 如果成功，返回 0；如果失败，返回 -1。
 */
int volc_listen(int sockfd, int backlog);
/**
 * @brief 接受一个传入的连接请求
 * 
 * 此函数用于从指定的监听套接字 `sockfd` 接受一个传入的连接请求。
 * 它会阻塞调用线程，直到有新的连接请求到达。
 * 当有新的连接请求时，函数会返回一个新的套接字描述符，用于与客户端进行通信。
 * 同时，会将客户端的地址信息存储在 `addr` 指向的结构体中，地址长度存储在 `addrlen` 指向的变量中。
 * 
 * @param sockfd 监听套接字的描述符，该套接字必须已经通过 `volc_listen` 函数设置为监听状态。
 * @param addr 指向 `struct sockaddr_in` 结构体的指针，用于存储客户端的地址信息。
 * @param addrlen 指向 `int` 类型的指针，用于存储 `addr` 结构体的长度。
 * @return int 如果成功，返回一个新的套接字描述符，用于与客户端进行通信；如果失败，返回 -1。
 */
int volc_accept(int sockfd, volc_ip_addr_t* addr, int* addrlen);

/**
 * @brief 尝试将套接字连接到指定地址
 * 
 * 此函数用于将指定的套接字描述符 `sockfd` 连接到由 `addr` 指向的地址，地址长度由 `addrlen` 指定。
 * 类似于标准的 `connect` 函数，它允许应用程序发起一个连接请求到远程服务器。
 * 
 * @param sockfd 要连接的套接字描述符。
 * @param addr 指向 `struct sockaddr_in` 结构体的指针，包含要连接的远程地址信息。
 * @return int 如果连接成功，返回 0；如果失败，返回 -1。
 */
int volc_connect(int sockfd, volc_ip_addr_t* addr);

/**
 * @brief 从指定套接字接收消息
 * 
 * 此函数用于从指定的套接字 `sockfd` 接收消息，并将接收到的数据存储在 `data` 指向的缓冲区中。
 * 接收的数据最大长度由 `size` 指定。同时，会将发送方的地址信息存储在 `addr` 指向的结构体中，
 * 并将接收状态存储在 `p_status` 指向的变量中。
 * 
 * @param sockfd 要接收消息的套接字描述符。
 * @param data 指向用于存储接收到的数据的缓冲区的指针。
 * @param size 缓冲区的最大长度，即最多可以接收的数据字节数。
 * @param p_addr 指向 `volc_ip_addr_t` 结构体的指针，用于存储发送方的地址信息。
 * @param p_status 指向 `uint32_t` 类型的指针，用于存储接收状态。
 * @return ssize_t 如果成功，返回接收到的字节数；如果发生错误，返回 -1。
 */
ssize_t volc_recv_msg(int sockfd, void* data, size_t size, volc_ip_addr_t *p_addr, uint32_t* p_status);

/**
 * @brief 通过指定套接字发送消息
 * 
 * 此函数用于通过指定的套接字 `sockfd` 向 `addr` 所指向的地址发送消息。
 * 要发送的数据存储在 `data` 指向的缓冲区中，数据长度由 `size` 指定。
 * 同时，可通过 `status` 参数设置发送的状态。
 * 
 * @param sockfd 用于发送消息的套接字描述符。
 * @param data 指向要发送的数据的缓冲区的指针。
 * @param size 要发送的数据的字节数。
 * @param addr 指向 `struct sockaddr_in` 结构体的指针，包含目标地址信息。
 * @param p_status 发送状态，可用于设置特定的发送条件或标志。
 * @return ssize_t 如果成功，返回实际发送的字节数；如果发生错误，返回 -1。
 */
ssize_t volc_send_msg(int sockfd, void* data, size_t size, volc_ip_addr_t* addr, uint32_t* p_status);

/**
 * @brief 关闭指定的套接字
 * 
 * 此函数用于关闭由 `sockfd` 指定的套接字。关闭套接字后，该套接字将不再可用，
 * 并且系统会释放与之相关的资源。类似于标准的 `close` 函数。
 * 
 * @param sockfd 要关闭的套接字描述符。
 * @return int 如果关闭成功，返回 0；如果发生错误，返回 -1。
 */
int volc_close(int sockfd);


/**
 * @brief 将指定套接字设置为非阻塞模式
 * 
 * 此函数用于将给定的套接字描述符 `sockfd` 设置为非阻塞模式。
 * 在非阻塞模式下，对该套接字的读写操作不会阻塞调用线程，
 * 而是立即返回，即使没有数据可读或写入操作无法立即完成。
 * 
 * @param sockfd 要设置为非阻塞模式的套接字描述符。
 * @return int 如果设置成功，返回 0；如果发生错误，返回 -1。
 */
int volc_set_nonblocking(int sockfd);

int volc_make_pipe(int sockfds[2]);
int volc_write(int sockfd, void* data, size_t size);
int volc_read(int sockfd, void* data, size_t size);
// int volc_sockopt_set(int sockfd, int level, int optname, const void* optval, int optlen);
// int volc_sockopt_get(int sockfd, int level, int optname, void* optval, int* optlen);


int volc_sockopt_set_buffer_size(int __fd, bool _is_send_buffer,int buffer_size) ;
 
int volc_sockopt_get_buffer_size(int __fd, bool _is_send_buffer) ;

int volc_getaddrinfo(const char* host, uint16_t port, volc_ip_addr_t** addrs, int* count);

int volc_freeaddrinfo(volc_ip_addr_t* addrs);

int volc_poll(struct volc_pollfd *fds, int nfds, int timeout);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_ERRNO_H__ */
