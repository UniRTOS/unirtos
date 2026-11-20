/*
 * Copyright (c) 2002 Florian Schulze.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ftpd.c - This file is part of the FTP daemon for lwIP
 *
 */
#include <common/bk_include.h>
#include <components/log.h>
#if CONFIG_FTP_SERVER
#include <os/mem.h>
#include "lwip/debug.h"
#include "lwip/stats.h"

#include "ftpd.h"

#include "lwip/tcp.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#if CONFIG_VFS
#include "bk_posix.h"
#endif
#include <sys/stat.h>


#define TAG "FTP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define FTP_USER        "bk7258"
#define FTP_PASSWORD    "123456"

uint32_t ftp_is_running = 0;

#if 0
#define EINVAL 1
#define ENOMEM 2
#define ENODEV 3
#endif

#define msg110 "110 MARK %s = %s."
/*
         110 Restart marker reply.
             In this case, the text is exact and not left to the
             particular implementation; it must read:
                  MARK yyyy = mmmm
             Where yyyy is User-process data stream marker, and mmmm
             server's equivalent marker (note the spaces between markers
             and "=").
*/
#define msg120 "120 Service ready in nnn minutes."
#define msg125 "125 Data connection already open; transfer starting."
#define msg150 "150 File status okay; about to open data connection."
#define msg150recv "150 Opening BINARY mode data connection for %s (%i bytes)."
#define msg150stor "150 Opening BINARY mode data connection for %s."
#define msg200 "200 Command okay."
#define msg202 "202 Command not implemented, superfluous at this site."
#define msg211 "211 System status, or system help reply."
#define msg212 "212 Directory status."
#define msg213 "213 %ld"
#define msg214 "214 %s."
/*
             214 Help message.
             On how to use the server or the meaning of a particular
             non-standard command.  This reply is useful only to the
             human user.
*/
#define msg214SYST "214 %s system type."
/*
         215 NAME system type.
             Where NAME is an official system name from the list in the
             Assigned Numbers document.
*/
#define msg220 "220 lwIP FTP Server ready."
/*
         220 Service ready for new user.
*/
#define msg221 "221 Goodbye."
/*
         221 Service closing control connection.
             Logged out if appropriate.
*/
#define msg225 "225 Data connection open; no transfer in progress."
#define msg226 "226 Closing data connection."
#define msg226_1 "226 Transfer ok."
/*
             Requested file action successful (for example, file
             transfer or file abort).
*/
#define msg227 "227 Entering Passive Mode (%i,%i,%i,%i,%i,%i)."
/*
         227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
*/
#define msg230 "230 User logged in, proceed."
#define msg250 "250 Requested file action okay, completed."
#define msg257PWD "257 \"%s\" is current directory."
#define msg257 "257 \"%s\" created."
/*
         257 "PATHNAME" created.
*/
#define msg331 "331 User name okay, need password."
#define msg332 "332 Need account for login."
#define msg350 "350 Requested file action pending further information."
#define msg421 "421 Service not available, closing control connection."
/*
             This may be a reply to any command if the service knows it
             must shut down.
*/
#define msg425 "425 Can't open data connection."
#define msg426 "426 Connection closed; transfer aborted."
#define msg450 "450 Requested file action not taken."
/*
             File unavailable (e.g., file busy).
*/
#define msg451 "451 Requested action aborted: local error in processing."
#define msg452 "452 Requested action not taken."
/*
             Insufficient storage space in system.
*/
#define msg500 "500 Syntax error, command unrecognized."
/*
             This may include errors such as command line too long.
*/
#define msg501 "501 Syntax error in parameters or arguments."
#define msg502 "502 Command not implemented."
#define msg503 "503 Bad sequence of commands."
#define msg504 "504 Command not implemented for that parameter."
#define msg530 "530 Not logged in."
#define msg532 "532 Need account for storing files."
#define msg550 "550 Requested action not taken."
/*
             File unavailable (e.g., file not found, no access).
*/
#define msg551 "551 Requested action aborted: page type unknown."
#define msg552 "552 Requested file action aborted."
/*
             Exceeded storage allocation (for current directory or
             dataset).
*/
#define msg553 "553 Requested action not taken."


#define msg_FEAT   "211-Extension supported\r\n UTF8\r\n MLSD\r\n MLST\r\n CLNT\r\n SIZE\r\n211 End."

/*
             File name not allowed.
*/

enum ftpd_state_e {
	FTPD_USER,
	FTPD_PASS,
	FTPD_IDLE,
	FTPD_NLST,
	FTPD_LIST,
	FTPD_RETR,
	FTPD_RNFR,
	FTPD_STOR,
	FTPD_QUIT,
	FTPD_MLSD,
};

static const char *ftpd_state_name(enum ftpd_state_e state)
{
	switch (state) {
	case FTPD_USER:
		return "USER";
	case FTPD_PASS:
		return "PASS";
	case FTPD_IDLE:
		return "IDLE";
	case FTPD_NLST:
		return "NLST";
	case FTPD_LIST:
		return "LIST";
	case FTPD_RETR:
		return "RETR";
	case FTPD_RNFR:
		return "RNFR";
	case FTPD_STOR:
		return "STOR";
	case FTPD_QUIT:
		return "QUIT";
	case FTPD_MLSD:
		return "MLSD";
	default:
		return "UNKNOWN";
	}
}

static const char *month_table[12] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dez"
};

/*
------------------------------------------------------------
	SFIFO 1.3
------------------------------------------------------------
 * Simple portable lock-free FIFO
 * (c) 2000-2002, David Olofson
 *
 * Platform support:
 *	gcc / Linux / x86:		Works
 *	gcc / Linux / x86 kernel:	Works
 *	gcc / FreeBSD / x86:		Works
 *	gcc / NetBSD / x86:		Works
 *	gcc / Mac OS X / PPC:		Works
 *	gcc / Win32 / x86:		Works
 *	Borland C++ / DOS / x86RM:	Works
 *	Borland C++ / Win32 / x86PM16:	Untested
 *	? / Various Un*ces / ?:		Untested
 *	? / Mac OS / PPC:		Untested
 *	gcc / BeOS / x86:		Untested
 *	gcc / BeOS / PPC:		Untested
 *	? / ? / Alpha:			Untested
 *
 * 1.2: Max buffer size halved, to avoid problems with
 *	the sign bit...
 *
 * 1.3:	Critical buffer allocation bug fixed! For certain
 *	requested buffer sizes, older version would
 *	allocate a buffer of insufficient size, which
 *	would result in memory thrashing. (Amazing that
 *	I've manage to use this to the extent I have
 *	without running into this... *heh*)
 */

/*
 * Porting note:
 *	Reads and writes of a variable of this type in memory
 *	must be *atomic*! 'int' is *not* atomic on all platforms.
 *	A safe type should be used, and  sfifo should limit the
 *	maximum buffer size accordingly.
 */
typedef int sfifo_atomic_t;
#ifdef __TURBOC__
#	define	SFIFO_MAX_BUFFER_SIZE	0x7fff
#else /* Kludge: Assume 32 bit platform */
#	define	SFIFO_MAX_BUFFER_SIZE	0x7fffffff
#endif

typedef struct sfifo_t {
	char *buffer;
	int size;			/* Number of bytes */
	sfifo_atomic_t readpos;		/* Read position */
	sfifo_atomic_t writepos;	/* Write position */
} sfifo_t;

#define SFIFO_SIZEMASK(x)	((x)->size - 1)

#define sfifo_used(x)	(((x)->writepos - (x)->readpos) & SFIFO_SIZEMASK(x))
#define sfifo_space(x)	((x)->size - 1 - sfifo_used(x))

#define DBG(x)

#define FTP_TEMP_BUFFER_LEN  1024
#define DATA_CMD_MAX_SIZE  640
#define FULL_PATH_SIZE 640
#define MAX_PRE_READ_BUFFER_SIZE 30*1024//8000
#define MAX_READ_BUFFER_SIZE 30*1024//8*1024
#define FTPD_DIR_ENTRY_BATCH 64
#define FTP_TIMEZONE_OFFSET_SECONDS (8 * 60 * 60)

static time_t ftpd_time_to_utc(time_t local_time)
{
#if CONFIG_NTP_SYNC_RTC
	if (local_time >= FTP_TIMEZONE_OFFSET_SECONDS)
		return local_time - FTP_TIMEZONE_OFFSET_SECONDS;
#endif
	return local_time;
}

static char *strprepend(char *dest,int size,const char *pre_str)
{
	if (dest == NULL || pre_str == NULL || size == 0) return NULL;
    size_t src_len = strlen(pre_str);
    size_t dest_len = strlen(dest);
    size_t total_needed = src_len + dest_len + 1;

    if (total_needed > size) {
        LOGE("%s size %d, str1 %s,str2 %s\r\n",__func__,size,dest,pre_str);
        return NULL;
    }
	memmove(dest + src_len, dest, dest_len + 1);
	memcpy(dest, pre_str, src_len);
	return dest;
}

static int sfifo_init(sfifo_t *f, int size)
{
	memset(f, 0, sizeof(sfifo_t));

	if (size > SFIFO_MAX_BUFFER_SIZE)
		return -EINVAL;

	f->size = 1;
	for (; f->size <= size; f->size <<= 1)
		;

	if (0 == (f->buffer = (void *)ftp_malloc(f->size)))
		return -ENOMEM;

	return 0;
}

static void sfifo_close(sfifo_t *f)
{
	if (f->buffer)
		os_free(f->buffer);
}

static int sfifo_write(sfifo_t *f, const void *_buf, int len)
{
	int total;
	int i;
	const char *buf = (const char *)_buf;

	if (!f->buffer)
		return -ENODEV;	/* No buffer! */

	total = sfifo_space(f);
	DBG(LOGI("sfifo_space() = %d\r\n", total));
	if (len > total)
		len = total;
	else
		total = len;

	i = f->writepos;
	if (i + len > f->size) {
		memcpy(f->buffer + i, buf, f->size - i);
		buf += f->size - i;
		len -= f->size - i;
		i = 0;
	}
	memcpy(f->buffer + i, buf, len);
	f->writepos = i + len;

	return total;
}

struct ftpd_datastate {
	int connected;
	DIR* vfs_dir;              // vfs_dir_t *vfs_dir;
	struct dirent* vfs_dirent; //vfs_dirent_t *vfs_dirent;
	int fd;                    //vfs_file_t *vfs_file;
	sfifo_t fifo;
	struct tcp_pcb *msgpcb;
	struct ftpd_msgstate *msgfs;
};

struct ftpd_msgstate {
	enum ftpd_state_e state;
	sfifo_t fifo;
	// vfs_t *vfs;
	ip_addr_t dataip;
	u16_t dataport;
	struct tcp_pcb *datapcb;
	struct ftpd_datastate *datafs;
	int passive;
	char *renamefrom;
	off_t restart_offset;
};

static void send_msg(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm, char *msg, ...);

static const char *ftp_path_skip_extra_root_slash(const char *path)
{
	if (path && path[0] == '/' && path[1] == '/')
		return path + 1;

	return path;
}

static const char *ftp_mount_name(void)
{
	const char *name = strrchr(FTP_MOUNT_PATH, '/');

	return name ? name + 1 : FTP_MOUNT_PATH;
}

static int ftp_path_has_mount_prefix(const char *path)
{
	size_t mount_len = strlen(FTP_MOUNT_PATH);

	path = ftp_path_skip_extra_root_slash(path);
	if (!path)
		return 0;

	return (strncmp(path, FTP_MOUNT_PATH, mount_len) == 0) &&
		(path[mount_len] == '\0' || path[mount_len] == '/');
}

static int ftp_append_path_segment(char *out, size_t out_size, const char *seg, size_t seg_len)
{
	size_t out_len = strlen(out);

	if ((out_len + 1 + seg_len + 1) > out_size)
		return -1;

	out[out_len++] = '/';
	memcpy(out + out_len, seg, seg_len);
	out[out_len + seg_len] = '\0';

	return 0;
}

static int ftp_normalize_mount_path(const char *path, char *out, size_t out_size)
{
	char input[MAX_PATH_LEN] = {0};
	const char *p;
	size_t mount_len = strlen(FTP_MOUNT_PATH);

	if (!path || !out || out_size <= mount_len)
		return -1;

	snprintf(input, sizeof(input), "%s", path);
	snprintf(out, out_size, "%s", FTP_MOUNT_PATH);
	p = ftp_path_skip_extra_root_slash(input);
	if (!p)
		return -1;

	if (ftp_path_has_mount_prefix(p)) {
		p += mount_len;
		if (*p == '/')
			p++;
	} else if (*p == '/') {
		p++;
	}

	while (*p != '\0') {
		const char *seg = p;
		size_t seg_len;

		while (*p != '\0' && *p != '/')
			p++;
		seg_len = p - seg;

		if (seg_len == 0 || (seg_len == 1 && seg[0] == '.')) {
			/* skip */
		} else if (seg_len == 2 && seg[0] == '.' && seg[1] == '.') {
			if (strlen(out) > mount_len) {
				char *last_slash = strrchr(out, '/');

				if (last_slash && last_slash > out)
					*last_slash = '\0';
			}
		} else if (ftp_append_path_segment(out, out_size, seg, seg_len) != 0) {
			return -1;
		}

		while (*p == '/')
			p++;
	}

	return 0;
}

static int ftp_make_path(const char *arg, char *out, size_t out_size)
{
	char cwd[MAX_PATH_LEN] = {0};
	char base[MAX_PATH_LEN] = {0};
	char candidate[MAX_PATH_LEN] = {0};
	char *cwd_ret;
	const char *cwd_path;
	const char *mount_name = ftp_mount_name();
	size_t mount_name_len = strlen(mount_name);

	if (!arg || arg[0] == '\0' || !out)
		return -1;

	cwd_ret = getcwd(cwd, MAX_PATH_LEN);
	cwd_path = ftp_path_skip_extra_root_slash(cwd_ret);
	if (!cwd_path || cwd_path[0] == '\0' || (cwd_path[0] == '/' && cwd_path[1] == '\0')) {
		snprintf(base, sizeof(base), "%s", FTP_MOUNT_PATH);
	} else if (ftp_normalize_mount_path(cwd_path, base, sizeof(base)) != 0) {
		return -1;
	}

	if (arg[0] == '/' && arg[1] == '\0') {
		snprintf(candidate, sizeof(candidate), "%s", FTP_MOUNT_PATH);
	} else if (ftp_path_has_mount_prefix(arg)) {
		snprintf(candidate, sizeof(candidate), "%s", ftp_path_skip_extra_root_slash(arg));
	} else if (arg[0] == '/') {
		snprintf(candidate, sizeof(candidate), "%s%s", FTP_MOUNT_PATH, arg);
	} else if ((strcmp(base, FTP_MOUNT_PATH) == 0) &&
		   (strncmp(arg, mount_name, mount_name_len) == 0) &&
		   (arg[mount_name_len] == '\0' || arg[mount_name_len] == '/')) {
		snprintf(candidate, sizeof(candidate), "%s", FTP_MOUNT_PATH);
		if (arg[mount_name_len] == '/')
			snprintf(candidate, sizeof(candidate), "%s/%s", FTP_MOUNT_PATH, arg + mount_name_len + 1);
	} else {
		snprintf(candidate, sizeof(candidate), "%s/%s", base, arg);
	}

	return ftp_normalize_mount_path(candidate, out, out_size);
}

static int ftpd_is_list_state(enum ftpd_state_e state)
{
	return state == FTPD_LIST || state == FTPD_NLST || state == FTPD_MLSD;
}

static void ftpd_data_cleanup_resources(struct ftpd_datastate *fsd)
{
	if (!fsd)
		return;

	if (fsd->vfs_dir) {
		closedir(fsd->vfs_dir);
		fsd->vfs_dir = NULL;
		fsd->vfs_dirent = NULL;
	}
	if (fsd->fd != -1) {
		close(fsd->fd);
		fsd->fd = -1;
	}
}

static void ftpd_dataerr(void *arg, err_t err)
{
	struct ftpd_datastate *fsd = arg;
	struct ftpd_msgstate *fsm;

	if (fsd == NULL)
		return;
	fsm = fsd->msgfs;
	LOGE("ftpd_dataerr: %s (%i) fsd=0x%x fsm=0x%x state=%s datafs=0x%x datapcb=0x%x connected=%d fd=%d vfs_dir=0x%x fifo_used=%d fifo_space=%d\r\n",
		lwip_strerr(err), err, (unsigned int)fsd, (unsigned int)fsm,
		fsm ? ftpd_state_name(fsm->state) : "(null)",
		fsm ? (unsigned int)fsm->datafs : 0,
		fsm ? (unsigned int)fsm->datapcb : 0,
		fsd->connected, fsd->fd, (unsigned int)fsd->vfs_dir,
		sfifo_used(&fsd->fifo), sfifo_space(&fsd->fifo));
	ftpd_data_cleanup_resources(fsd);
	sfifo_close(&fsd->fifo);
	if (fsm) {
		fsm->datafs = NULL;
		fsm->datapcb = NULL;
		fsm->passive = 0;
		fsm->state = FTPD_IDLE;
	}
	os_free(fsd);
}

static void ftpd_dataclose(struct tcp_pcb *pcb, struct ftpd_datastate *fsd)
{
	struct ftpd_msgstate *fsm = fsd ? fsd->msgfs : NULL;
	err_t close_err;

	LOGV("ftpd_dataclose: begin pcb=0x%x pcb_state=%d fsd=0x%x fsm=0x%x state=%s connected=%d fd=%d vfs_dir=0x%x fifo_used=%d fifo_space=%d\r\n",
		(unsigned int)pcb, pcb ? pcb->state : -1, (unsigned int)fsd,
		(unsigned int)fsm, fsm ? ftpd_state_name(fsm->state) : "(null)",
		fsd ? fsd->connected : -1, fsd ? fsd->fd : -1,
		fsd ? (unsigned int)fsd->vfs_dir : 0,
		fsd ? sfifo_used(&fsd->fifo) : -1,
		fsd ? sfifo_space(&fsd->fifo) : -1);
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	fsd->msgfs->datafs = NULL;
	fsd->msgfs->passive = 0;
	ftpd_data_cleanup_resources(fsd);
	sfifo_close(&fsd->fifo);
	os_free(fsd);
	tcp_arg(pcb, NULL);
	close_err = tcp_close(pcb);
	LOGV("ftpd_dataclose: tcp_close pcb=0x%x ret=%d\r\n",
		(unsigned int)pcb, close_err);
}

static int send_data(struct tcp_pcb *pcb, struct ftpd_datastate *fsd)
{
	err_t err;
	u16_t len;

	if (sfifo_used(&fsd->fifo) > 0) {
		int i;

		/* We cannot send more data than space available in the send
		   buffer. */
		if (tcp_sndbuf(pcb) < sfifo_used(&fsd->fifo))
			len = tcp_sndbuf(pcb);
		else
			len = (u16_t) sfifo_used(&fsd->fifo);

		if (len == 0) {
			LOGV("send_data: wait sndbuf state=%s pcb=0x%x pcb_state=%d fsd=0x%x fifo_used=%d fifo_space=%d\r\n",
				fsd->msgfs ? ftpd_state_name(fsd->msgfs->state) : "(null)",
				(unsigned int)pcb, pcb ? pcb->state : -1,
				(unsigned int)fsd, sfifo_used(&fsd->fifo),
				sfifo_space(&fsd->fifo));
			return 0;
		}

		LOGV("send_data: state=%s pcb=0x%x pcb_state=%d fsd=0x%x sndbuf=%u fifo_used=%d fifo_space=%d send_len=%u\r\n",
			fsd->msgfs ? ftpd_state_name(fsd->msgfs->state) : "(null)",
			(unsigned int)pcb, pcb ? pcb->state : -1, (unsigned int)fsd,
			pcb ? tcp_sndbuf(pcb) : 0, sfifo_used(&fsd->fifo),
			sfifo_space(&fsd->fifo), len);
		i = fsd->fifo.readpos;
		if ((i + len) > fsd->fifo.size) {
			err = tcp_write(pcb, fsd->fifo.buffer + i, (u16_t)(fsd->fifo.size - i), 1);
			if (err != ERR_OK) {
				LOGE("send_data: error writing first chunk! state=%s err=%d pcb=0x%x pcb_state=%d len=%u fifo_used=%d\r\n",
					fsd->msgfs ? ftpd_state_name(fsd->msgfs->state) : "(null)",
					err, (unsigned int)pcb, pcb ? pcb->state : -1,
					(u16_t)(fsd->fifo.size - i), sfifo_used(&fsd->fifo));
				return -1;
			}
			len -= fsd->fifo.size - i;
			fsd->fifo.readpos = 0;
			i = 0;
		}

		if (len > 0) {
			err = tcp_write(pcb, fsd->fifo.buffer + i, len, 1);
			if (err != ERR_OK) {
				LOGE("send_data: error writing second chunk! state=%s err=%d pcb=0x%x pcb_state=%d len=%u fifo_used=%d\r\n",
					fsd->msgfs ? ftpd_state_name(fsd->msgfs->state) : "(null)",
					err, (unsigned int)pcb, pcb ? pcb->state : -1,
					len, sfifo_used(&fsd->fifo));
				return -1;
			}
			fsd->fifo.readpos += len;
		}
		tcp_output(pcb);
		return 1;
	}

	return 0;
}

static void send_file(struct ftpd_datastate *fsd, struct tcp_pcb *pcb)
{
	uint8_t *buffer = NULL;
	uint32_t temp_len = MAX_READ_BUFFER_SIZE;
	int len;

	if (!fsd->connected)
		return;

	if (-1 != fsd->fd) {

		buffer = ftp_malloc(temp_len);
		if (buffer == NULL) {
			LOGE(" buffer test malloc fail len %d\r\n",temp_len);
			goto error;
		}

		len = sfifo_space(&fsd->fifo);

		if (len == 0) {
			send_data(pcb, fsd);
			goto error;
		}

		if (len > temp_len)
			len = temp_len;

		len = read(fsd->fd, buffer, len);

		if (len <= 0) {
			if (feof(fsd->fd) == 0)
				goto error;

			close(fsd->fd);
			fsd->fd = -1;
			goto error;
		}

		sfifo_write(&fsd->fifo, buffer, len);
		os_free(buffer);
		send_data(pcb, fsd);
	} else {
		struct ftpd_msgstate *fsm;
		struct tcp_pcb *msgpcb;

		if (sfifo_used(&fsd->fifo) > 0) {
			send_data(pcb, fsd);
			return;
		}
		fsm = fsd->msgfs;
		msgpcb = fsd->msgpcb;

		// close(fsd->fd);
		// fsd->fd = -1;
		ftpd_dataclose(pcb, fsd);
		fsm->datapcb = NULL;
		fsm->datafs = NULL;
		fsm->state = FTPD_IDLE;
		send_msg(msgpcb, fsm, msg226);
		return;
	}

	return;
error:
	if (buffer) {
		os_free(buffer);
		buffer = NULL;
	}
}

static void send_next_directory(struct ftpd_datastate *fsd, struct tcp_pcb *pcb, int list_type)
{
	char *cwd_path = NULL;
	char *cwd_buffer = NULL;
	char *buffer = NULL;
	char *path = NULL;
	int len;
	int processed = 0;
	time_t current_time = {0};
	int current_year = 0;
	struct tm *current_tm = NULL;

	LOGV("%s list_type %d\r\n",__func__,list_type);

	buffer = os_malloc(MAX_PATH_LEN);
	if(buffer == NULL) {
		LOGE("send_next_directory: Out of memory\r\n");
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);
	path = os_malloc(MAX_PATH_LEN);
	if(path == NULL) {
		LOGE("send_next_directory: Out of memory\r\n");
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);
	cwd_buffer = os_malloc(MAX_PATH_LEN);
	if(cwd_buffer == NULL) {
		LOGE("send_next_directory: Out of memory\r\n");
		goto exit;
	}
	os_memset(cwd_buffer, 0, MAX_PATH_LEN);

	if (sfifo_used(&fsd->fifo) > 0 && tcp_sndbuf(pcb) == 0)
		goto exit;

	cwd_path = getcwd(cwd_buffer, MAX_PATH_LEN);
	if (!cwd_path || ftp_normalize_mount_path(cwd_path, cwd_buffer, MAX_PATH_LEN) != 0)
		snprintf(cwd_buffer, MAX_PATH_LEN, "%s", FTP_MOUNT_PATH);

#if CONFIG_NTP_SYNC_RTC
	{
		extern time_t timestamp_get();
		current_time = timestamp_get();
	}
#else
	/* Fallback to time() if NTP is not configured */
	current_time = time(NULL);
#endif
	current_tm = gmtime(&current_time);
	current_year = current_tm ? current_tm->tm_year : 0;

	while (processed < FTPD_DIR_ENTRY_BATCH) {
		if (fsd->vfs_dirent == NULL) {
			fsd->vfs_dirent = readdir(fsd->vfs_dir);
		}

		if (fsd->vfs_dirent) {
			if (list_type == FTPD_NLST) {
				len = sprintf(buffer, "%s\r\n", fsd->vfs_dirent->d_name);
				if (sfifo_space(&fsd->fifo) < len) {
					send_data(pcb, fsd);
					goto exit;
				}
				sfifo_write(&fsd->fifo, buffer, len);
				fsd->vfs_dirent = NULL;
				LOGV("NLST : %s",buffer);
			} else {
				struct stat st = {0};
				struct tm *s_time = NULL;
				time_t entry_time = 0;
				int stat_ok = 0;

				LOGV("%s cwd_path %s\r\n",__func__,cwd_buffer);
				snprintf(path, MAX_PATH_LEN, "%s/%s", cwd_buffer, fsd->vfs_dirent->d_name);
				LOGV("%s path %s\r\n",__func__,path);
				if (fsd->vfs_dirent->d_stat_valid) {
					st.st_size = fsd->vfs_dirent->d_size;
					st.st_mtime = fsd->vfs_dirent->d_mtime;
					st.st_mode = fsd->vfs_dirent->d_mode;
					stat_ok = 1;
				} else if (list_type == FTPD_LIST || list_type == FTPD_MLSD) {
					stat_ok = (stat(path, &st) == 0);
				}

				/* If st_mtime is 0 (file system doesn't support timestamps),
				 * use current time instead */
				entry_time = (!stat_ok || st.st_mtime == 0) ? current_time : st.st_mtime;
				if (list_type == FTPD_MLSD)
					entry_time = ftpd_time_to_utc(entry_time);
				s_time = gmtime(&entry_time);
				if (!s_time)
					goto exit;
				if(list_type == FTPD_LIST){
					if (s_time->tm_year == current_year) {
						len = sprintf(buffer, "-rw-rw-rw-   1 user     ftp  %11ld %s %02i %02i:%02i %s\r\n", st.st_size, month_table[s_time->tm_mon], s_time->tm_mday, s_time->tm_hour, s_time->tm_min, fsd->vfs_dirent->d_name);
					} else {
						len = sprintf(buffer, "-rw-rw-rw-   1 user     ftp  %11ld %s %02i %5i %s\r\n", st.st_size, month_table[s_time->tm_mon], s_time->tm_mday, s_time->tm_year + 1900, fsd->vfs_dirent->d_name);
					}
					if (S_ISDIR(st.st_mode))
						buffer[0] = 'd';
					LOGV("LIST : %s",buffer);
				} else {
					//MLSD
					if (stat_ok && S_ISDIR(st.st_mode))
					{
						/* MLSD format: modify=YYYYMMDDHHmmss */
						len = sprintf(buffer, "type=%s;perm=%s;modify=%04d%02d%02d%02d%02d%02d; %s\r\n",
									 "dir","elrwx",
									 s_time->tm_year + 1900,
									 s_time->tm_mon + 1,
									 s_time->tm_mday,
									 s_time->tm_hour,
									 s_time->tm_min,
									 s_time->tm_sec,
									 fsd->vfs_dirent->d_name);
					} 
					else 
					{
						if (stat_ok) {
							/* MLSD format: modify=YYYYMMDDHHmmss */
							len = sprintf(buffer, "type=%s;perm=%s;modify=%04d%02d%02d%02d%02d%02d;size=%ld; %s\r\n",
										"file","rwx",
										s_time->tm_year + 1900,
										s_time->tm_mon + 1,
										s_time->tm_mday,
										s_time->tm_hour,
										s_time->tm_min,
										s_time->tm_sec,
										st.st_size,
										fsd->vfs_dirent->d_name);
						} else {
							len = sprintf(buffer, "type=file;perm=rwx; %s\r\n",
								fsd->vfs_dirent->d_name);
						}
					}
				}
				LOGV("MLSD : %s",buffer);

				if (sfifo_space(&fsd->fifo) < len) {
					LOGV("send_next_directory: fifo full state=%s fsd=0x%x vfs_dir=0x%x entry=%s need=%d used=%d space=%d\r\n",
						fsd->msgfs ? ftpd_state_name(fsd->msgfs->state) : "(null)",
						(unsigned int)fsd, (unsigned int)fsd->vfs_dir,
						fsd->vfs_dirent ? fsd->vfs_dirent->d_name : "(null)",
						len, sfifo_used(&fsd->fifo), sfifo_space(&fsd->fifo));
					send_data(pcb, fsd);
					goto exit;
				}

				sfifo_write(&fsd->fifo, buffer, len);
				fsd->vfs_dirent = NULL;
			} 
			processed++;
		} else {
			struct ftpd_msgstate *fsm;
			struct tcp_pcb *msgpcb;

			if (sfifo_used(&fsd->fifo) > 0) {
				send_data(pcb, fsd);
				goto exit;
			}
			fsm = fsd->msgfs;
			msgpcb = fsd->msgpcb;
			if(fsd->vfs_dir) {
				closedir(fsd->vfs_dir);
				fsd->vfs_dir = NULL;
			}
			LOGV("send_next_directory: complete state=%s fsd=0x%x pcb=0x%x fifo_used=%d fifo_space=%d\r\n",
				fsm ? ftpd_state_name(fsm->state) : "(null)",
				(unsigned int)fsd, (unsigned int)pcb,
				sfifo_used(&fsd->fifo), sfifo_space(&fsd->fifo));
			ftpd_dataclose(pcb, fsd);
			fsm->datapcb = NULL;
			fsm->datafs = NULL;
			fsm->state = FTPD_IDLE;
			send_msg(msgpcb, fsm, msg226_1);
			goto exit;
		}
	}

	if (sfifo_used(&fsd->fifo) > 0)
		send_data(pcb, fsd);

exit:
	if (path)
		os_free(path);
	if (cwd_buffer)
		os_free(cwd_buffer);
	if (buffer)
		os_free(buffer);
}

static err_t ftpd_datasent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	struct ftpd_datastate *fsd = arg;
	struct ftpd_msgstate *fsm = fsd ? fsd->msgfs : NULL;

	LOGV("ftpd_datasent: ack_len=%u pcb=0x%x pcb_state=%d fsd=0x%x state=%s fd=%d vfs_dir=0x%x fifo_used=%d fifo_space=%d\r\n",
		len, (unsigned int)pcb, pcb ? pcb->state : -1, (unsigned int)fsd,
		fsm ? ftpd_state_name(fsm->state) : "(null)",
		fsd ? fsd->fd : -1, fsd ? (unsigned int)fsd->vfs_dir : 0,
		fsd ? sfifo_used(&fsd->fifo) : -1,
		fsd ? sfifo_space(&fsd->fifo) : -1);

	switch (fsd->msgfs->state) {
	case FTPD_LIST:
	case FTPD_NLST:
	case FTPD_MLSD:
		send_next_directory(fsd, pcb, fsd->msgfs->state);
		break;
	case FTPD_RETR:
		send_file(fsd, pcb);
		break;
	default:
		break;
	}

	return ERR_OK;
}

static err_t ftpd_datarecv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	struct ftpd_datastate *fsd = arg;
	struct ftpd_msgstate *fsm = fsd ? fsd->msgfs : NULL;

	LOGV("ftpd_datarecv: err=%d p=0x%x p_tot_len=%u pcb=0x%x pcb_state=%d fsd=0x%x state=%s fd=%d vfs_dir=0x%x fifo_used=%d fifo_space=%d\r\n",
		err, (unsigned int)p, p ? p->tot_len : 0, (unsigned int)pcb,
		pcb ? pcb->state : -1, (unsigned int)fsd,
		fsm ? ftpd_state_name(fsm->state) : "(null)",
		fsd ? fsd->fd : -1, fsd ? (unsigned int)fsd->vfs_dir : 0,
		fsd ? sfifo_used(&fsd->fifo) : -1,
		fsd ? sfifo_space(&fsd->fifo) : -1);

	if (err == ERR_OK && p != NULL) {
		struct pbuf *q;
		u16_t tot_len = 0;

		if (fsm && fsm->state != FTPD_STOR) {
			LOGE("ftpd_datarecv: unexpected payload on non-upload data connection state=%s p_tot_len=%u fd=%d\r\n",
				ftpd_state_name(fsm->state), p->tot_len, fsd->fd);
			tcp_recved(pcb, p->tot_len);
			pbuf_free(p);
			return ERR_OK;
		}

		for (q = p; q != NULL; q = q->next) {
			int len;

			len = write(fsd->fd, q->payload, q->len);
			tot_len += len;
			if (len != q->len)
				break;
		}

		tcp_recved(pcb, tot_len);
		pbuf_free(p);
	}
	if (err == ERR_OK && p == NULL) {
		struct ftpd_msgstate *fsm;
		struct tcp_pcb *msgpcb;

		fsm = fsd->msgfs;
		msgpcb = fsd->msgpcb;
		LOGV("ftpd_datarecv: remote closed data connection, state=%s datafs=0x%x datapcb=0x%x fd=%d vfs_dir=0x%x fifo_used=%d fifo_space=%d\r\n",
			fsm ? ftpd_state_name(fsm->state) : "(null)",
			fsm ? (unsigned int)fsm->datafs : 0,
			fsm ? (unsigned int)fsm->datapcb : 0,
			fsd->fd, (unsigned int)fsd->vfs_dir,
			sfifo_used(&fsd->fifo), sfifo_space(&fsd->fifo));
		if (fsm && ftpd_is_list_state(fsm->state)) {
			LOGE("ftpd_datarecv: list data connection closed by peer before server close, state=%s vfs_dir=0x%x fifo_used=%d\r\n",
				ftpd_state_name(fsm->state), (unsigned int)fsd->vfs_dir,
				sfifo_used(&fsd->fifo));
			ftpd_dataclose(pcb, fsd);
			fsm->datapcb = NULL;
			fsm->datafs = NULL;
			fsm->passive = 0;
			fsm->state = FTPD_IDLE;
			send_msg(msgpcb, fsm, msg426);
			return ERR_OK;
		}
		ftpd_dataclose(pcb, fsd);
		fsm->datapcb = NULL;
		fsm->datafs = NULL;
		fsm->passive = 0;
		fsm->state = FTPD_IDLE;
		send_msg(msgpcb, fsm, msg226);
	}

	return ERR_OK;
}

static err_t ftpd_dataconnected(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct ftpd_datastate *fsd = arg;

	fsd->msgfs->datapcb = pcb;
	fsd->connected = 1;
	LOGV("ftpd_dataconnected: err=%d state=%s pcb=0x%x pcb_state=%d fsd=0x%x fd=%d vfs_dir=0x%x sndbuf=%u\r\n",
		err, ftpd_state_name(fsd->msgfs->state), (unsigned int)pcb,
		pcb ? pcb->state : -1, (unsigned int)fsd, fsd->fd,
		(unsigned int)fsd->vfs_dir, pcb ? tcp_sndbuf(pcb) : 0);

	tcp_recv(pcb, ftpd_datarecv);
	tcp_sent(pcb, ftpd_datasent);
	tcp_err(pcb, ftpd_dataerr);

	switch (fsd->msgfs->state) {
	case FTPD_LIST:
	case FTPD_NLST:
	case FTPD_MLSD:
		send_next_directory(fsd, pcb, fsd->msgfs->state);
		break;
	case FTPD_RETR:
		send_file(fsd, pcb);
		break;
	default:
		break;
	}

	return ERR_OK;
}

static err_t ftpd_dataaccept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct ftpd_datastate *fsd = arg;

	LOGV("ftpd_dataaccept: err=%d state=%s listen_pcb=0x%x listen_state=%d new_pcb=0x%x new_state=%d datafs=0x%x fd=%d vfs_dir=0x%x sndbuf=%u\r\n",
		err, ftpd_state_name(fsd->msgfs->state),
		(unsigned int)fsd->msgfs->datapcb,
		fsd->msgfs->datapcb ? fsd->msgfs->datapcb->state : -1,
		(unsigned int)pcb, pcb ? pcb->state : -1,
		(unsigned int)fsd, fsd->fd, (unsigned int)fsd->vfs_dir,
		pcb ? tcp_sndbuf(pcb) : 0);
	tcp_close(fsd->msgfs->datapcb);
	fsd->msgfs->datapcb = pcb;
	fsd->connected = 1;

	tcp_recv(pcb, ftpd_datarecv);
	tcp_sent(pcb, ftpd_datasent);
	tcp_err(pcb, ftpd_dataerr);

	switch (fsd->msgfs->state) {
	case FTPD_LIST:
	case FTPD_NLST:
	case FTPD_MLSD:
		send_next_directory(fsd, pcb, fsd->msgfs->state);
		break;
	case FTPD_RETR:
		send_file(fsd, pcb);
		break;
	default:
		break;
	}

	return ERR_OK;
}

static int open_dataconnection(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (fsm->passive) {
		LOGV("open_dataconnection: passive state=%s datafs=0x%x datapcb=0x%x\r\n",
			ftpd_state_name(fsm->state), (unsigned int)fsm->datafs,
			(unsigned int)fsm->datapcb);
		return 0;
	}

	fsm->datafs = (struct ftpd_datastate *)ftp_malloc(sizeof(struct ftpd_datastate));
	if (fsm->datafs == NULL) {
		send_msg(pcb, fsm, msg451);
		return 1;
	}
	memset(fsm->datafs, 0, sizeof(struct ftpd_datastate));
	fsm->datafs->msgfs = fsm;
	fsm->datafs->msgpcb = pcb;
	fsm->datafs->fd = -1;
	sfifo_init(&fsm->datafs->fifo, MAX_PRE_READ_BUFFER_SIZE);
	fsm->datapcb = tcp_new();
	ip_set_option(fsm->datapcb, SOF_REUSEADDR);
	tcp_bind(fsm->datapcb, (ip_addr_t *)&pcb->local_ip, 20);
	tcp_arg(fsm->datapcb, fsm->datafs);
	ip_addr_t dataip;
	ip_addr_copy(dataip, fsm->dataip);
	tcp_connect(fsm->datapcb, &dataip, fsm->dataport, ftpd_dataconnected);
	LOGV("open_dataconnection: active state=%s datafs=0x%x datapcb=0x%x port=%u\r\n",
		ftpd_state_name(fsm->state), (unsigned int)fsm->datafs,
		(unsigned int)fsm->datapcb, fsm->dataport);

	return 0;
}

static void cmd_user(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (!strcmp(arg,FTP_USER))
	{
		send_msg(pcb, fsm, msg331);
		fsm->state = FTPD_PASS;
	}
	else
	{
		send_msg(pcb, fsm, msg530);
	}

}

static void cmd_pass(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (!strcmp(arg,FTP_PASSWORD))
	{
		send_msg(pcb, fsm, msg230);
		fsm->state = FTPD_IDLE;
	}
	else
	{
		send_msg(pcb, fsm, msg530);
	}
}

static void cmd_port(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	int nr;
	unsigned pHi, pLo;
	unsigned ip[4];

	nr = sscanf(arg, "%u,%u,%u,%u,%u,%u", &(ip[0]), &(ip[1]), &(ip[2]), &(ip[3]), &pHi, &pLo);
	if (nr != 6)
		send_msg(pcb, fsm, msg501);
	else {
		IP4_ADDR(&fsm->dataip, (u8_t) ip[0], (u8_t) ip[1], (u8_t) ip[2], (u8_t) ip[3]);
		fsm->dataport = ((u16_t) pHi << 8) | (u16_t) pLo;
		send_msg(pcb, fsm, msg200);
	}
}

static void cmd_quit(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	send_msg(pcb, fsm, msg221);
	fsm->state = FTPD_QUIT;
}

static void cmd_cwd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *new_path = NULL;

	new_path = os_malloc(MAX_PATH_LEN);
	if(new_path == NULL) {
		LOGE("cmd_cwd: malloc memory failed\r\n");
		goto exit;
	}
	os_memset(new_path, 0, MAX_PATH_LEN);

	if (arg == NULL || arg[0] == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (ftp_make_path(arg, new_path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	LOGI("cmd_cwd: arg=%s dir=%s\r\n", arg, new_path);
	errno = 0;
	if (chdir(new_path) == 0)
		send_msg(pcb, fsm, msg250);
	else {
		LOGE("cmd_cwd: chdir failed dir=%s errno=%d\r\n", new_path, errno);
		send_msg(pcb, fsm, msg550);
	}

exit:
	if (new_path)
		os_free(new_path);
}

static void cmd_cdup(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *new_path = NULL;

	new_path = os_malloc(MAX_PATH_LEN);
	if(new_path == NULL) {
		LOGE("%s: malloc memory failed\r\n",__func__);
		goto exit;
	}
	os_memset(new_path, 0, MAX_PATH_LEN);

	if (ftp_make_path("..", new_path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	LOGI("cmd_cdup: dir=%s\r\n", new_path);
	errno = 0;
	if (chdir(new_path) != 0) {
		LOGE("cmd_cdup: chdir failed dir=%s errno=%d\r\n", new_path, errno);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	send_msg(pcb, fsm, msg250,"CDUP");

exit:
	if (new_path)
		os_free(new_path);
}

static void cmd_pwd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *path = NULL;
	char *buffer = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if(buffer == NULL) {
		LOGE("cmd_pwd: malloc memory failed\r\n");
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	path = getcwd(buffer, MAX_PATH_LEN);
	if (!path)
		goto exit;
	if (ftp_normalize_mount_path(path, buffer, MAX_PATH_LEN) != 0)
		goto exit;
	send_msg(pcb, fsm, msg257PWD, buffer);

exit:
	if (buffer)
		os_free(buffer);
}

static void cmd_list_common(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm, int list_type)
{
	DIR *vfs_dir;
	char *cwd;
	char *buffer = NULL;
	char *path = NULL;

	buffer = os_malloc(MAX_PATH_LEN);
	if(buffer == NULL) {
		LOGE("cmd_list_common: Out of memory\r\n");
		goto exit;
	}
	os_memset(buffer, 0, MAX_PATH_LEN);

	path = os_malloc(MAX_PATH_LEN);
	if(path == NULL) {
		LOGE("cmd_list_common: Out of memory\r\n");
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);
	
	do {
		errno = 0;
		cwd = getcwd(buffer, MAX_PATH_LEN);
		if ((!cwd)) {
			LOGE("cmd_list_common: getcwd failed, list_type=%d errno=%d passive=%d datafs=0x%x datapcb=0x%x\r\n",
				list_type, errno, fsm->passive, (unsigned int)fsm->datafs,
				(unsigned int)fsm->datapcb);
			send_msg(pcb, fsm, msg451);
			break;
		}
		if (cwd[0] == '/' && cwd[1] == '/')
			cwd++;
		if (ftp_normalize_mount_path(cwd, path, MAX_PATH_LEN) != 0) {
			LOGE("cmd_list_common: normalize failed cwd=%s\r\n", cwd);
			send_msg(pcb, fsm, msg451);
			break;
		}
		LOGV("cmd_list_common: list_type=%d cwd=%s path=%s passive=%d datafs=0x%x datapcb=0x%x\r\n",
			list_type, cwd, path, fsm->passive, (unsigned int)fsm->datafs,
			(unsigned int)fsm->datapcb);
		errno = 0;
		vfs_dir = opendir(path);
		if (!vfs_dir) {
			LOGE("cmd_list_common: opendir failed path=%s errno=%d passive=%d datafs=0x%x datapcb=0x%x\r\n",
				path, errno, fsm->passive, (unsigned int)fsm->datafs,
				(unsigned int)fsm->datapcb);
			send_msg(pcb, fsm, msg451);
			break;
		}

		if (open_dataconnection(pcb, fsm) != 0) {
			LOGE("cmd_list_common: open data connection failed path=%s passive=%d datafs=0x%x datapcb=0x%x\r\n",
				path, fsm->passive, (unsigned int)fsm->datafs,
				(unsigned int)fsm->datapcb);
			closedir(vfs_dir);
			break;
		}

		fsm->datafs->vfs_dir = vfs_dir;
		fsm->datafs->vfs_dirent = NULL;

		LOGV("%s set fsm->state %d\r\n",__func__,list_type);
		fsm->state = list_type;

		send_msg(pcb, fsm, msg150);
	} while(0);

exit:
	if (path)
		os_free(path);
	if (buffer)
		os_free(buffer);
}

static void cmd_nlst(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	cmd_list_common(arg, pcb, fsm, FTPD_NLST);
}

static void cmd_list(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	cmd_list_common(arg, pcb, fsm, FTPD_LIST);
}

static void cmd_retr(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	int ret = -1;
	int fd = -1;
	struct stat st = {0};
	off_t transfer_size;
	char *path = NULL;

	path = os_malloc(MAX_PATH_LEN);
	if(path == NULL) {
		LOGE("cmd_pwd: Out of memory\r\n");
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (ftp_make_path(arg, path, MAX_PATH_LEN) != 0) {
		LOGE("cmd_retr: make path failed arg=%s\r\n", arg);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	LOGI("cmd_retr: arg=%s path=%s\r\n", arg, path);
	errno = 0;
	ret = stat(path, &st);
	if (0 != ret || !S_ISREG(st.st_mode)) {
		LOGE("cmd_retr: stat/type failed ret=%d errno=%d path=%s mode=0x%x size=%ld\r\n",
			ret, errno, path, (unsigned int)st.st_mode, (long)st.st_size);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	LOGV("cmd_retr: stat success path=%s mode=0x%x size=%ld restart_offset=%ld\r\n",
		path, (unsigned int)st.st_mode, (long)st.st_size, (long)fsm->restart_offset);
	errno = 0;
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		LOGE("cmd_retr: open failed path=%s errno=%d\r\n", path, errno);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	if (fsm->restart_offset > 0) {
		errno = 0;
		if (lseek(fd, fsm->restart_offset, SEEK_SET) == (off_t)-1) {
			LOGE("cmd_retr: lseek failed path=%s offset=%ld errno=%d\r\n",
				path, (long)fsm->restart_offset, errno);
			close(fd);
			send_msg(pcb, fsm, msg550);
			goto exit;
		}

		if (fsm->restart_offset >= st.st_size) {
			LOGE("cmd_retr: restart offset out of range path=%s offset=%ld size=%ld\r\n",
				path, (long)fsm->restart_offset, (long)st.st_size);
			close(fd);
			send_msg(pcb, fsm, msg550);
			goto exit;
		}
		transfer_size = st.st_size - fsm->restart_offset;
	} else {
		transfer_size = st.st_size;
	}

	send_msg(pcb, fsm, msg150recv, arg, transfer_size);

	if (open_dataconnection(pcb, fsm) != 0) {
		close(fd);
		goto exit;
	}

	fsm->datafs->fd = fd;
	fsm->state = FTPD_RETR;
	fsm->restart_offset = 0;

exit:
	if (path)
		os_free(path);
}

static void cmd_stor(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	int fd = -1; //vfs_file_t *vfs_file;
	char *path = NULL;

	path = os_malloc(MAX_PATH_LEN);
	if(path == NULL) {
		LOGE("cmd_pwd: Out of memory\r\n");
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (ftp_make_path(arg, path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	LOGI("cmd_stor: arg=%s path=%s\r\n", arg, path);
	fd = open(path, O_RDWR | O_CREAT | O_APPEND);
	if (-1 == fd) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	send_msg(pcb, fsm, msg150stor, arg);

	if (open_dataconnection(pcb, fsm) != 0) {
		close(fd);
		goto exit;
	}

	fsm->datafs->fd = fd;
	fsm->state = FTPD_STOR;

exit:
	if (path)
		os_free(path);
}

static void cmd_noop(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	send_msg(pcb, fsm, msg200);
}

static void cmd_syst(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	send_msg(pcb, fsm, msg214SYST, "UNIX");
}

static void cmd_pasv(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	static u16_t port = FTPD_COMMON_PORT;
	static u16_t start_port = FTPD_COMMON_PORT;
	struct tcp_pcb *temppcb;

	fsm->datafs = (struct ftpd_datastate *)ftp_malloc(sizeof(struct ftpd_datastate));

	if (fsm->datafs == NULL) {
		send_msg(pcb, fsm, msg451);
		return;
	}
	memset(fsm->datafs, 0, sizeof(struct ftpd_datastate));
	fsm->datafs->msgfs = fsm;
	fsm->datafs->fd = -1;

	fsm->datapcb = tcp_new();

	if (!fsm->datapcb) {
		os_free(fsm->datafs);
		fsm->datafs = NULL;
		send_msg(pcb, fsm, msg451);
		return;
	}

	sfifo_init(&fsm->datafs->fifo, MAX_PRE_READ_BUFFER_SIZE);

	start_port = port;

	while (1) {
		err_t err;

		if (++port > 0x7fff)
			port = FTPD_COMMON_PORT;

		fsm->dataport = port;
		err = tcp_bind(fsm->datapcb, (ip_addr_t *)&pcb->local_ip, fsm->dataport);
		if (err == ERR_OK)
			break;
		if (start_port == port)
			err = ERR_CLSD;
		if (err == ERR_USE)
			continue;
		if (err != ERR_OK) {
			ftpd_dataclose(fsm->datapcb, fsm->datafs);
			fsm->datapcb = NULL;
			fsm->datafs = NULL;
			return;
		}
	}

	temppcb = tcp_listen(fsm->datapcb);

	if (!temppcb) {

		ftpd_dataclose(fsm->datapcb, fsm->datafs);
		fsm->datapcb = NULL;
		fsm->datafs = NULL;
		return;
	}
	fsm->datapcb = temppcb;
	fsm->passive = 1;
	fsm->datafs->connected = 0;
	fsm->datafs->msgpcb = pcb;

	tcp_arg(fsm->datapcb, fsm->datafs);
	tcp_accept(fsm->datapcb, ftpd_dataaccept);
	send_msg(pcb, fsm, msg227, ip4_addr1(&pcb->local_ip), ip4_addr2(&pcb->local_ip), ip4_addr3(&pcb->local_ip), ip4_addr4(&pcb->local_ip), (fsm->dataport >> 8) & 0xff, (fsm->dataport) & 0xff);
}

static void cmd_abrt(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	LOGI("cmd_abrt: datafs=0x%x datapcb=0x%x passive=%d state=%d\r\n",
		(unsigned int)fsm->datafs, (unsigned int)fsm->datapcb, fsm->passive, fsm->state);
	if (fsm->datafs != NULL) {
		if (fsm->datapcb != NULL) {
			LOGI("cmd_abrt: close data connection datapcb=0x%x datafs=0x%x\r\n",
				(unsigned int)fsm->datapcb, (unsigned int)fsm->datafs);
			ftpd_dataclose(fsm->datapcb, fsm->datafs);
		} else {
			LOGI("cmd_abrt: free datafs without datapcb datafs=0x%x\r\n",
				(unsigned int)fsm->datafs);
			ftpd_data_cleanup_resources(fsm->datafs);
			sfifo_close(&fsm->datafs->fifo);
			os_free(fsm->datafs);
			fsm->passive = 0;
		}
		fsm->datapcb = NULL;
		fsm->datafs = NULL;
	}
	fsm->state = FTPD_IDLE;
	send_msg(pcb, fsm, msg226);
}

static void cmd_type(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	LOGI("Got TYPE -%s-\r\n", arg);
	send_msg(pcb, fsm, msg200);
}

static void cmd_mode(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	LOGI("Got MODE -%s-\r\n", arg);
	send_msg(pcb, fsm, msg502);
}

static void cmd_rnfr(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *path = NULL;

	if (arg == NULL) {
		send_msg(pcb, fsm, msg501);
		return;
	}
	if (*arg == '\0') {
		send_msg(pcb, fsm, msg501);
		return;
	}
	if (fsm->renamefrom) {
		os_free(fsm->renamefrom);
		fsm->renamefrom = NULL;
	}

	path = (char *)os_malloc(MAX_PATH_LEN);
	if (path == NULL) {
		send_msg(pcb, fsm, msg451);
		return;
	}
	os_memset(path, 0, MAX_PATH_LEN);
	if (ftp_make_path(arg, path, MAX_PATH_LEN) != 0) {
		os_free(path);
		send_msg(pcb, fsm, msg550);
		return;
	}

	fsm->renamefrom = path;
	LOGI("cmd_rnfr: arg=%s path=%s\r\n", arg, fsm->renamefrom);
	fsm->state = FTPD_RNFR;
	send_msg(pcb, fsm, msg350);
}

static void cmd_rnto(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *new_full_path = NULL;

	new_full_path = os_malloc(MAX_PATH_LEN);
	if (new_full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(new_full_path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (fsm->state != FTPD_RNFR) {
		send_msg(pcb, fsm, msg503);
		goto exit;
	}
	fsm->state = FTPD_IDLE;

	if (ftp_make_path(arg, new_full_path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	LOGI("cmd_rnto: old=%s new=%s\r\n", fsm->renamefrom, new_full_path);
	if (rename(fsm->renamefrom, new_full_path))
		send_msg(pcb, fsm, msg450);
	else
		send_msg(pcb, fsm, msg250);
	os_free(fsm->renamefrom);
	fsm->renamefrom = NULL;

exit:
	if (new_full_path)
		os_free(new_full_path);
}

static void cmd_mkd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	char *full_path = NULL;

	full_path = os_malloc(MAX_PATH_LEN);
	if (full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(full_path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (ftp_make_path(arg, full_path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	LOGI("cmd_mkd: arg=%s full_path=%s\r\n", arg, full_path);
	errno = 0;
	if (mkdir(full_path, 0777) != 0) {
		LOGE("cmd_mkd: mkdir failed path=%s errno=%d\r\n", full_path, errno);
		send_msg(pcb, fsm, msg550);
	} else {
		send_msg(pcb, fsm, msg257, arg);
	}

exit:
	if (full_path)
		os_free(full_path);
}

static void cmd_rmd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	struct stat st;
	char *full_path = NULL;

	full_path = os_malloc(MAX_PATH_LEN);
	if (full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(full_path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (ftp_make_path(arg, full_path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	LOGI("cmd_rmd: arg=%s full_path=%s\r\n", arg, full_path);
	errno = 0;
	if (stat(full_path, &st) != 0) {
		LOGE("cmd_rmd: stat failed path=%s errno=%d\r\n", full_path, errno);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (!S_ISDIR(st.st_mode)) {
		LOGE("cmd_rmd: not dir path=%s mode=0x%x\r\n", full_path, (unsigned int)st.st_mode);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	errno = 0;
	if (rmdir(full_path) != 0) {
		LOGE("cmd_rmd: rmdir failed path=%s errno=%d\r\n", full_path, errno);
		send_msg(pcb, fsm, msg550);
	} else {
		send_msg(pcb, fsm, msg250);
	}

exit:
	if (full_path)
		os_free(full_path);
}

static void cmd_dele(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	struct stat st;
	char *full_path = NULL;

	full_path = os_malloc(MAX_PATH_LEN);
	if (full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(full_path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (ftp_make_path(arg, full_path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	LOGI("cmd_dele: arg=%s full_path=%s\r\n", arg, full_path);
	errno = 0;
	if (stat(full_path, &st) != 0) {
		LOGE("cmd_dele: stat failed path=%s errno=%d\r\n", full_path, errno);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (!S_ISREG(st.st_mode)) {
		LOGE("cmd_dele: not regular path=%s mode=0x%x\r\n", full_path, (unsigned int)st.st_mode);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	errno = 0;
	if (unlink(full_path) != 0) {
		LOGE("cmd_dele: unlink failed path=%s errno=%d\r\n", full_path, errno);
		send_msg(pcb, fsm, msg550);
	} else {
		send_msg(pcb, fsm, msg250);
	}

exit:
	if (full_path)
		os_free(full_path);
}

static void cmd_size(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm) {
	struct stat st;
	char *path = NULL;

	char cwd[MAX_PATH_LEN] = {0};
	char *cwd_ret = getcwd(cwd, MAX_PATH_LEN);
	int stat_errno = 0;

	LOGI("cmd_size: arg=%s cwd=%s\r\n",
		arg ? arg : "(null)", cwd_ret ? cwd_ret : "(null)");

	path = os_malloc(MAX_PATH_LEN);
	if (path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (ftp_make_path(arg, path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	LOGI("cmd_size: arg=%s path=%s\r\n", arg, path);
	errno = 0;
	if (stat(path, &st) != 0) {
		stat_errno = errno;
		LOGI("cmd_size: stat failed arg=%s cwd=%s path=%s errno=%d\r\n",
			arg, cwd_ret ? cwd_ret : "(null)", path, stat_errno);

		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	if (!S_ISREG(st.st_mode)) {
		LOGI("cmd_size: not regular arg=%s mode=0x%x\r\n",
			arg, (unsigned int)st.st_mode);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	LOGI("cmd_size: success arg=%s size=%ld\r\n", arg, (long)st.st_size);
	send_msg(pcb, fsm, msg213, st.st_size);

exit:
	if (path)
		os_free(path);
}

static void cmd_reset(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	long offset;
	char *endptr = NULL;

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		return;
	}

	offset = strtol(arg, &endptr, 10);
	if (*endptr != '\0' || offset < 0) {
		send_msg(pcb, fsm, msg501);
		return;
	}

	fsm->restart_offset = (off_t)offset;
	send_msg(pcb, fsm, msg350);
}

static void cmd_clnt(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		return;
	}

	/* Log client name for debugging */
	LOGI("FTP client: %s\r\n", arg);

	send_msg(pcb, fsm, msg200);
}

static void cmd_feat(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	send_msg(pcb, fsm, msg_FEAT);
}

static void cmd_opts(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		return;
	}

	/* Parse option and value */
	if (strncmp(arg, "UTF8", 4) == 0) {
		/* UTF8 option - we support it */
		if (strlen(arg) > 5 && arg[4] == ' ') {
			/* OPTS UTF8 ON or OPTS UTF8 OFF */
			send_msg(pcb, fsm, msg200);
		} else {
			/* OPTS UTF8 */
			send_msg(pcb, fsm, msg200);
		}
	} else {
		/* Unknown option - return 501 */
		send_msg(pcb, fsm, msg501);
	}
}

static void cmd_appe(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	int fd = -1;
	char *path = NULL;

	path = os_malloc(MAX_PATH_LEN);
	if (path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(path, 0, MAX_PATH_LEN);

	if (arg == NULL || *arg == '\0') {
		send_msg(pcb, fsm, msg501);
		goto exit;
	}

	if (ftp_make_path(arg, path, MAX_PATH_LEN) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}
	LOGI("cmd_appe: arg=%s path=%s\r\n", arg, path);
	fd = open(path, O_RDWR | O_CREAT | O_APPEND);
	if (-1 == fd) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	send_msg(pcb, fsm, msg150stor, arg);

	if (open_dataconnection(pcb, fsm) != 0) {
		close(fd);
		goto exit;
	}

	fsm->datafs->fd = fd;
	fsm->state = FTPD_STOR;

exit:
	if (path)
		os_free(path);
}

static void cmd_mlsd(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm) {
	cmd_list_common(arg, pcb, fsm, FTPD_MLSD);
}

static void cmd_mlst(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm) {
	struct stat st = {0};
	struct tm *s_time = NULL;
	time_t current_time = {0};
	time_t modify_time = {0};
	int ret = -1;
	char *buffer = NULL;
	char *path_to_stat = NULL;
	char *cwd = NULL;
	char *cwd_buffer = NULL;
	char *full_path = NULL;

	buffer = os_malloc(FTP_TEMP_BUFFER_LEN);

	if (NULL == buffer) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(buffer, 0, FTP_TEMP_BUFFER_LEN);

#if CONFIG_NTP_SYNC_RTC
	extern time_t timestamp_get();
	current_time = timestamp_get();
#else
	/* Fallback to time() if NTP is not configured */
	current_time = time(NULL);
#endif

	full_path = os_malloc(FULL_PATH_SIZE);
	if (full_path == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		send_msg(pcb, fsm, msg451);
		goto exit;
	}
	os_memset(full_path, 0, FULL_PATH_SIZE);

	/* If no argument, use current directory */
	if (arg == NULL || *arg == '\0') {
		cwd_buffer = os_malloc(MAX_PATH_LEN);
		if (cwd_buffer == NULL) {
			LOGE("%s: memory malloc failed\r\n",__func__);
			send_msg(pcb, fsm, msg451);
			goto exit;
		}
		os_memset(cwd_buffer, 0, MAX_PATH_LEN);
		cwd = getcwd(cwd_buffer, MAX_PATH_LEN);
		if (cwd == NULL || ftp_normalize_mount_path(cwd, full_path, FULL_PATH_SIZE) != 0) {
			LOGE("%s: getcwd/normalize failed\r\n",__func__);
			send_msg(pcb, fsm, msg550);
			goto exit;
		}
	} else if (ftp_make_path(arg, full_path, FULL_PATH_SIZE) != 0) {
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	path_to_stat = full_path;
	LOGV("%s path_to_stat %s\r\n",__func__,path_to_stat);
	/* Get file/directory information */
	errno = 0;
	ret = stat(full_path, &st);
	if (ret != 0) {
		LOGE("%s: stat failed path=%s ret=%d errno=%d\r\n",
			__func__, full_path, ret, errno);
		send_msg(pcb, fsm, msg550);
		goto exit;
	}

	/* If st_mtime is 0 (file system doesn't support timestamps),
	 * use current time instead */
	modify_time = st.st_mtime == 0 ? current_time : st.st_mtime;
	modify_time = ftpd_time_to_utc(modify_time);
	s_time = gmtime(&modify_time);

	/* MLST returns one machine-readable facts line on the control connection. */
	if (S_ISDIR(st.st_mode)) {
		/* MLSD format: modify=YYYYMMDDHHmmss */
		sprintf(buffer, " type=%s;perm=%s;modify=%04d%02d%02d%02d%02d%02d; %s",
				"dir","elrwx",
				s_time->tm_year + 1900,
				s_time->tm_mon + 1,
				s_time->tm_mday,
				s_time->tm_hour,
				s_time->tm_min,
				s_time->tm_sec,
				path_to_stat);
	} else {
		/* MLSD format: modify=YYYYMMDDHHmmss */
		sprintf(buffer, " type=%s;perm=%s;modify=%04d%02d%02d%02d%02d%02d;size=%ld; %s",
				"file","rwx",
				s_time->tm_year + 1900,
				s_time->tm_mon + 1,
				s_time->tm_mday,
				s_time->tm_hour,
				s_time->tm_min,
				s_time->tm_sec,
				st.st_size,
				path_to_stat);
	}

	send_msg(pcb, fsm, "250-Listing %s", path_to_stat);
	send_msg(pcb, fsm, "%s", buffer);
	send_msg(pcb, fsm, "250 End");

exit:
	if (full_path)
		os_free(full_path);
	if (cwd_buffer)
		os_free(cwd_buffer);
	if (buffer)
		os_free(buffer);
}

struct ftpd_command {
	char *cmd;
	void (*func)(const char *arg, struct tcp_pcb *pcb, struct ftpd_msgstate *fsm);
};

static struct ftpd_command ftpd_commands[] = {
	{"USER", cmd_user},
	{"PASS", cmd_pass},
	{"PORT", cmd_port},
	{"QUIT", cmd_quit},
	{"CWD", cmd_cwd},
	{"CDUP", cmd_cdup},
	{"PWD", cmd_pwd},
	{"XPWD", cmd_pwd},
	{"NLST", cmd_nlst},
	{"LIST", cmd_list},
	{"RETR", cmd_retr},
	{"STOR", cmd_stor},
	{"NOOP", cmd_noop},
	{"SYST", cmd_syst},
	{"ABOR", cmd_abrt},
	{"TYPE", cmd_type},
	{"MODE", cmd_mode},
	{"RNFR", cmd_rnfr},
	{"RNTO", cmd_rnto},
	{"MKD", cmd_mkd},
	{"XMKD", cmd_mkd},
	{"RMD", cmd_rmd},
	{"XRMD", cmd_rmd},
	{"DELE", cmd_dele},
	{"PASV", cmd_pasv},
	{"CLNT", cmd_clnt},
	{"FEAT", cmd_feat},
	{"OPTS", cmd_opts},
	{"MLSD", cmd_mlsd},
	{"APPE", cmd_appe},
	{"REST", cmd_reset},
	{"SIZE", cmd_size},
	{"MLST", cmd_mlst},
	{NULL, NULL}
};

static void send_msgdata(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	err_t err;
	u16_t len;

	if (sfifo_used(&fsm->fifo) > 0) {
		int i;

		/* We cannot send more data than space available in the send
		   buffer. */
		if (tcp_sndbuf(pcb) < sfifo_used(&fsm->fifo))
			len = tcp_sndbuf(pcb);
		else
			len = (u16_t) sfifo_used(&fsm->fifo);

		i = fsm->fifo.readpos;
		if ((i + len) > fsm->fifo.size) {
			err = tcp_write(pcb, fsm->fifo.buffer + i, (u16_t)(fsm->fifo.size - i), 1);
			if (err != ERR_OK) {
				LOGW("send_msgdata: error writing!\r\n");
				return;
			}
			len -= fsm->fifo.size - i;
			fsm->fifo.readpos = 0;
			i = 0;
		}

		err = tcp_write(pcb, fsm->fifo.buffer + i, len, 1);
		if (err != ERR_OK) {
			LOGW("send_msgdata: error writing!\r\n");
			return;
		}
		fsm->fifo.readpos += len;
	}
}

static void send_msg(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm, char *msg, ...)
{
	va_list arg;
	char *buffer = NULL;
	int len;

	buffer = os_malloc(FTP_TEMP_BUFFER_LEN);
	if (buffer == NULL) {
		LOGE("%s: memory malloc failed\r\n",__func__);
		goto exit;
	}
	os_memset(buffer, 0, FTP_TEMP_BUFFER_LEN);

	va_start(arg, msg);
	vsprintf(buffer, msg, arg);
	va_end(arg);
	strcat(buffer, "\r\n");
	len = strlen(buffer);
	if (sfifo_space(&fsm->fifo) < len) {
		LOGE("%s: sfifo_space is not enough\r\n",__func__);
		goto exit;
	}
	sfifo_write(&fsm->fifo, buffer, len);
	LOGI("response: %s", buffer);
	send_msgdata(pcb, fsm);

exit:
	if (buffer)
		os_free(buffer);
}

static void ftpd_msgerr(void *arg, err_t err)
{
	struct ftpd_msgstate *fsm = arg;

	if (fsm == NULL)
		return;
	LOGE("ftpd_msgerr: %s (%i) fsm=0x%x state=%s datafs=0x%x datapcb=0x%x passive=%d renamefrom=0x%x fifo_used=%d fifo_space=%d\r\n",
		lwip_strerr(err), err, (unsigned int)fsm,
		ftpd_state_name(fsm->state), (unsigned int)fsm->datafs,
		(unsigned int)fsm->datapcb, fsm->passive,
		(unsigned int)fsm->renamefrom, sfifo_used(&fsm->fifo),
		sfifo_space(&fsm->fifo));
	if (fsm->datafs) {
		ftpd_dataclose(fsm->datapcb, fsm->datafs);
	}
	sfifo_close(&fsm->fifo);
	// vfs_close(fsm->vfs);
	// fsm->vfs = NULL;

	if (fsm->renamefrom)
		os_free(fsm->renamefrom);
	fsm->renamefrom = NULL;
	os_free(fsm);
}

static void ftpd_msgclose(struct tcp_pcb *pcb, struct ftpd_msgstate *fsm)
{
	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	if (fsm->datafs) {
		ftpd_dataclose(fsm->datapcb, fsm->datafs);
	}
	sfifo_close(&fsm->fifo);
	// vfs_close(fsm->vfs);
	// fsm->vfs = NULL;
	if (fsm->renamefrom)
		os_free(fsm->renamefrom);
	fsm->renamefrom = NULL;
	os_free(fsm);
	tcp_arg(pcb, NULL);
	tcp_close(pcb);
}

static err_t ftpd_msgsent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	struct ftpd_msgstate *fsm = arg;

	if (pcb->state > ESTABLISHED)
		return ERR_OK;

	if ((sfifo_used(&fsm->fifo) == 0) && (fsm->state == FTPD_QUIT)) {
		ftpd_msgclose(pcb, fsm);
		return ERR_OK;
	}

	send_msgdata(pcb, fsm);

	return ERR_OK;
}

static err_t ftpd_msgrecv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	char *text;
	struct ftpd_msgstate *fsm = arg;

	if (err == ERR_OK && p != NULL) {

		/* Inform TCP that we have taken the data. */
		tcp_recved(pcb, p->tot_len);

		text = (char *)ftp_malloc(p->tot_len + 1);
		if (text) {
			char cmd[5];
			struct pbuf *q;
			char *pt = text;
			struct ftpd_command *ftpd_cmd;

			for (q = p; q != NULL; q = q->next) {
				bcopy(q->payload, pt, q->len);
				pt += q->len;
			}
			*pt = '\0';

			pt = &text[strlen(text) - 1];
			while (((*pt == '\r') || (*pt == '\n')) && pt >= text)
				*pt-- = '\0';

			LOGI("query: %s\r\n", text);

			strncpy(cmd, text, 4);
			for (pt = cmd; isalpha((int)*pt) && pt < &cmd[4]; pt++)
				*pt = toupper(*pt);
			*pt = '\0';

			for (ftpd_cmd = ftpd_commands; ftpd_cmd->cmd != NULL; ftpd_cmd++) {
				if (!strcmp(ftpd_cmd->cmd, cmd))
					break;
			}

			if (strlen(text) < (strlen(cmd) + 1))
				pt = "";
			else
				pt = &text[strlen(cmd) + 1];

			if (ftpd_cmd->func)
				ftpd_cmd->func(pt, pcb, fsm);
			else
				send_msg(pcb, fsm, msg502);

			os_free(text);
		}
		pbuf_free(p);
	}
	else if ((err == ERR_OK && p == NULL)) {
		LOGV("ftpd_msgrecv: control connection closed by peer fsm=0x%x state=%s datafs=0x%x datapcb=0x%x\r\n",
			(unsigned int)fsm, fsm ? ftpd_state_name(fsm->state) : "(null)",
			fsm ? (unsigned int)fsm->datafs : 0,
			fsm ? (unsigned int)fsm->datapcb : 0);
		ftpd_msgclose(pcb, fsm);
	}

	return ERR_OK;
}

static err_t ftpd_msgpoll(void *arg, struct tcp_pcb *pcb)
{
	struct ftpd_msgstate *fsm = arg;

	if (fsm == NULL)
		return ERR_OK;

	if (fsm->datafs) {
		if (fsm->datafs->connected) {
			switch (fsm->state) {
			case FTPD_LIST:
			case FTPD_NLST:
			case FTPD_MLSD:
				send_next_directory(fsm->datafs, fsm->datapcb, fsm->state);
				break;
			case FTPD_RETR:
				send_file(fsm->datafs, fsm->datapcb);
				break;
			default:
				break;
			}
		}
	}

	return ERR_OK;
}

static err_t ftpd_msgaccept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	struct ftpd_msgstate *fsm;

	fsm = (struct ftpd_msgstate *)ftp_malloc(sizeof(struct ftpd_msgstate));
	if (fsm == NULL) {
		LOGE("%s: malloc memory failed\r\n",__func__);
		return ERR_MEM;
	}
	memset(fsm, 0, sizeof(struct ftpd_msgstate));

	sfifo_init(&fsm->fifo, MAX_PRE_READ_BUFFER_SIZE);
	fsm->state = FTPD_IDLE;

	tcp_arg(pcb, fsm);
	tcp_recv(pcb, ftpd_msgrecv);
	tcp_sent(pcb, ftpd_msgsent);
	tcp_err(pcb, ftpd_msgerr);
	tcp_poll(pcb, ftpd_msgpoll, 1);
	send_msg(pcb, fsm, msg220);

	return ERR_OK;
}

beken_thread_t ftpd_server_task = NULL;

static void ftpd_server_cc_main(beken_thread_arg_t data)
{
	err_t err = ERR_OK;
	struct tcp_pcb *pcb;

	pcb = tcp_new();
    ip_set_option(pcb, SOF_REUSEADDR);
    err = tcp_bind(pcb, IP_ADDR_ANY, 21);
    if (err != ERR_OK) {
        LOGE("ftp bind failed, ret: %d\r\n", err);
    }
	pcb = tcp_listen(pcb);
	tcp_accept(pcb, ftpd_msgaccept);
	ftp_is_running = 1;

	while(ftp_is_running) {
		rtos_delay_milliseconds(100);
	}

	ftp_is_running = 0;
	tcp_close(pcb);
	ftpd_server_task = NULL;
	rtos_delete_thread(NULL);
}

#if (CONFIG_FATFS)
static int _fs_mount(void)
{
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;
	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
#if (CONFIG_SDCARD)
	partition.part_dev.device_name = FATFS_DEV_SDCARD;
#else
	partition.part_dev.device_name = FATFS_DEV_FLASH;
#endif

	partition.mount_path = FTP_MOUNT_PATH;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}
#endif


bk_err_t ftpd_server_init(void)
{
	int ret;

#if (CONFIG_FATFS)
	ret = _fs_mount();
	if (BK_OK != ret)
	{
		bk_printf("[%s][%d] mount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
		return ret;
	}
#endif

	ret = rtos_create_thread(&ftpd_server_task,
							 4,
							 "ftpd_server",
							 (beken_thread_function_t)ftpd_server_cc_main,
							 1024 * 4,
							 (beken_thread_arg_t)NULL);
	if (ret != kNoErr)
	{
		bk_printf("Error: Failed to create ftpd server: %d\n", ret);
		return kGeneralErr;
	}

	return kNoErr;
}

void ftpd_server_deinit(void)
{
	bk_err_t ret = BK_FAIL;
	LOGI("[%s]\r\n", __FUNCTION__);

	ftp_is_running = 0;

#if (CONFIG_FATFS)
	ret = umount(FTP_MOUNT_PATH);
	if (BK_OK != ret) {
		LOGE("[%s][%d] unmount fail: %d\r\n", __FUNCTION__, __LINE__, ret);
	}
#endif
	LOGI("[%s][%d] unmount success\r\n", __FUNCTION__, __LINE__);
}

#if CONFIG_CLI
void cli_wifi_ftp_server_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	char *msg = NULL;

	if (argc < 2) {
		bk_printf("Invalid ftp server paramter\r\n");
		goto error;
	}

	if(os_strcmp(argv[1], "server") == 0) {
		#if CONFIG_FTP_SERVER
		#if CONFIG_VFS
		ftpd_server_init();
		#endif
		#endif
	}
	else {
		bk_printf("Invalid ftp server paramter\r\n");
		goto error;
	}

	if (!ret) {
		msg = WIFI_CMD_RSP_SUCCEED;
		os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
		return;
	}
error:
	msg = WIFI_CMD_RSP_ERROR;
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
	return;
}

static const struct cli_command s_ftpd_commands[] = {
    {"ftp", "ftp server", cli_wifi_ftp_server_cmd},
};

int ftpd_cli_init(void)
{
    return cli_register_commands(s_ftpd_commands, FTPD_CMD_CNT);
}
#endif
#endif
