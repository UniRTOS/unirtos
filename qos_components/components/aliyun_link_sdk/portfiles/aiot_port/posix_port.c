/*
 * 这个移植示例适用于`Linux`这类支持pthread的POSIX设备，它实现了移植SDK所需要的接口。
 * 移植接口大体可以分为两类：网络接口(TCP)、系统接口(OS)
 *
 * + 网络接口：TCP/UDP的socket适配，请勿在此实现TLS传输，TLS实现已封装在SDK内部。
 * + 系统接口：包含内存管理、随机数、系统时间、互斥锁等功能移植。
 */

#include "qosa_def.h"
#include "qosa_sockets.h"
#include "qosa_rtc.h"
#include "qosa_sys.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"


/* socket建联时间默认最大值 */
#define CORE_SYSDEP_DEFAULT_CONNECT_TIMEOUT_MS (10 * 1000)
#define MAX_LOG_SIZE 200

typedef struct {
    int fd;
    core_sysdep_socket_type_t socket_type;
    char *host;
    char backup_ip[16];
    qosa_uint16_t port;
    qosa_uint32_t connect_timeout_ms;
} core_network_handle_t;


/*
 * 日志会通过用户设置的日志回调函数g_logcb_handler输出，若未设置，则不输出。
 * 设置日志回调接口：aiot_state_set_logcb
 */
extern aiot_state_logcb_t g_logcb_handler;
static void _core_printf(const char* format, ...)
{
    va_list args;
    char buffer[MAX_LOG_SIZE] = {0};
    qosa_int32_t written = 0;

    /* 如果没有设置日志回调处理程序，则直接返回 */
    if (g_logcb_handler == QOSA_NULL) {
        return;
    }

    va_start(args, format);

    /* 初始化缓冲区 */
    qosa_memset(buffer, 0, sizeof(buffer));

    /* 计算需要写入缓冲区的日志长度 */
    written = qosa_vsnprintf(buffer, MAX_LOG_SIZE, format, args);

    /* 检查是否成功写入日志 */
    if (written < 0) {
        va_end(args);
        return;
    }

    /* 若输出日志超出buffer长度，只输出buffer长度 */
    if(written >= MAX_LOG_SIZE) {
        buffer[MAX_LOG_SIZE - 1] = 0;
    }

    va_end(args);

    /* 调用日志回调处理程序 */
    g_logcb_handler(STATE_PORT_BASE, buffer);
}

void *core_sysdep_malloc(qosa_uint32_t size, char *name)
{
    (void)name;
    void *res = qosa_malloc(size);
    if(res == QOSA_NULL) {
         _core_printf("sysdep malloc failed \n");
    }
    return res;
}

void core_sysdep_free(void *ptr)
{
    qosa_free(ptr);
}

qosa_uint64_t core_sysdep_time(void)
{
    return (qosa_uint64_t)qosa_get_system_time_milliseconds();
}

void core_sysdep_sleep(qosa_uint64_t time_ms)
{
    while (time_ms > UINT32_MAX) {
        qosa_task_sleep_ms(UINT32_MAX);
        time_ms -= UINT32_MAX;
    }

    qosa_task_sleep_ms((qosa_uint32_t)time_ms);
}

static void *core_sysdep_network_init(void)
{
    core_network_handle_t *handle = qosa_malloc(sizeof(core_network_handle_t));
    if (handle == QOSA_NULL) {
        return QOSA_NULL;
    }
    qosa_memset(handle, 0, sizeof(core_network_handle_t));
    handle->connect_timeout_ms = CORE_SYSDEP_DEFAULT_CONNECT_TIMEOUT_MS;
    handle->fd = -1;

    return handle;
}

static int32_t core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == QOSA_NULL || data == QOSA_NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (option >= CORE_SYSDEP_NETWORK_MAX) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    switch (option) {
        case CORE_SYSDEP_NETWORK_SOCKET_TYPE: {
            network_handle->socket_type = *(core_sysdep_socket_type_t *)data;
        }
        break;
        case CORE_SYSDEP_NETWORK_HOST: {
            network_handle->host = qosa_malloc(qosa_strlen((const char *)data) + 1);
            if (network_handle->host == QOSA_NULL) {
                 _core_printf("malloc failed\n");
                return STATE_PORT_MALLOC_FAILED;
            }
            qosa_memset(network_handle->host, 0, qosa_strlen((const char *)data) + 1);
            qosa_memcpy(network_handle->host, data, qosa_strlen((const char *)data));
        }
        break;
        case CORE_SYSDEP_NETWORK_BACKUP_IP: {
            qosa_memcpy(network_handle->backup_ip, data, qosa_strlen((const char *)data));
        }
        break;
        case CORE_SYSDEP_NETWORK_PORT: {
            network_handle->port = *(qosa_uint16_t *)data;
        }
        break;
        case CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS: {
            network_handle->connect_timeout_ms = *(qosa_uint32_t *)data;
        }
        break;
        default: {
            break;
        }
    }

    return STATE_SUCCESS;
}

static void _port_uint2str(qosa_uint16_t input, char *output)
{
    qosa_uint8_t i = 0, j = 0;
    char temp[6] = {0};

    do {
        temp[i++] = input % 10 + '0';
    } while ((input /= 10) > 0);

    do {
        output[--i] = temp[j++];
    } while (i > 0);
}

static int _core_sysdep_socket_errno(int fd)
{
    int sock_err = errno;

#ifdef UNIRTOS_LWIP_MODIFY
    if (sock_err == 0 && fd >= 0) {
        int lwip_err = lwip_get_error(fd);
        if (lwip_err != 0) {
            sock_err = lwip_err;
        }
    }
#else
    (void)fd;
#endif

    return sock_err;
}

static qosa_int32_t _core_sysdep_network_connect(char *host, qosa_uint16_t port, int family, int socktype, int protocol,
        qosa_uint32_t timeout_ms, int *fd_out)
{
    qosa_int32_t res = STATE_SUCCESS;
    int fd = 0, sock_option = 0, sock_err = 0;
    char service[6] = {0};
    struct addrinfo hints;
    struct addrinfo *addrInfoList = QOSA_NULL, *pos = QOSA_NULL;
    struct sockaddr_in loc_addr;
    socklen_t len = sizeof(sizeof(loc_addr));
    fd_set write_sets;
    struct timeval timeselect;

    qosa_memset(&hints, 0, sizeof(hints));
    hints.ai_family = family; /* only IPv4 */
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = 0;
    _port_uint2str(port, service);

    /* lwIP/RTOS 平台无 SIGPIPE 信号机制，无需忽略 */

    res = getaddrinfo(host, service, &hints, &addrInfoList);
    if (res == 0) {
        for (pos = addrInfoList; pos != QOSA_NULL; pos = pos->ai_next) {
            fd = socket(pos->ai_family, pos->ai_socktype, pos->ai_protocol);
            if (fd < 0) {
                 _core_printf("create socket error\n");
                res = STATE_PORT_NETWORK_SOCKET_CREATE_FAILED;
                continue;
            }

            res = fcntl(fd, F_GETFL, 0);
            if (res != -1) {
                res = fcntl(fd, F_SETFL, sock_option | O_NONBLOCK);
            }

            if (res == -1) {
                /* block connect */
                if (connect(fd, pos->ai_addr, pos->ai_addrlen) == 0) {
                    *fd_out = fd;
                    res = STATE_SUCCESS;
                    break;
                } else {
                    res = STATE_PORT_NETWORK_CONNECT_FAILED;
                }
            } else {
                /* non-block connect */
                int connect_res = 0;

                FD_ZERO(&write_sets);
                FD_SET(fd, &write_sets);

                timeselect.tv_sec = timeout_ms / 1000;
                timeselect.tv_usec = timeout_ms % 1000 * 1000;

                connect_res = connect(fd, pos->ai_addr, pos->ai_addrlen);
                if (connect_res == 0) {
                    *fd_out = fd;
                    res = STATE_SUCCESS;
                    break;
                }

                sock_err = _core_sysdep_socket_errno(fd);
                if (sock_err != EINPROGRESS && sock_err != EALREADY) {
                    res = STATE_PORT_NETWORK_CONNECT_FAILED;
                } else {
                    res = select(fd + 1, NULL, &write_sets, NULL, &timeselect);
                    if (res == 0) {
                        res = STATE_MQTT_LOG_CONNECT_TIMEOUT;
                    } else if (res < 0) {
                        res = STATE_PORT_NETWORK_CONNECT_FAILED;
                    } else {
                        if (FD_ISSET(fd, &write_sets)) {
                            int so_error = 0;
                            socklen_t so_error_len = sizeof(so_error);

                            res = getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len);
                            if (res == 0 && (so_error == 0 || so_error == EISCONN)) {
                                *fd_out = fd;
                                res = STATE_SUCCESS;
                                break;
                            } else {
                                sock_err = (res == 0) ? so_error : _core_sysdep_socket_errno(fd);
                                res = STATE_PORT_NETWORK_CONNECT_FAILED;
                            }
                        }
                    }
                }
            }

            sock_err = _core_sysdep_socket_errno(fd);
            close(fd);
             _core_printf("connect error, errno: %d\n", sock_err);
        }
    } else {
        res = STATE_PORT_NETWORK_DNS_FAILED;
    }

    if (res < 0) {
         _core_printf("fail to establish tcp\n");
    } else {
         _core_printf("success to establish tcp, fd=%d\n", *fd_out);
        qosa_memset(&loc_addr, 0, len);
        /* 获取本地的port并打印 */
        if (0 == getsockname(*fd_out, (struct sockaddr *)&loc_addr, &len)
            && loc_addr.sin_family == AF_INET) {
            _core_printf("local port: %u\n", ntohs(loc_addr.sin_port));
        }

        res = STATE_SUCCESS;
    }

    if(addrInfoList) {
        freeaddrinfo(addrInfoList);
    }

    return res;
}

static int32_t _core_sysdep_network_tcp_establish(core_network_handle_t *network_handle)
{
    qosa_int32_t res = STATE_SUCCESS;

    _core_printf("establish tcp connection with server(host='%s', port=[%u])\n", network_handle->host, network_handle->port);
    res = _core_sysdep_network_connect(network_handle->host, network_handle->port,
                                       AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, network_handle->connect_timeout_ms, &network_handle->fd);
    if (res == STATE_PORT_NETWORK_DNS_FAILED && qosa_strlen(network_handle->backup_ip) > 0) {
         _core_printf("using backup ip: %s\n", network_handle->backup_ip);
        res = _core_sysdep_network_connect(network_handle->backup_ip, network_handle->port,
                                           AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, network_handle->connect_timeout_ms, &network_handle->fd);
    }

    return res;
}

static qosa_int32_t _core_sysdep_network_udp_server_establish(core_network_handle_t *network_handle)
{
    qosa_int32_t sockfd;
    struct sockaddr_in servaddr;
    int opt_val = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
         _core_printf("create socket error, errno: %d\n", errno);
        return STATE_PORT_NETWORK_SOCKET_CREATE_FAILED;
    }

    if (0 != setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val))) {
         _core_printf("setsockopt(SO_REUSEADDR) falied, errno: %d\n", errno);
        close(sockfd);
        return STATE_PORT_NETWORK_SOCKET_CONFIG_FAILED;
    }

    qosa_memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(network_handle->port);

    if (-1 == bind(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in))) {
         _core_printf("bind(%d) falied, errno: %d\n", (int)sockfd, errno);
        close(sockfd);
        return STATE_PORT_NETWORK_SOCKET_BIND_FAILED;
    }

    network_handle->fd = sockfd;
     _core_printf("success to establish udp, fd=%d\n", (int)sockfd);

    return 0;
}

static int32_t core_sysdep_network_establish(void *handle)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;
    if (handle == QOSA_NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        if (network_handle->host == QOSA_NULL) {
            return STATE_PORT_MISSING_HOST;
        }
        return _core_sysdep_network_tcp_establish(network_handle);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        if (network_handle->host == QOSA_NULL) {
            return STATE_PORT_MISSING_HOST;
        }
        return  _core_sysdep_network_connect(network_handle->host, network_handle->port,
                                             AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP, network_handle->connect_timeout_ms, &network_handle->fd);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return _core_sysdep_network_udp_server_establish(network_handle);
    }

     _core_printf("unknown nwk type or tcp host absent\n");
    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}


static qosa_int32_t _core_sysdep_network_recv(core_network_handle_t *network_handle, qosa_uint8_t *buffer, qosa_uint32_t len,
        qosa_uint32_t timeout_ms)
{
    int res = 0;
    qosa_int32_t recv_bytes = 0;
    qosa_ssize_t recv_res = 0;
    qosa_uint64_t timestart_ms = 0, timenow_ms = 0, timeselect_ms = 0;
    fd_set recv_sets;
    struct timeval timeselect;

    FD_ZERO(&recv_sets);
    FD_SET(network_handle->fd, &recv_sets);

    timestart_ms = (qosa_uint64_t)qosa_get_system_time_milliseconds();
    timenow_ms = timestart_ms;

    do {
        timenow_ms = (qosa_uint64_t)qosa_get_system_time_milliseconds();

        if (timenow_ms - timestart_ms >= timenow_ms ||
            timeout_ms - (timenow_ms - timestart_ms) > timeout_ms) {
            break;
        }

        timeselect_ms = timeout_ms - (timenow_ms - timestart_ms);
        timeselect.tv_sec = timeselect_ms / 1000;
        timeselect.tv_usec = timeselect_ms % 1000 * 1000;

        res = select(network_handle->fd + 1, &recv_sets, NULL, NULL, &timeselect);
        if (res == 0) {
            /*  _core_printf("_core_sysdep_network_recv, nwk select timeout\n"); */
            continue;
        } else if (res < 0) {
             _core_printf("_core_sysdep_network_recv, errno: %d\n", errno);
            return STATE_PORT_NETWORK_SELECT_FAILED;
        } else {
            if (FD_ISSET(network_handle->fd, &recv_sets)) {
                recv_res = recv(network_handle->fd, buffer + recv_bytes, len - recv_bytes, 0);
                if (recv_res == 0) {
                     _core_printf("_core_sysdep_network_recv, nwk connection closed\n");
                    return STATE_PORT_NETWORK_RECV_CONNECTION_CLOSED;
                } else if (recv_res < 0) {
                     _core_printf("_core_sysdep_network_recv, errno: %d\n", errno);
                    if (errno == EINTR) {
                        continue;
                    }
                    return STATE_PORT_NETWORK_RECV_FAILED;
                } else {
                    recv_bytes += recv_res;
                    /*  _core_printf("recv_bytes: %d, len: %d\n",recv_bytes,len); */
                    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT || (qosa_uint32_t)recv_bytes == len) {
                        break;
                    }
                }
            }
        }
    } while (((timenow_ms - timestart_ms) < timeout_ms) && ((qosa_uint32_t)recv_bytes < len));

    /*  _core_printf("%s: recv over\n",__FUNCTION__); */
    return recv_bytes;
}

static qosa_int32_t _core_sysdep_network_udp_server_recv(core_network_handle_t *network_handle, qosa_uint8_t *buffer,
        qosa_uint32_t len, qosa_uint32_t timeout_ms, core_sysdep_addr_t *addr)
{
    int res;
    struct sockaddr_in cliaddr;
    socklen_t addr_len = sizeof(cliaddr);
    fd_set read_fds;
    char *inet_addr = QOSA_NULL;
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};

    FD_ZERO(&read_fds);
    FD_SET(network_handle->fd, &read_fds);

    res = select(network_handle->fd + 1, &read_fds, NULL, NULL, &timeout);
    if (res == 0) {
         _core_printf("select timeout\n");
        return 0;
    } else if (res < 0) {
         _core_printf("_linux_nwk_udp_read select errno: %d\n", errno);
        return STATE_PORT_NETWORK_SELECT_FAILED;
    }

    res = recvfrom(network_handle->fd, buffer, len, 0, (struct sockaddr *)&cliaddr, &addr_len);
    if (res >= 0) {
        if (QOSA_NULL != addr) {
            addr->port = ntohs(cliaddr.sin_port);
            inet_addr = inet_ntoa(cliaddr.sin_addr);
            qosa_strlcpy((char *)addr->addr, inet_addr, sizeof(addr->addr));
        }

        return res;
    } else {
         _core_printf("_linux_nwk_udp_read errno: %d\n", errno);
        return STATE_PORT_NETWORK_RECV_FAILED;
    }
}

static int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == QOSA_NULL || buffer == QOSA_NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (len == 0 || timeout_ms == 0) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        return _core_sysdep_network_recv(network_handle, buffer, len, timeout_ms);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        return _core_sysdep_network_recv(network_handle, buffer, len, timeout_ms);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return _core_sysdep_network_udp_server_recv(network_handle, buffer, len, timeout_ms, addr);
    }

     _core_printf("unknown nwk type\n");

    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

qosa_int32_t _core_sysdep_network_send(core_network_handle_t *network_handle, qosa_uint8_t *buffer, qosa_uint32_t len,
                                  qosa_uint32_t timeout_ms)
{
    int res = 0;
    qosa_int32_t send_bytes = 0;
    qosa_ssize_t send_res = 0;
    qosa_uint64_t timestart_ms = 0, timenow_ms = 0, timeselect_ms = 0;
    fd_set send_sets;
    struct timeval timeselect;

    FD_ZERO(&send_sets);
    FD_SET(network_handle->fd, &send_sets);

    timestart_ms = (qosa_uint64_t)qosa_get_system_time_milliseconds();
    timenow_ms = timestart_ms;

    do {
        timenow_ms = (qosa_uint64_t)qosa_get_system_time_milliseconds();

        if (timenow_ms - timestart_ms >= timenow_ms ||
            timeout_ms - (timenow_ms - timestart_ms) > timeout_ms) {
            break;
        }

        timeselect_ms = timeout_ms - (timenow_ms - timestart_ms);
        timeselect.tv_sec = timeselect_ms / 1000;
        timeselect.tv_usec = timeselect_ms % 1000 * 1000;

        res = select(network_handle->fd + 1, NULL, &send_sets, NULL, &timeselect);
        if (res == 0) {
             _core_printf("_core_sysdep_network_send, nwk select timeout\n");
            continue;
        } else if (res < 0) {
             _core_printf("_core_sysdep_network_send, errno: %d\n", errno);
            return STATE_PORT_NETWORK_SELECT_FAILED;
        } else {
            if (FD_ISSET(network_handle->fd, &send_sets)) {
                send_res = send(network_handle->fd, buffer + send_bytes, len - send_bytes, 0);
                if (send_res == 0) {
                     _core_printf("_core_sysdep_network_send, nwk connection closed\n");
                    return STATE_PORT_NETWORK_SEND_CONNECTION_CLOSED;
                } else if (send_res < 0) {
                     _core_printf("_core_sysdep_network_send, errno: %d\n", errno);
                    if (errno == EINTR) {
                        continue;
                    }
                    return STATE_PORT_NETWORK_SEND_FAILED;
                } else {
                    send_bytes += send_res;
                    if ((qosa_uint32_t)send_bytes == len) {
                        break;
                    }
                }
            }
        }
    } while (((timenow_ms - timestart_ms) < timeout_ms) && ((qosa_uint32_t)send_bytes < len));

    return send_bytes;
}


qosa_int32_t _core_sysdep_network_udp_send(core_network_handle_t *network_handle, qosa_uint8_t *buffer, qosa_uint32_t len,
                                      qosa_uint32_t timeout_ms, core_sysdep_addr_t *addr)
{
    struct sockaddr_in cliaddr;
    fd_set write_fds;
    struct timeval timeout = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    int res;

    if (addr == QOSA_NULL) {
         _core_printf("invalid parameter addr\n");
        return STATE_PORT_NETWORK_SEND_FAILED;
    }

    FD_ZERO(&write_fds);
    FD_SET(network_handle->fd, &write_fds);

    res = select(network_handle->fd + 1, NULL, &write_fds, NULL, &timeout);
    if (res == 0) {
         _core_printf("select timeout\n");
        return 0;
    } else if (res < 0) {
         _core_printf("_linux_nwk_udp_write select errno: %d\n", errno);
        return STATE_PORT_NETWORK_SELECT_FAILED;
    }

    res = inet_aton((char *)addr->addr, &cliaddr.sin_addr);
    if (res < 0) {
         _core_printf("sys_nwk_write, addr error\r\n");
        return STATE_PORT_NETWORK_SEND_FAILED;
    }

    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(addr->port);

    res = sendto(network_handle->fd, buffer, len, 0, (struct sockaddr *)&cliaddr, sizeof(struct sockaddr_in));
    if (res < 0) {
         _core_printf("_linux_nwk_udp_write errno: %d\n", errno);
        return STATE_PORT_NETWORK_SEND_FAILED;
    }

    return res;
}

static int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;

    if (handle == QOSA_NULL || buffer == QOSA_NULL) {
         _core_printf("invalid parameter\n");
        return STATE_PORT_INPUT_NULL_POINTER;
    }
    if (len == 0 || timeout_ms == 0) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        return _core_sysdep_network_send(network_handle, buffer, len, timeout_ms);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        return _core_sysdep_network_send(network_handle, buffer, len, timeout_ms);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return _core_sysdep_network_udp_send(network_handle, buffer, len, timeout_ms, addr);
    }

     _core_printf("unknown nwk type\n");
    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

static void _core_sysdep_network_tcp_disconnect(core_network_handle_t *network_handle)
{
    /* 仅仅对正常的fd 进行close操作 */
    if (network_handle->fd >= 0) {
        shutdown(network_handle->fd, 2);
        close(network_handle->fd);
    }
}

static int32_t core_sysdep_network_deinit(void **handle)
{
    core_network_handle_t *network_handle = QOSA_NULL;

    if (handle == QOSA_NULL || *handle == QOSA_NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    network_handle = *(core_network_handle_t **)handle;

    /* Shutdown both send and receive operations. */
    if ((network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT ||
         network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) && network_handle->host != QOSA_NULL) {
        _core_sysdep_network_tcp_disconnect(network_handle);
    }

    if (network_handle->host != QOSA_NULL) {
        qosa_free(network_handle->host);
        network_handle->host = QOSA_NULL;
    }

    qosa_free(network_handle);
    *handle = QOSA_NULL;

    return 0;
}

void core_sysdep_rand(qosa_uint8_t *output, qosa_uint32_t output_len)
{
    qosa_uint32_t idx = 0, bytes = 0, rand_num = 0;

    for (idx = 0; idx < output_len;) {
        if (output_len - idx < 4) {
            bytes = output_len - idx;
        } else {
            bytes = 4;
        }
        rand_num = (qosa_uint32_t)qosa_rand();
        while (bytes-- > 0) {
            output[idx++] = (qosa_uint8_t)(rand_num >> bytes * 8);
        }
    }
}

void *core_sysdep_mutex_init(void)
{
    qosa_mutex_t mutex = QOSA_NULL;

    if (qosa_mutex_create(&mutex) != QOSA_OK) {
        _core_printf("create mutex failed \n");
        return QOSA_NULL;
    }

    return mutex;
}

void core_sysdep_mutex_lock(void *mutex)
{
    if (mutex != QOSA_NULL) {
        qosa_int32_t res = qosa_mutex_lock((qosa_mutex_t)mutex, QOSA_WAIT_FOREVER);
        if (res != QOSA_OK) {
            _core_printf("lock mutex failed: %d\n", res);
        }
    }
}

void core_sysdep_mutex_unlock(void *mutex)
{
    if (mutex != QOSA_NULL) {
        qosa_int32_t res = qosa_mutex_unlock((qosa_mutex_t)mutex);
        if (res != QOSA_OK) {
            _core_printf("unlock mutex failed: %d\n", res);
        }
    }
}

void core_sysdep_mutex_deinit(void **mutex)
{
    if (mutex != QOSA_NULL) {
        qosa_int32_t res = qosa_mutex_delete((qosa_mutex_t)*mutex);
        if (res != QOSA_OK) {
            _core_printf("destroy mutex failed: %d\n", res);
        }
        *mutex = QOSA_NULL;
    }
}

aiot_sysdep_portfile_t g_aiot_sysdep_portfile = {
    .core_sysdep_malloc = core_sysdep_malloc,
    .core_sysdep_free = core_sysdep_free,
    .core_sysdep_time = core_sysdep_time,
    .core_sysdep_sleep = core_sysdep_sleep,
    .core_sysdep_network_init = core_sysdep_network_init,
    .core_sysdep_network_setopt = core_sysdep_network_setopt,
    .core_sysdep_network_establish = core_sysdep_network_establish,
    .core_sysdep_network_recv = core_sysdep_network_recv,
    .core_sysdep_network_send = core_sysdep_network_send,
    .core_sysdep_network_deinit = core_sysdep_network_deinit,
    .core_sysdep_rand = core_sysdep_rand,
    .core_sysdep_mutex_init = core_sysdep_mutex_init,
    .core_sysdep_mutex_lock = core_sysdep_mutex_lock,
    .core_sysdep_mutex_unlock = core_sysdep_mutex_unlock,
    .core_sysdep_mutex_deinit = core_sysdep_mutex_deinit,
};

