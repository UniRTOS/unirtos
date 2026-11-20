/*************************************************************
 * Author:	Lionfore Hao (haolianfu@agora.io)
 * Date	 :	Jul 29th, 2020
 * Module:	OS errno implementation file
 *
 *
 * This is a part of the Advanced High Performance Library.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#include <api/aosl_types.h>
#include <api/aosl_defs.h>
#include <api/aosl_errno.h>
#include <api/aosl_mm.h>
#include <kernel/thread.h>
#include <stdlib.h>

static k_tls_key_t errno_key = -1;

void k_errno_init (void)
{
	if (k_tls_key_create (&errno_key) < 0)
		abort ();
}

void k_errno_fini (void)
{
	k_tls_key_delete (errno_key);
	errno_key = -1;
}

__export_in_so__ int *aosl_errno_ptr (void)
{
	void **errno_ptr;

	if (errno_key < 0)
		abort ();

	errno_ptr = k_tls_key_get_ref (errno_key);
	if (errno_ptr == NULL) {
		if (k_tls_key_set (errno_key, NULL) < 0)
			abort ();

		errno_ptr = k_tls_key_get_ref (errno_key);
		if (NULL == errno_ptr) {
			abort ();
		}
		*errno_ptr = 0;
	}

	return (int *)errno_ptr;
}

__export_in_so__ char *aosl_strerror (int errnum)
{
	switch (errnum) {
		/* 错误码定义 */
		case AOSL_EPERM: return "EPERM";
		case AOSL_ENOENT: return "ENOENT";
		case AOSL_ESRCH: return "ESRCH";
		case AOSL_EINTR: return "EINTR";
		case AOSL_EIO: return "EIO";
		case AOSL_ENXIO: return "ENXIO";
		case AOSL_E2BIG: return "E2BIG";
		case AOSL_ENOEXEC: return "ENOEXEC";
		case AOSL_EBADF: return "EBADF";
		case AOSL_ECHILD: return "ECHILD";
		case AOSL_EAGAIN: return "EAGAIN";
		case AOSL_ENOMEM: return "ENOMEM";
		case AOSL_EACCES: return "EACCES";
		case AOSL_EFAULT: return "EFAULT";
		case AOSL_ENOTBLK: return "ENOTBLK";
		case AOSL_EBUSY: return "EBUSY";
		case AOSL_EEXIST: return "EEXIST";
		case AOSL_EXDEV: return "EXDEV";
		case AOSL_ENODEV: return "ENODEV";
		case AOSL_ENOTDIR: return "ENOTDIR";
		case AOSL_EISDIR: return "EISDIR";
		case AOSL_EINVAL: return "EINVAL";
		case AOSL_ENFILE: return "ENFILE";
		case AOSL_EMFILE: return "EMFILE";
		case AOSL_ENOTTY: return "ENOTTY";
		case AOSL_ETXTBSY: return "ETXTBSY";
		case AOSL_EFBIG: return "EFBIG";
		case AOSL_ENOSPC: return "ENOSPC";
		case AOSL_ESPIPE: return "ESPIPE";
		case AOSL_EROFS: return "EROFS";
		case AOSL_EMLINK: return "EMLINK";
		case AOSL_EPIPE: return "EPIPE";
		case AOSL_EDOM: return "EDOM";
		case AOSL_ERANGE: return "ERANGE";

		/* 更多错误码... */
		case AOSL_EDEADLK: return "EDEADLK";
		case AOSL_ENAMETOOLONG: return "ENAMETOOLONG";
		case AOSL_ENOLCK: return "ENOLCK";
		case AOSL_ENOSYS: return "ENOSYS";
		case AOSL_ENOTEMPTY: return "ENOTEMPTY";
		case AOSL_ELOOP: return "ELOOP";

		/* 网络相关错误 */
		//case AOSL_EWOULDBLOCK: return "EWOULDBLOCK";
		case AOSL_ENOMSG: return "ENOMSG";
		case AOSL_EIDRM: return "EIDRM";
		case AOSL_ECHRNG: return "ECHRNG";
		case AOSL_EL2NSYNC: return "EL2NSYNC";
		case AOSL_EL3HLT: return "EL3HLT";
		case AOSL_EL3RST: return "EL3RST";
		case AOSL_ELNRNG: return "ELNRNG";
		case AOSL_EUNATCH: return "EUNATCH";

/* 套接字错误 */
		case AOSL_ENONET: return "ENONET";
		case AOSL_ENOPKG: return "ENOPKG";
		case AOSL_EREMOTE: return "EREMOTE";
		case AOSL_ENOLINK: return "ENOLINK";
		case AOSL_EADV: return "EADV";
		case AOSL_ESRMNT: return "ESRMNT";
		case AOSL_ECOMM: return "ECOMM";
		case AOSL_EPROTO: return "EPROTO";
		case AOSL_EMULTIHOP: return "EMULTIHOP";
		case AOSL_EDOTDOT: return "EDOTDOT";
		case AOSL_EBADMSG: return "EBADMSG";
		case AOSL_EOVERFLOW: return "EOVERFLOW";
		case AOSL_ENOTUNIQ: return "ENOTUNIQ";
		case AOSL_EBADFD: return "EBADFD";
		case AOSL_EREMCHG: return "EREMCHG";
		case AOSL_ELIBACC: return "ELIBACC";
		case AOSL_ELIBBAD: return "ELIBBAD";
		case AOSL_ELIBSCN: return "ELIBSCN";
		case AOSL_ELIBMAX: return "ELIBMAX";
		case AOSL_ELIBEXEC: return "ELIBEXEC";
		case AOSL_EILSEQ: return "EILSEQ";
		case AOSL_ERESTART: return "ERESTART";
		case AOSL_ESTRPIPE: return "ESTRPIPE";
		case AOSL_EUSERS: return "EUSERS";
		case AOSL_ENOTSOCK: return "ENOTSOCK";
		case AOSL_EDESTADDRREQ: return "EDESTADDRREQ";
		case AOSL_EMSGSIZE: return "EMSGSIZE";
		case AOSL_EPROTOTYPE: return "EPROTOTYPE";
		case AOSL_ENOPROTOOPT: return "ENOPROTOOPT";
		case AOSL_EPROTONOSUPPORT: return "EPROTONOSUPPORT";
		case AOSL_ESOCKTNOSUPPORT: return "ESOCKTNOSUPPORT";
		case AOSL_EOPNOTSUPP: return "EOPNOTSUPP";
		case AOSL_EPFNOSUPPORT: return "EPFNOSUPPORT";
		case AOSL_EAFNOSUPPORT: return "EAFNOSUPPORT";
		case AOSL_EADDRINUSE: return "EADDRINUSE";
		case AOSL_EADDRNOTAVAIL: return "EADDRNOTAVAIL";
		case AOSL_ENETDOWN: return "ENETDOWN";
		case AOSL_ENETUNREACH: return "ENETUNREACH";
		case AOSL_ENETRESET: return "ENETRESET";
		case AOSL_ECONNABORTED: return "ECONNABORTED";
		case AOSL_ECONNRESET: return "ECONNRESET";
		case AOSL_ENOBUFS: return "ENOBUFS";
		case AOSL_EISCONN: return "EISCONN";
		case AOSL_ENOTCONN: return "ENOTCONN";
		case AOSL_ESHUTDOWN: return "ESHUTDOWN";
		case AOSL_ETOOMANYREFS: return "ETOOMANYREFS";
		case AOSL_ETIMEDOUT: return "ETIMEDOUT";
		case AOSL_ECONNREFUSED: return "ECONNREFUSED";
		case AOSL_EHOSTDOWN: return "EHOSTDOWN";
		case AOSL_EHOSTUNREACH: return "EHOSTUNREACH";
		case AOSL_EALREADY: return "EALREADY";
		case AOSL_EINPROGRESS: return "EINPROGRESS";
		case AOSL_ESTALE: return "ESTALE";
		case AOSL_EUCLEAN: return "EUCLEAN";
		case AOSL_ENOTNAM: return "ENOTNAM";
		case AOSL_ENAVAIL: return "ENAVAIL";
		case AOSL_EISNAM: return "EISNAM";
		case AOSL_EREMOTEIO: return "EREMOTEIO";
	default:
		break;
	}

	return "Unknown Error";
}