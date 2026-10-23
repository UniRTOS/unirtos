/** @file         qurl_url.c
 *  @brief        Brief description: Provides URL-related operations
 *  @details      Detailed description: RFC reference: https://tools.ietf.org/html/rfc3986
 *  @author       lzm
 *  @date         2023-09-06 17:54:16
 *  @version      v1.0
 *  @copyright    Copyright By lizhuming, All Rights Reserved
 *
 **********************************************************
 *  @LOG Change Log: -----
 **********************************************************
 */

#include "qurl_config.h"
#include "qurl_port.h"
#include "qurl_prot.h"
#include "qurl_url.h"
#include "qurl_default_basic.h"

#define URL_HOSTNAME_END(x) (((x) == '/') || ((x) == '?') || ((x) == '#'))

qurl_bit_t qurl_url_is_absolute(const char *url_ptr, char *buf_ptr, qosa_uint16_t buf_len);

static void *url_malloc(qosa_size_t size)
{
    void *ptr = QOSA_NULL;

    ptr = qurl_malloc(size + 1);
    if (ptr != QOSA_NULL)
    {
        qurl_memset(ptr, 0x00, size + 1);
    }

    return ptr;
}

#define url_free(p)                                                                                                                                            \
    do {                                                                                                                                                       \
        if (p != QOSA_NULL)                                                                                                                                    \
        {                                                                                                                                                      \
            qurl_free(p);                                                                                                                                      \
            p = QOSA_NULL;                                                                                                                                     \
        }                                                                                                                                                      \
    } while (0);

void qurl_url_url_free(qurl_url_t *uurl_ptr)
{
    if (uurl_ptr == QOSA_NULL)
    {
        return;
    }

    url_free(uurl_ptr->scheme);
    url_free(uurl_ptr->user);
    url_free(uurl_ptr->password);
    url_free(uurl_ptr->options);
    url_free(uurl_ptr->host);
    url_free(uurl_ptr->zoneid);
    url_free(uurl_ptr->port);
    url_free(uurl_ptr->path);
    url_free(uurl_ptr->query);
    url_free(uurl_ptr->fragment);
    url_free(uurl_ptr->scratch);
    url_free(uurl_ptr->temppath);
}

/**
 * @brief Determine whether the given character is a non-reserved character. Non-reserved characters do not require URL encoding parsing.
 * See https://tools.ietf.org/html/rfc3986#section-2.3
 */
static qurl_bit_t url_is_unreserved(unsigned char in)
{
    if (in >= '0' && in <= '9')
    {
        return QOSA_TRUE;
    }
    if (in >= 'a' && in <= 'z')
    {
        return QOSA_TRUE;
    }
    if (in >= 'A' && in <= 'Z')
    {
        return QOSA_TRUE;
    }
    if (in == '-' || in == '.' || in == '_' || in == '~')
    {
        return QOSA_TRUE;
    }
    return QOSA_FALSE;
}

/* scan for byte values < 31 or 127 */
static qurl_ecode_t url_junk_scan(const char *part)
{
    qosa_uint16_t n = 0;
    qosa_uint16_t nfine = 0;

    if (part)
    {
        static const char badbytes[] = {
            /* */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
            0x12,       0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x7f, 0x00 /* null-terminate */
        };
        n = qurl_strlen(part);
        nfine = qurl_strcspn(part, badbytes);
        if (nfine != n)
        {
            /* since we don't know which part is scanned, return a generic error code */
            return QURL_ECODE_URL_MALFORMED_INPUT;
        }
    }
    return QURL_OK;
}

/**
 * @brief Find the delimiter at the end of the hostname. For example, find the '?' symbol in cases like http://www.url.com?id=2380.
 */
static const char *url_find_host_sep(const char *url)
{
    const char *sep;
    const char *query;

    /* Find the start of the hostname */
    sep = qurl_strstr(url, "//");
    if (!sep)
        sep = url;
    else
        sep += 2;

    query = qurl_strchr(sep, '?');
    sep = qurl_strchr(sep, '/');

    if (!sep)
        sep = url + qurl_strlen(url);

    if (!query)
        query = url + qurl_strlen(url);

    return sep < query ? sep : query;
}

/**
 * @brief Determine whether characters in a URL need to be escaped in an encoding-independent manner.
 * The same criteria must be used in the url_strlen() and url_strcpy() functions.
 */
static qurl_bit_t url_char_needs_escaping(int c)
{
    return !(qurl_ctype_iscntrl(c) || qurl_ctype_isspace(c) || qurl_ctype_isgraph(c));
}

/**
 * @brief url_strlen() calculates the actual character length after URL encoding.
 * For hostnames, URL encoding is skipped; otherwise, IDN resolution will fail.
 */
static qosa_size_t url_strlen(char *url, qurl_bit_t relative)
{
    const unsigned char *ptr = QOSA_NULL;
    const unsigned char *host_sep = (const unsigned char *)url;
    qosa_size_t          newlen = 0;
    qurl_bit_t           left = QOSA_TRUE; /*Left side of '?'*/

    if (!relative)
    {
        host_sep = (const unsigned char *)url_find_host_sep(url);
    }

    for (ptr = (unsigned char *)url; *ptr; ptr++)
    {
        if (ptr < host_sep)
        {
            ++newlen;
            continue;
        }

        switch (*ptr)
        {
            case '?':
                left = QOSA_FALSE;
                /* FALLTHROUGH */
            default:
                if (url_char_needs_escaping(*ptr))
                {
                    newlen += 2;
                }
                newlen++;
                break;
            case ' ':
                if (left)
                {
                    newlen += 3;
                }
                else
                {
                    newlen++;
                }
                break;
        }
    }
    return newlen;
}

/**
 * @brief The url_strcpy() function copies a URL to the output buffer and URL-encodes spaces in the source URL accordingly.
 * For hostnames, URL encoding is skipped; otherwise, IDN resolution will fail.
 */
static void url_strcpy(char *output, const char *url, qurl_bit_t relative)
{
    /* we must add this with whitespace-replacing */
    qurl_bit_t           left = QOSA_TRUE;
    const unsigned char *iptr = QOSA_NULL;
    char                *optr = output;
    const unsigned char *host_sep = (const unsigned char *)url;

    if (!relative)
    {
        host_sep = (const unsigned char *)url_find_host_sep(url);
    }

    for (iptr = (unsigned char *)url; /* read from here */
         *iptr;                       /* until zero byte */
         iptr++)
    {
        if (iptr < host_sep)
        {
            *optr++ = *iptr;
            continue;
        }

        switch (*iptr)
        {
            case '?':
                left = QOSA_FALSE;
                /* FALLTHROUGH */
            default:
                if (url_char_needs_escaping(*iptr))
                {
                    qurl_snprintf(optr, 4, "%%%02x", *iptr); /*[note]curl uses msnprintf*/
                    optr += 3;
                }
                else
                    *optr++ = *iptr;
                break;
            case ' ':
                if (left)
                {
                    *optr++ = '%'; /* add a '%' */
                    *optr++ = '2'; /* add a '2' */
                    *optr++ = '0'; /* add a '0' */
                }
                else
                {
                    *optr++ = '+'; /* add a '+' here */
                }
                break;
        }
    }
    *optr = 0; /* null-terminate output buffer */
}

/**
 * @brief The function receives a null-terminated path containing '.' and '..' sequences and removes them according to the rules in RFC 3986 Section 5.2.4.
 * The '?' indicates that the query parameter section is not part of the path; '.' and '..' do not need to be processed in this part.
 */
char *qurl_url_dedotdotify(const char *input)
{
    qosa_size_t inlen = qurl_strlen(input);
    char       *clone = QOSA_NULL;
    qosa_size_t clen = inlen; /* the length of the cloned input */
    char       *out = url_malloc(inlen + 1);
    char       *outptr = QOSA_NULL;
    char       *orgclone = QOSA_NULL;
    char       *queryp = QOSA_NULL;

    QOSA_UNUSED(clen);

    if (!out)
    {
        return QOSA_NULL; /* out of memory */
    }

    *out = 0; /* null-terminates, for inputs like "./" */

    /*copy the input string for further processing*/
    clone = qurl_strdup(input);
    if (!clone)
    {
        url_free(out);
        return QOSA_NULL;
    }
    orgclone = clone;
    outptr = out;

    if (!*clone)
    {
        /* zero length string, return that */
        url_free(out);
        return clone;
    }

    /*Truncate the '?' part, do not process it, and add it back later after processing.*/
    queryp = qurl_strchr(clone, '?');
    if (queryp)
    {
        *queryp = 0;
    }

    do {
        /*1. If it starts with the prefix "../" or "./", remove that prefix from the input buffer.*/
        if (!qurl_strncmp("./", clone, 2))
        {
            clone += 2;
            clen -= 2;
        }
        else if (!qurl_strncmp("../", clone, 3))
        {
            clone += 3;
            clen -= 3;
        }

        /*2. If it starts with the prefix "/./" or "/." (where "." is a complete path segment), replace that prefix with "/" in the input buffer*/
        else if (!qurl_strncmp("/./", clone, 3))
        {
            clone += 2;
            clen -= 2;
        }
        else if (!qurl_strcmp("/.", clone))
        {
            clone[1] = '/';
            clone++;
            clen -= 1;
        }

        /*3. If it starts with the prefix "/../" or "/.." (where ".." is a complete path segment), replace this prefix in the input buffer with "/", and remove the last path segment and the preceding "/" (if any) from the output buffer.*/
        else if (!qurl_strncmp("/../", clone, 4))
        {
            clone += 3;
            clen -= 3;
            /*Remove the last path segment from the output buffer*/
            while (outptr > out)
            {
                outptr--;
                if (*outptr == '/')
                    break;
            }
            *outptr = 0; /*Add a null terminator at the stop position*/
        }
        else if (!qurl_strcmp("/..", clone))
        {
            clone[2] = '/';
            clone += 2;
            clen -= 2;
            /*Remove the last path segment from the output buffer*/
            while (outptr > out)
            {
                outptr--;
                if (*outptr == '/')
                    break;
            }
            *outptr = 0; /*Add a null terminator at the stop position*/
        }

        /*4. If the input buffer consists solely of "." or "..", remove it from the input buffer.*/

        else if (!qurl_strcmp(".", clone) || !qurl_strcmp("..", clone))
        {
            *clone = 0;
            *out = 0;
        }

        else
        {
            /*5. Move the first path segment from the input buffer (including the initial '/' character (if present) and subsequent characters, up to the next '/' character or the end of the input buffer) to the end of the output buffer.*/
            do {
                *outptr++ = *clone++;
                clen--;
            } while (*clone && (*clone != '/'));
            *outptr = 0;
        }

    } while (*clone);

    if (queryp)
    {
        qosa_size_t qlen;
        /*Restore the '?' query parameter section*/
        qosa_size_t oindex = queryp - orgclone;
        qlen = qurl_strlen(&input[oindex]);
        qurl_memcpy(outptr, &input[oindex], qlen + 1); /* include the end zero byte */
    }

    url_free(orgclone);
    return out;
}

/**
 * @brief url_parse_login_details()
 *
 * Function for parsing login strings containing username, password, and options, supporting the following formats:
 *   user
 *   user:password
 *   user:password;options
 *   user;options
 *   user;options:password
 *   :password
 *   :password;options
 *   ;options
 *   ;options:password
 *
 * Parameters:
 *
 * @param[in] const char *login     - Login string.
 * @param[in] qosa_size_t len             - Length of the login string.
 * @param[in,out] char **userp       - The address of a pointer to store the user name. After parsing is complete, it will store the user name in newly allocated memory.
 * @param[in,out] char **passwdp     - Address of the pointer to store the password; after parsing, it will hold the password in newly allocated memory.
 * @param[in,out] char **optionsp    - Address of the pointer to store options; after parsing, it will store the options in newly allocated memory.
 *
 * @return qurl_ecode_t
 */
qurl_ecode_t url_parse_login_details(const char *login, const qosa_size_t len, char **userp, char **passwdp, char **optionsp)
{
    qurl_ecode_t ret = QURL_OK;
    char        *ubuf = QOSA_NULL;
    char        *pbuf = QOSA_NULL;
    char        *obuf = QOSA_NULL;
    const char  *psep = QOSA_NULL;
    const char  *osep = QOSA_NULL;
    qosa_size_t  ulen = 0;
    qosa_size_t  plen = 0;
    qosa_size_t  olen = 0;

    /*The length check is because this function can be called directly from setopt without going through the usual string length verification.*/
    qosa_size_t llen = qurl_strlen(login);
    if (llen > QURL_CFG_URL_LEN_MAX)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    /*Attempt to find the password delimiter*/
    if (passwdp)
    {
        psep = qurl_strchr(login, ':');

        /*Under the constraints of the login string*/
        if (psep >= login + len)
            psep = QOSA_NULL;
    }

    /*Attempt to find the option delimiter*/
    if (optionsp)
    {
        osep = qurl_strchr(login, ';');

        /*Under the constraints of the login string*/
        if (osep >= login + len)
            osep = QOSA_NULL;
    }

    /*Calculate the length of each part*/
    ulen = (psep ? (qosa_size_t)(osep && psep > osep ? osep - login : psep - login) : (osep ? (qosa_size_t)(osep - login) : len));
    plen = (psep ? (osep && osep > psep ? (qosa_size_t)(osep - psep) : (qosa_size_t)(login + len - psep)) - 1 : 0);
    olen = (osep ? (psep && psep > osep ? (qosa_size_t)(psep - osep) : (qosa_size_t)(login + len - osep)) - 1 : 0);

    /*Allocate user portion of the buffer*/
    if (userp && ulen)
    {
        ubuf = url_malloc(ulen + 1);
        if (!ubuf)
        {
            ret = QURL_ECODE_NO_MEMORY;
        }
    }

    /*Allocate buffer for the password section*/
    if (ret == QURL_OK && passwdp && plen)
    {
        pbuf = url_malloc(plen + 1);
        if (!pbuf)
        {
            url_free(ubuf);
            ret = QURL_ECODE_NO_MEMORY;
        }
    }

    /*Allocate buffer for the options section*/
    if (ret == QURL_OK && optionsp && olen)
    {
        obuf = url_malloc(olen + 1);
        if (!obuf)
        {
            url_free(pbuf);
            url_free(ubuf);
            ret = QURL_ECODE_NO_MEMORY;
        }
    }

    if (ret == QURL_OK)
    {
        /*Store necessary user sections*/
        if (ubuf)
        {
            qurl_memcpy(ubuf, login, ulen);
            ubuf[ulen] = '\0';
            url_free(*userp);
            *userp = ubuf;
        }

        /*Store the necessary password part*/
        if (pbuf)
        {
            qurl_memcpy(pbuf, psep + 1, plen);
            pbuf[plen] = '\0';
            url_free(*passwdp);
            *passwdp = pbuf;
        }

        /*Store the necessary options section*/
        if (obuf)
        {
            qurl_memcpy(obuf, osep + 1, olen);
            obuf[olen] = '\0';
            url_free(*optionsp);
            *optionsp = obuf;
        }
    }

    return ret;
}

/**
 * @brief Extract the login username, password, and options from the URL, and strip them from the hostname.
 */
static qurl_ecode_t url_parse_hostname_login(qurl_url_t *uurl_ptr, char **hostname, unsigned int flags)
{
    qurl_ecode_t            ret = QURL_OK;
    char                   *userp = QOSA_NULL;
    char                   *passwdp = QOSA_NULL;
    char                   *optionsp = QOSA_NULL;
    const qurl_port_prot_t *port_prot_ptr = QOSA_NULL;

    /*At this point, we hope all other special cases have been handled,
	 * After this, conn->host.name contains at most the following format:
	 *    [user[:password][;options]]@]hostname
	 *
	 * We need to find a suitable location for the embedded login information before any other operations.
	 */

    char *ptr = qurl_strchr(*hostname, '@');
    char *login = *hostname;

    if (!ptr)
    {
        goto exit;
    }

    /*Now we will attempt to extract possible login information, such as: ftp://user:password@ftp.my.site:8021/README*/
    *hostname = ++ptr;

    /* if this is a known scheme, get some details */
    if (uurl_ptr->scheme)
    {
        ret = qurl_port_prot_match_by_scheme(uurl_ptr->scheme, &port_prot_ptr);
        if (ret != QURL_OK)
        {
            goto exit;
        }
    }

    /* We could use the login information in the URL so extract it. Only parse
	   options if the handler says we should. Note that 'h' might be NULL! */
    /*Use the login information from the URL and extract it. Parse options only when the handler specifies that option parsing is required. Note that port_protocol_ptr may be NULL!*/
    ret = url_parse_login_details(
        login,
        ptr - login - 1,
        &userp,
        &passwdp,
        (port_prot_ptr && (port_prot_ptr->flags & QURL_PROT_OPT_URLOPTIONS)) ? &optionsp : QOSA_NULL
    );
    if (ret != QURL_OK)
    {
        goto exit;
    }

    if (userp)
    {
        if (flags & QURL_URL_DISALLOW_USER)
        {
            /*Users are not allowed to include usernames and passwords.*/
            ret = QURL_ECODE_URL_USER_NOT_ALLOWED;
            goto exit;
        }

        uurl_ptr->user = userp;
    }

    if (passwdp)
    {
        uurl_ptr->password = passwdp;
    }

    if (optionsp)
    {
        uurl_ptr->options = optionsp;
    }

    return QURL_OK;
exit:

    url_free(userp);
    url_free(passwdp);
    url_free(optionsp);

    return ret;
}

/**
 * @brief Parse the port from the hostname into the QUrl.
 */
qurl_ecode_t qurl_url_parse_port(qurl_url_t *uurl_ptr, char *hostname)
{
    char *portptr = QOSA_NULL;
    char  endbracket = 0;
    int   len = 0;

    /*
	 * Find the end position of an IPv6 address. The end position can be a closing bracket ']' or a percent-encoded zone index.
	 */
    if (1 == qurl_sscanf(hostname, "[%*45[0123456789abcdefABCDEF:.]%c%n", &endbracket, &len))
    {
        if (']' == endbracket)
            portptr = &hostname[len];
        else if ('%' == endbracket)
        {
            int zonelen = len;
            if (1 == qurl_sscanf(hostname + zonelen, "%*[^]]%c%n", &endbracket, &len))
            {
                if (']' != endbracket)
                {
                    return QURL_ECODE_URL_MALFORMED_INPUT;
                }
                portptr = &hostname[--zonelen + len + 1];
            }
            else
            {
                return QURL_ECODE_URL_MALFORMED_INPUT;
            }
        }
        else
        {
            return QURL_ECODE_URL_MALFORMED_INPUT;
        }

        /* this is a RFC2732-style specified IP-address */
        if (portptr && *portptr)
        {
            if (*portptr != ':')
            {
                return QURL_ECODE_URL_MALFORMED_INPUT;
            }
        }
        else
        {
            portptr = QOSA_NULL;
        }
    }
    else
    {
        portptr = qurl_strchr(hostname, ':');
    }

    if (portptr)
    {
        char *rest = QOSA_NULL;
        long  port = 0;
        char  portbuf[7] = {0};

        /*Browser behavior adjustment mechanism. If a colon appears in the URL but is not followed by a number, the URL will be truncated, and the part before the colon will be used as the hostname, with the default port used by default.
		 * This is to accommodate the lenient parsing of URL syntax by certain browsers. Well-known browsers such as Firefox, Chrome, and Safari all adopt this behavior.*/
        if (!portptr[1])
        {
            *portptr = '\0';
            return QURL_OK;
        }

        if (!qurl_ctype_isdigit(portptr[1]))
        {
            return QURL_ECODE_URL_BAD_PORT_NUMBER;
        }

        port = qurl_strtol(portptr + 1, &rest, 10); /*Restriction rule: the port number must be in decimal format.*/

        if ((port <= 0) || (port > 0xffff))
        {
            /*Regarding the restrictions on port number length and value: According to the Single UNIX Specification, the port number is 16 bits in length, meaning its value range is from 0 to 65535. However, port number zero (0) is not considered an acceptable value.*/
            return QURL_ECODE_URL_BAD_PORT_NUMBER;
        }

        if (rest[0])
        {
            return QURL_ECODE_URL_BAD_PORT_NUMBER;
        }

        *portptr++ = '\0'; /* cut off the name there */
        *rest = 0;
        /* generate a new port number string to get rid of leading zeroes etc */
        qurl_snprintf(portbuf, sizeof(portbuf), "%ld", port);
        uurl_ptr->portnum = port;
        uurl_ptr->port = qurl_strdup(portbuf);
        if (!uurl_ptr->port)
        {
            return QURL_ECODE_NO_MEMORY;
        }
    }

    return QURL_OK;
}

/**
 * @brief Check the validity of the hostname
 */
static qurl_ecode_t url_hostname_check(qurl_url_t *uurl_ptr, char *hostname)
{
    qosa_size_t len = 0;
    qosa_size_t hlen = qurl_strlen(hostname);

    if (hostname[0] == '[')
    {
#if QURL_CFG_ENABLE_IPV6
        char dest[16] = {0}; /*Represents a binary IPv6 address*/
#endif
        const char *l = "0123456789abcdefABCDEF:.";
        if (hlen < 4) /*'[::]' is the shortest valid IPv6 address string*/
        {
            return QURL_ECODE_URL_MALFORMED_INPUT;
        }
        hostname++;
        hlen -= 2;

        if (hostname[hlen] != ']')
        {
            return QURL_ECODE_URL_MALFORMED_INPUT;
        }

        /* only valid letters are ok */
        len = qurl_strspn(hostname, l);
        if (hlen != len)
        {
            hlen = len;
            if (hostname[len] == '%')
            {
                /* this could now be '%[zone id]' */
                char  zoneid[16] = {0};
                int   i = 0;
                char *h = &hostname[len + 1];
                /* pass '25' if present and is a url encoded percent sign */
                if (!qurl_strncmp(h, "25", 2) && h[2] && (h[2] != ']'))
                    h += 2;
                while (*h && (*h != ']') && (i < 15)) zoneid[i++] = *h++;
                if (!i || (']' != *h))
                {
                    return QURL_ECODE_URL_MALFORMED_INPUT;
                }
                zoneid[i] = 0;
                uurl_ptr->zoneid = qurl_strdup(zoneid);
                if (!uurl_ptr->zoneid)
                {
                    return QURL_ECODE_URL_MALFORMED_INPUT;
                }
                hostname[len] = ']';   /* insert end bracket */
                hostname[len + 1] = 0; /* terminate the hostname */
            }
            else
            {
                return QURL_ECODE_URL_MALFORMED_INPUT;
            }
            /* hostname is fine */
        }
#if QURL_CFG_ENABLE_IPV6
        hostname[hlen] = 0; /* end the address there */
        if (1 != inet_pton(AF_INET6, hostname, dest))
        {
            return QURL_ECODE_URL_MALFORMED_INPUT;
        }
        hostname[hlen] = ']'; /* restore ending bracket */
#endif
    }
    else
    {
        /* letters from the second string is not ok */
        len = qurl_strcspn(hostname, " ");
        if (hlen != len)
        {
            /* hostname with bad content */
            return QURL_ECODE_URL_MALFORMED_INPUT;
        }
    }
    if (!hostname[0])
    {
        return QURL_ECODE_URL_NO_HOST;
    }
    return QURL_OK;
}

/**
 * @brief Concatenation of URL paths. Making it an absolute path URL.
 * @note Returns non-null, must be freed by the user.
 */
static char *url_concat(const char *base_url, const char *rel_url)
{
    qurl_bit_t  host_changed = QOSA_FALSE; /*Whether the rel_url includes a hostname*/
    char       *newest = QOSA_NULL;
    char       *protsep = QOSA_NULL;
    char       *pathsep = QOSA_NULL;
    qosa_size_t newlen = 0;
    qosa_size_t urllen = 0;
    const char *use_url = rel_url;

    /*Processing with a copy*/
    char *url_clone = qurl_strdup(base_url);
    if (!url_clone)
    {
        return QOSA_NULL;
    }

    /*Skip scheme*/
    protsep = qurl_strstr(url_clone, "//");
    if (!protsep)
    {
        protsep = url_clone;
    }
    else
    {
        protsep += 2; /*Skip "//"*/
    }

    /*The rel_url passed is not an absolute path*/
    if ('/' != rel_url[0])
    {
        int level = 0;

        /*First, truncate the query parameters and any subsequent strings after the base_url.*/
        pathsep = qurl_strchr(protsep, '?');
        if (pathsep)
        {
            *pathsep = 0;
        }

        /*If rel_url contains a relative path and base_url has a trailing slash, the relative path will be appended after the last slash.
If rel_url is only a query string (starting with a question mark ?), append this new query string to the end of the entire base_url.*/
        if (use_url[0] != '?')
        {
            pathsep = qurl_strrchr(protsep, '/'); /*Find the last "/" in the path of base_url, and truncate the string after it (essentially the resource name).*/
            if (pathsep)
            {
                *pathsep = 0;
            }
        }
        /*Find the first "/" in the path of base_url*/
        pathsep = qurl_strchr(protsep, '/');
        if (pathsep)
        {
            protsep = pathsep + 1;
        }
        else
        {
            protsep = QOSA_NULL;
        }

        /* now deal with one "./" or any amount of "../" in the newurl and act accordingly */

        /*Analyze the beginning part of the rel_url relative path. "./" indicates the same level, "../" indicates the parent level.*/
        if ((use_url[0] == '.') && (use_url[1] == '/'))
        {
            use_url += 2; /* just skip the "./" */
        }

        while ((use_url[0] == '.') && (use_url[1] == '.') && (use_url[2] == '/'))
        {
            level++;
            use_url += 3; /* pass the "../" */
        }

        if (protsep)
        {
            while (level--)
            {
                /*base_url calculates its directory levels from the rightmost side*/
                pathsep = qurl_strrchr(protsep, '/');
                if (pathsep)
                {
                    *pathsep = 0;
                }
                else
                {
                    *protsep = 0;
                    break;
                }
            }
        }
    }
    else /*The passed rel_url is an absolute path*/
    {
        /*That is the new absolute path.*/

        if (rel_url[1] == '/') /*"//xx": indicates that the hostname part is included*/
        {
            *protsep = 0;
            use_url = &rel_url[2]; /*Skip double slashes*/
            host_changed = QOSA_TRUE;
        }
        else /*"/xx": pure path*/
        {
            /* cut off the original URL from the first slash, or deal with URLs without slash */
            pathsep = qurl_strchr(protsep, '/');
            if (pathsep) /*There is a "/"*/
            {
                /*If it is such a strange URL, there is no path, for example: "http://www.url.com?dir=/home/daniel"
				 * In this case, the found "/" does not belong to the path part and should be discarded. Also, the query parameter part should be truncated.*/
                char *sep = qurl_strchr(protsep, '?');
                if (sep && (sep < pathsep))
                {
                    pathsep = sep;
                }
                *pathsep = 0;
            }
            else /*Without "/", there is no path*/
            {
                /*No path, such as "http://www.url.com?id=2380"
				 * Then directly cut off the query parameter part*/
                pathsep = qurl_strchr(protsep, '?');
                if (pathsep)
                {
                    *pathsep = 0;
                }
            }
        }
    }

    /*URL redirection: replace spaces on the left side of the query marker '?' with "%20", and spaces on the right side with "+".*/
    newlen = url_strlen((char *)use_url, !host_changed);

    urllen = qurl_strlen(url_clone);

    newest = url_malloc(
        urllen + 1 + /*+1: May need to add '/'*/
        newlen + 1   /*+1: Append '\0'*/
    );

    if (!newest)
    {
        url_free(url_clone);
        return QOSA_NULL;
    }

    /*Combine to form a new absolute path URL*/
    qurl_memcpy(newest, url_clone, urllen);

    /*Check if a slash needs to be added*/
    /*Conditions for not appending '/': 1. rel_url is an absolute path; 2. rel_url has no path.*/
    if (('/' == use_url[0]) || (protsep && !*protsep) || ('?' == use_url[0]))
    {
        ;
    }
    else
    {
        newest[urllen++] = '/';
    }

    /*The remaining part parses URL encoding and appends it to the newest output.*/
    url_strcpy(&newest[urllen], use_url, !host_changed);

    url_free(url_clone);

    return newest;
}

#define url_strprecmp(a, b) qurl_strncmp(a, b, qurl_strlen(a))

static qurl_ecode_t url_parse(qurl_url_t *uurl_ptr, const char *url_ptr, unsigned int flags)
{
    qurl_ecode_t  ret = QURL_OK;
    qurl_bit_t    path_alloced = QOSA_FALSE;
    int           i = 0;
    qosa_uint16_t url_len = 0;
    char         *path = QOSA_NULL;
    char         *fragment = QOSA_NULL;
    char         *query = QOSA_NULL;
    char         *hostname = QOSA_NULL;
    qurl_bit_t    url_has_scheme = QOSA_FALSE;
    char          scheme_buf[QURL_CFG_URL_SCHEME_LEN_MAX + 1] = {0};
    qosa_uint16_t scheme_len = 0;
    const char   *schemep = QOSA_NULL;
    const char   *p = QOSA_NULL;
    const char   *hostp = QOSA_NULL;
    qosa_uint16_t len = 0;

    /*1. Parameter validation: handled by external API*/
    /*2. Ensure that the data has been cleared to avoid memory leaks.*/
    qurl_url_url_free(uurl_ptr);
    /*3. URL Check*/
    url_len = qurl_strlen(url_ptr);
    if (url_len > QURL_CFG_URL_LEN_MAX)
    {
        qurl_loge("url is too long:[%d][%d]\r\n", QURL_CFG_URL_LEN_MAX, qurl_strlen(url_ptr));
        return QURL_ECODE_URL_TOO_LONG;
    }

    /*4. Prepare Resources*/
    path = uurl_ptr->scratch = url_malloc(url_len * 2 + 2); /*Prepare sufficient memory resources at once*/
    if (path == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }
    hostname = &path[url_len + 1];

    /*5. Parsing*/
    /*scheme parsing*/
    if (qurl_url_is_absolute(url_ptr, scheme_buf, sizeof(scheme_buf)))
    {
        url_has_scheme = QOSA_TRUE;
        scheme_len = qurl_strlen(scheme_buf);
    }

    if (url_has_scheme)
    {
        p = &url_ptr[scheme_len + 1];
        while (p && (*p == '/') && (i < 4))
        {
            p++;
            i++;
        }
        if ((i < 1) || (i > 3))
        {
            ret = QURL_ECODE_URL_MALFORMED_INPUT;
            goto exit;
        }

        /*Matching application protocol stack*/
        schemep = scheme_buf;
        ret = qurl_port_prot_match_by_scheme(schemep, QOSA_NULL);
        if (ret != QURL_OK && !(flags & QURL_URL_NON_SUPPORT_SCHEME))
        {
            ret = QURL_ECODE_URL_MALFORMED_INPUT;
            goto exit;
        }

        ret = url_junk_scan(schemep);
        if (ret != QURL_OK)
        {
            ret = QURL_ECODE_URL_MALFORMED_INPUT;
            goto exit;
        }
    }
    else
    {
        if (!(flags & (QURL_URL_DEFAULT_SCHEME | QURL_URL_GUESS_SCHEME)))
        {
            ret = QURL_ECODE_URL_MALFORMED_INPUT;
            goto exit;
        }
        if (flags & QURL_URL_DEFAULT_SCHEME)
        {
            schemep = QURL_CFG_URL_SCHEME_DEFAULT;
        }

        /*
		 * The URL was badly formatted, let's try without scheme specified.
		 */
        p = url_ptr;
    }
    if (schemep) /*Scheme parsing completed, writing*/
    {
        uurl_ptr->scheme = qurl_strdup(schemep);
        if (!uurl_ptr->scheme)
        {
            ret = QURL_ECODE_NO_MEMORY;
            goto exit;
        }
    }

    /*Parse hostname*/
    hostp = p;
    while (*p && !URL_HOSTNAME_END(*p)) /* find end of host name */
    {
        p++;
    }
    len = p - hostp;
    if (len)
    {
        qurl_memcpy(hostname, hostp, len);
        hostname[len] = 0;
    }
    else
    {
        if (!(flags & QURL_URL_NO_AUTHORITY))
        {
            ret = QURL_ECODE_URL_MALFORMED_INPUT;
            goto exit;
        }
    }
    len = qurl_strlen(p);
    qurl_memcpy(path, p, len);
    path[len] = 0;

    ret = url_junk_scan(path);
    if (ret != QURL_OK)
    {
        ret = QURL_ECODE_URL_MALFORMED_INPUT;
        goto exit;
    }

    if ((flags & QURL_URL_URLENCODE) && path[0])
    {
        /* worst case output length is 3x the original! */
        char *newp = qurl_malloc(qurl_strlen(path) * 3);
        if (!newp)
        {
            ret = QURL_ECODE_NO_MEMORY;
            goto exit;
        }
        path_alloced = QOSA_TRUE;
        url_strcpy(newp, path, QOSA_TRUE); /* consider it relative */
        uurl_ptr->temppath = path = newp;
    }

    /*Parse fragment*/
    fragment = qurl_strchr(path, '#');
    if (fragment)
    {
        *fragment++ = 0;
        if (fragment[0])
        {
            uurl_ptr->fragment = qurl_strdup(fragment);
            if (!uurl_ptr->fragment)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
        }
    }

    /*Parse query*/
    query = qurl_strchr(path, '?');
    if (query)
    {
        *query++ = 0;
        /* done even if the query part is a blank string */
        uurl_ptr->query = qurl_strdup(query);
        if (!uurl_ptr->query)
        {
            ret = QURL_ECODE_NO_MEMORY;
            goto exit;
        }
    }

    if (path[0] == 0)
    {
        /* if there's no path left set, unset */
        path = QOSA_NULL;
    }
    else
    {
        if (!(flags & QURL_URL_PATH_AS_IS))
        {
            /* remove ../ and ./ sequences according to RFC3986 */
            char *newp = qurl_url_dedotdotify(path);
            if (!newp)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
            if (qurl_strcmp(newp, path))
            {
                /* if we got a new version */
                if (path_alloced)
                {
                    url_free(uurl_ptr->temppath);
                }
                uurl_ptr->temppath = path = newp;
                path_alloced = QOSA_TRUE;
            }
            else
            {
                url_free(newp);
            }
        }

        uurl_ptr->path = path_alloced ? path : qurl_strdup(path);
        if (!uurl_ptr->path)
        {
            return QURL_ECODE_NO_MEMORY;
        }
        uurl_ptr->temppath = QOSA_NULL; /* used now */
    }

    if (hostname)
    {
        /*
		 * Parse and strip login details from the hostname.
		 */
        if (url_junk_scan(hostname))
        {
            ret = QURL_ECODE_URL_MALFORMED_INPUT;
            goto exit;
        }

        ret = url_parse_hostname_login(uurl_ptr, &hostname, flags);
        if (ret != QURL_OK)
        {
            goto exit;
        }

        ret = qurl_url_parse_port(uurl_ptr, hostname);
        if (ret != QURL_OK)
        {
            goto exit;
        }

        if (0 == qurl_strlen(hostname) && (flags & QURL_URL_NO_AUTHORITY))
        {
            /*Skip the hostname check, as this field can be empty.*/
        }
        else
        {
            ret = url_hostname_check(uurl_ptr, hostname);
            if (ret)
            {
                goto exit;
            }
        }

        uurl_ptr->host = qurl_strdup(hostname);
        if (!uurl_ptr->host)
        {
            ret = QURL_ECODE_NO_MEMORY;
            goto exit;
        }

        if ((flags & QURL_URL_GUESS_SCHEME) && !schemep)
        {
            /* legacy curl-style guess based on host name */
            if (!url_strprecmp("ftp.", hostname))
                schemep = "ftp";
            else if (!url_strprecmp("dict.", hostname))
                schemep = "dict";
            else if (!url_strprecmp("ldap.", hostname))
                schemep = "ldap";
            else if (!url_strprecmp("imap.", hostname))
                schemep = "imap";
            else if (!url_strprecmp("smtp.", hostname))
                schemep = "smtp";
            else if (!url_strprecmp("pop3.", hostname))
                schemep = "pop3";
            else
                schemep = "http";

            uurl_ptr->scheme = qurl_strdup(schemep);
            if (!uurl_ptr->scheme)
            {
                ret = QURL_ECODE_NO_MEMORY;
                goto exit;
            }
        }
    }

    url_free(uurl_ptr->scratch);
    url_free(uurl_ptr->temppath);
    return QURL_OK;

exit:
    qurl_loge("faild:[%s]\r\n", url_ptr);
    qurl_loge("faild:[%x]\r\n", ret);
    /*1. Resource Release*/
    qurl_url_url_free(uurl_ptr);

    return ret;
}

static void url_mv(qurl_url_t *src_url_ptr, qurl_url_t *dst_url_ptr)
{
    /*1. First, safely clear out the old qurl stored in the target.*/
    qurl_url_url_free(dst_url_ptr);

    /*2. Copy source qurl to destination qurl*/
    qurl_memcpy(dst_url_ptr, src_url_ptr, sizeof(qurl_url_t));
}

/**
 * @brief Parse the given string for URL decoding
 * @details
 * @param[in] const char *str_ptr: string to be decoded
 * @param[in] qosa_size_t str_len: The length of the string to be decoded. When it is 0, the string length is automatically recognized.
 * @param[out] char **ostr_pptr: The decoded string
 * @param[out] qosa_size_t *ostr_len_ptr: length of the decoded string
 * @param[in] qurl_url_reject_e ctrl: Control option
 * @retval
 */
qurl_ecode_t qurl_url_decode(const char *str_ptr, qosa_size_t str_len, char **ostr_pptr, qosa_size_t *ostr_len_ptr, qurl_url_reject_e ctrl)
{
    qurl_ecode_t  ret = QURL_OK;
    qosa_size_t   len = 0;
    qosa_size_t   str_index = 0;
    unsigned long hex = 0;
    char         *ostr_ptr = QOSA_NULL;

    if (str_ptr == QOSA_NULL || ostr_pptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    len = (str_len ? str_len : qurl_strlen(str_ptr)) + 1;
    ostr_ptr = url_malloc(len);
    if (ostr_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }

    while (--len > 0)
    {
        unsigned char chr = *str_ptr;
        if (('%' == chr) && (len > 2) && qurl_ctype_isxdigit(str_ptr[1]) && qurl_ctype_isxdigit(str_ptr[2]))
        {
            /* this is two hexadecimal digits following a '%' */
            char  hexstr[3] = {0};
            char *ptr = QOSA_NULL;
            hexstr[0] = str_ptr[1];
            hexstr[1] = str_ptr[2];
            hexstr[2] = 0;

            hex = qurl_strtoul(hexstr, &ptr, 16);

            chr = (unsigned char)(hex & 0xFF); /* this long is never bigger than 255 anyway */

            str_ptr += 2;
            len -= 2;
        }

        if (((ctrl == QURL_URL_REJECT_CTRL) && (chr < 0x20)) || ((ctrl == QURL_URL_REJECT_ZERO) && (chr == 0)))
        {
            url_free(ostr_ptr);
            return QURL_ECODE_URL_MALFORMED;
        }

        ostr_ptr[str_index++] = chr;
        str_ptr++;
    }
    ostr_ptr[str_index] = 0; /* terminate it */

    if (ostr_len_ptr)
    {
        /* store output size */
        *ostr_len_ptr = str_index;
    }

    /* store output string */
    *ostr_pptr = ostr_ptr;

    return ret;
}

qurl_ecode_t qurl_url_create(qurl_url_t **uurt_pptr)
{
    qurl_url_t *uurt_ptr = QOSA_NULL;

    if (uurt_pptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    uurt_ptr = url_malloc(sizeof(qurl_url_t));
    if (uurt_ptr == QOSA_NULL)
    {
        return QURL_ECODE_NO_MEMORY;
    }

    *uurt_pptr = uurt_ptr;

    return QURL_OK;
}

qurl_ecode_t qurl_url_delete(qurl_url_t *uurt_ptr)
{
    qurl_url_url_free(uurt_ptr);
    url_free(uurt_ptr);

    return QURL_OK;
}

qurl_ecode_t qurl_url_getopt(qurl_url_t *uurl_ptr, qurl_url_opt_e opt, char **part_pptr, unsigned int flags)
{
    qurl_ecode_t ret = 0;
    char         port_buf[8] = {0};
    char        *ptr = QOSA_NULL;
    qurl_bit_t   urldecode = (flags & QURL_URL_URLDECODE) ? 1 : 0;
    qurl_bit_t   plusdecode = QOSA_FALSE;

    if (uurl_ptr == QOSA_NULL || part_pptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }
    *part_pptr = QOSA_NULL;

    switch (opt)
    {
        case QURL_URL_OPT_URL: {
            char *url = QOSA_NULL;
            char *scheme = QOSA_NULL;
            char *options = uurl_ptr->options;
            char *port = uurl_ptr->port;
            char *allochost = QOSA_NULL;
            qurl_logd("scheme:   [%s]\r\n", uurl_ptr->scheme);
            qurl_logd("user:     [%s]\r\n", uurl_ptr->user);
            qurl_logd("password: [%s]\r\n", uurl_ptr->password);
            qurl_logd("options:  [%s]\r\n", uurl_ptr->options);
            qurl_logd("host:     [%s]\r\n", uurl_ptr->host);
            qurl_logd("zoneid:   [%s]\r\n", uurl_ptr->zoneid);
            qurl_logd("port:     [%s]\r\n", uurl_ptr->port);
            qurl_logd("path:     [%s]\r\n", uurl_ptr->path);
            qurl_logd("query:    [%s]\r\n", uurl_ptr->query);
            qurl_logd("fragment: [%s]\r\n", uurl_ptr->fragment);

            if (uurl_ptr->scheme && !qurl_strcasecmp("file", uurl_ptr->scheme)) /*File System*/
            {
#if 0
				url_len = qurl_strlen("file://") + qurl_strlen(uurl_ptr->path) + (uurl_ptr->fragment?(1+qurl_strlen(uurl_ptr->fragment)):0);
				url = qurl_malloc(url_len + 1);
				if(url != QOSA_NULL)
				{
					url[url_len] = 0x00;
					qurl_snprintf(url, url_len+1, "file://%s%s%s",
					         uurl_ptr->path,
					         uurl_ptr->fragment ? "#" : "",
					         uurl_ptr->fragment ? uurl_ptr->fragment : "");
				}
#endif

                qurl_asprintf(&url, "file://%s%s%s", uurl_ptr->path, uurl_ptr->fragment ? "#" : "", uurl_ptr->fragment ? uurl_ptr->fragment : "");
            }
            else if (!uurl_ptr->host)
            {
                return QURL_ECODE_URL_NO_HOST;
            }
            else
            {
                const qurl_port_prot_t *port_prot_ptr = QOSA_NULL;

                if (uurl_ptr->scheme)
                {
                    scheme = uurl_ptr->scheme;
                }
                else if (flags & QURL_URL_DEFAULT_SCHEME)
                {
                    scheme = (char *)QURL_CFG_URL_SCHEME_DEFAULT;
                }
                else
                {
                    return QURL_ECODE_URL_NO_SCHEME;
                }

                ret = qurl_port_prot_match_by_scheme(scheme, &port_prot_ptr);
                if (!port && (flags & QURL_URL_DEFAULT_PORT))
                {
                    /*If no port number is specified in the qurl, the protocol's default port number is used.*/
                    if (port_prot_ptr)
                    {
                        qurl_snprintf(port_buf, sizeof(port_buf), "%ld", port_prot_ptr->def_port);
                        port = port_buf;
                    }
                }
                else if (port)
                {
                    /*with a specified port number*/
                    if (port_prot_ptr && (port_prot_ptr->def_port == uurl_ptr->portnum) && (flags & QURL_URL_NO_DEFAULT_PORT))
                    {
                        port = QOSA_NULL;
                    }
                }

                if (port_prot_ptr && !(port_prot_ptr->flags & QURL_PROT_OPT_URLOPTIONS))
                {
                    options = QOSA_NULL;
                }

                if ((uurl_ptr->host[0] == '[') && uurl_ptr->zoneid)
                {
                    /* make it '[ host %25 zoneid ]' */
                    qosa_size_t hostlen = qurl_strlen(uurl_ptr->host);
                    qosa_size_t alen = hostlen + 3 + qurl_strlen(uurl_ptr->zoneid) + 1;
                    allochost = url_malloc(alen);
                    if (!allochost)
                    {
                        return QURL_ECODE_NO_MEMORY;
                    }
                    qurl_memcpy(allochost, uurl_ptr->host, hostlen - 1);
                    qurl_snprintf(&allochost[hostlen - 1], alen - hostlen + 1, "%%25%s]", uurl_ptr->zoneid);
                }

#if 0
				url_len = (qurl_strlen(scheme)) + (qurl_strlen("://")) + (uurl_ptr->user ? qurl_strlen(uurl_ptr->user) : 0) + (uurl_ptr->password ? qurl_strlen(uurl_ptr->password) + 1 : 0) + (options ? qurl_strlen(options) + 1 : 0) + ((uurl_ptr->user || uurl_ptr->password || options) ? 1 : 0) + (allochost ? qurl_strlen(allochost) : qurl_strlen(uurl_ptr->host)) + (port ? qurl_strlen(port) + 1 : 0) + ((uurl_ptr->path && (uurl_ptr->path[0] != '/')) ? 1 : 0) + ((uurl_ptr->path) ? qurl_strlen(uurl_ptr->path) : 1) + ((uurl_ptr->query && uurl_ptr->query[0]) ? qurl_strlen(uurl_ptr->query) + 1 : 0) + (uurl_ptr->fragment ? qurl_strlen(uurl_ptr->fragment) + 1 : 0);

				url = qurl_malloc(url_len + 1);
				if (url == QOSA_NULL)
				{
					url[url_len] = 0x00;
					qurl_snprintf(url, url_len + 1, "%s://%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
					         scheme,
					         uurl_ptr->user ? uurl_ptr->user : "",
					         uurl_ptr->password ? ":" : "",
					         uurl_ptr->password ? uurl_ptr->password : "",
					         options ? ";" : "",
					         options ? options : "",
					         (uurl_ptr->user || uurl_ptr->password || options) ? "@" : "",
					         allochost ? allochost : uurl_ptr->host,
					         port ? ":" : "",
					         port ? port : "",
					         (uurl_ptr->path && (uurl_ptr->path[0] != '/')) ? "/" : "",
					         uurl_ptr->path ? uurl_ptr->path : "/",
					         (uurl_ptr->query && uurl_ptr->query[0]) ? "?" : "",
					         (uurl_ptr->query && uurl_ptr->query[0]) ? uurl_ptr->query : "",
					         uurl_ptr->fragment ? "#" : "",
					         uurl_ptr->fragment ? uurl_ptr->fragment : "");
				}
#endif
                qurl_asprintf(
                    &url,
                    "%s://%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                    scheme,
                    uurl_ptr->user ? uurl_ptr->user : "",
                    uurl_ptr->password ? ":" : "",
                    uurl_ptr->password ? uurl_ptr->password : "",
                    options ? ";" : "",
                    options ? options : "",
                    (uurl_ptr->user || uurl_ptr->password || options) ? "@" : "",
                    allochost ? allochost : uurl_ptr->host,
                    port ? ":" : "",
                    port ? port : "",
                    (uurl_ptr->path && (uurl_ptr->path[0] != '/')) ? "/" : "",
                    uurl_ptr->path ? uurl_ptr->path : "/",
                    (uurl_ptr->query && uurl_ptr->query[0]) ? "?" : "",
                    (uurl_ptr->query && uurl_ptr->query[0]) ? uurl_ptr->query : "",
                    uurl_ptr->fragment ? "#" : "",
                    uurl_ptr->fragment ? uurl_ptr->fragment : ""
                );

                url_free(allochost);
            }
            if (!url)
            {
                return QURL_ECODE_NO_MEMORY;
            }
            *part_pptr = url;
            return QURL_OK;
            //break;
        }
        case QURL_URL_OPT_SCHEME: {
            ret = QURL_ECODE_URL_NO_SCHEME;
            ptr = uurl_ptr->scheme;
            urldecode = QOSA_FALSE; /*The scheme does not have encoding/decoding.*/
            break;
        }
        case QURL_URL_OPT_USER: {
            ret = QURL_ECODE_URL_NO_USER;
            ptr = uurl_ptr->user;
            break;
        }
        case QURL_URL_OPT_PASSWORD: {
            ret = QURL_ECODE_URL_NO_PASSWORD;
            ptr = uurl_ptr->password;
            break;
        }
        case QURL_URL_OPT_OPTIONS: {
            ret = QURL_ECODE_URL_NO_OPTIONS;
            ptr = uurl_ptr->options;
            break;
        }
        case QURL_URL_OPT_HOST: {
            ret = QURL_ECODE_URL_NO_HOST;
            ptr = uurl_ptr->host;
            break;
        }
        case QURL_URL_OPT_ZONEID: {
            ptr = uurl_ptr->zoneid;
            break;
        }
        case QURL_URL_OPT_PORT: {
            ret = QURL_ECODE_URL_NO_PORT;
            ptr = uurl_ptr->port;
            urldecode = QOSA_FALSE; /*The port does not have codec support.*/
            if (!ptr && (flags & QURL_URL_DEFAULT_PORT) && uurl_ptr->scheme)
            {
                /*If not specified, use the default PORT of the protocol stack.*/
                const qurl_port_prot_t *port_prot_ptr = QOSA_NULL;
                ret = qurl_port_prot_match_by_scheme(uurl_ptr->scheme, &port_prot_ptr);
                if (ret == QURL_OK && port_prot_ptr)
                {
                    qurl_snprintf(port_buf, sizeof(port_buf), "%d", port_prot_ptr->def_port);
                    ptr = port_buf;
                }
            }
            else if (ptr && uurl_ptr->scheme)
            {
                /*If the specified port number matches the default port number of the protocol stack, it is considered as if the URL did not specify a port number.*/
                const qurl_port_prot_t *port_prot_ptr = QOSA_NULL;
                ret = qurl_port_prot_match_by_scheme(uurl_ptr->scheme, &port_prot_ptr);
                if (ret == QURL_OK && port_prot_ptr && (port_prot_ptr->def_port == uurl_ptr->portnum) && (flags & QURL_URL_NO_DEFAULT_PORT))
                {
                    ptr = QOSA_NULL;
                }
            }
            break;
        }
        case QURL_URL_OPT_PATH: {
            ptr = uurl_ptr->path;
            if (!ptr)
            {
                ptr = "/";
            }
            break;
        }
        case QURL_URL_OPT_QUERY: {
            ret = QURL_ECODE_URL_NO_QUERY;
            ptr = uurl_ptr->query;
            plusdecode = urldecode;
            break;
        }
        case QURL_URL_OPT_FRAGMENT: {
            ret = QURL_ECODE_URL_NO_FRAGMENT;
            ptr = uurl_ptr->fragment;
            break;
        }
        default:
            ret = QURL_ECODE_PARAM_INVALID;
            break;
    }

    if (ptr)
    {
        *part_pptr = qurl_strdup(ptr);
        if (!*part_pptr)
        {
            return QURL_ECODE_NO_MEMORY;
        }
        if (plusdecode) /*query decoding*/
        {
            /* '+' ==> ' ' */
            char *plus = QOSA_NULL;
            for (plus = *part_pptr; *plus; ++plus)
            {
                if (*plus == '+')
                {
                    *plus = ' ';
                }
            }
        }
        if (urldecode) /*URL decoding*/
        {
            char        *decoded = QOSA_NULL;
            qosa_size_t  dlen = 0;
            qurl_ecode_t res = qurl_url_decode(*part_pptr, 0, &decoded, &dlen, QURL_URL_REJECT_CTRL);
            url_free(*part_pptr);
            if (res != QURL_OK)
            {
                return QURL_ECODE_URL_DECODE;
            }
            *part_pptr = decoded;
        }
        return QURL_OK;
    }
    else
        return ret;
}

qurl_ecode_t qurl_url_setopt(qurl_url_t *uurl_ptr, qurl_url_opt_e opt, const char *part_ptr, unsigned int flags)
{
    qurl_ecode_t ret = 0;
    long         port = 0;
    char       **storep = QOSA_NULL; /*Fields to be operated on*/
    char        *newp = QOSA_NULL;
    qosa_size_t  len = 0;
    qurl_bit_t   urlencode = (flags & QURL_URL_URLENCODE) ? 1 : 0;
    qurl_bit_t   plusencode = QOSA_FALSE;
    qurl_bit_t   urlskipslash = QOSA_FALSE;
    qurl_bit_t   appendquery = QOSA_FALSE;
    qurl_bit_t   equalsencode = QOSA_FALSE;

    if (uurl_ptr == QOSA_NULL)
    {
        return QURL_ECODE_PARAM_INVALID;
    }

    switch (opt)
    {
        case QURL_URL_OPT_URL: {
            qurl_url_t qurl_cache = {0}; /*Processing buffer for qurl*/
            char      *old_url = QOSA_NULL;
            char      *redired_url = QOSA_NULL;

            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                qurl_url_url_free(uurl_ptr);
                return QURL_OK;
            }

            qurl_logd("part_ptr:[%s]\r\n", part_ptr);

            /*When the URL's path is an absolute path, it can be parsed directly; if it is a relative path, it must be appended to the old URL's path before parsing.*/

            /*1. If the new URL is an absolute path, directly replace the old URL.*/
            if (qurl_url_is_absolute(part_ptr, QOSA_NULL, QURL_CFG_URL_SCHEME_LEN_MAX + 1))
            {
                ret = url_parse(&qurl_cache, part_ptr, flags);
                if (ret == QURL_OK)
                {
                    url_mv(&qurl_cache, uurl_ptr);
                }
                //else the url_parse interface will actively release resources if an internal operation fails.

                return ret;
            }

            /*2. If it is a relative path and there is no old URL, proceed directly to parsing.*/
            ret = qurl_url_getopt(uurl_ptr, QURL_URL_OPT_URL, &old_url, flags);
            if (ret != QURL_OK)
            {
                ret = url_parse(&qurl_cache, part_ptr, flags);
                if (ret == QURL_OK)
                {
                    url_mv(&qurl_cache, uurl_ptr);
                }
                //else the url_parse interface will actively release resources if an internal operation fails.

                return ret;
            }

            /*3. Relative path: if an old URL exists, concatenate the path parts of the two URLs.*/
            redired_url = url_concat(old_url, part_ptr);
            url_free(old_url);
            if (redired_url == QOSA_NULL)
            {
                return QURL_ECODE_NO_MEMORY;
            }
            /*Now it can be parsed.*/
            ret = url_parse(&qurl_cache, redired_url, flags);
            if (ret == QURL_OK)
            {
                url_mv(&qurl_cache, uurl_ptr);
            }
            url_free(redired_url);//Destroy internally allocated resources
            //else the url_parse interface will actively release resources if an internal operation fails.
            return ret;
            // break;
        }
        case QURL_URL_OPT_SCHEME: {
            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->scheme);
                return QURL_OK;
            }

            if (qurl_strlen(part_ptr) > QURL_CFG_URL_SCHEME_LEN_MAX)
            {
                /* too long */
                return QURL_ECODE_URL_TOO_LONG;
            }
            if (!(flags & QURL_URL_NON_SUPPORT_SCHEME) &&
                /* verify that it is a fine scheme */
                qurl_port_prot_match_by_scheme(part_ptr, QOSA_NULL) != QURL_OK)
            {
                return QURL_ECODE_PROT_NO_MATCH;
            }
            storep = &uurl_ptr->scheme;
            urlencode = QOSA_FALSE; /* never */
            break;
        }
        case QURL_URL_OPT_USER: {
            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->user);
                return QURL_OK;
            }

            storep = &uurl_ptr->user;
            break;
        }
        case QURL_URL_OPT_PASSWORD: {
            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->password);
                return QURL_OK;
            }

            storep = &uurl_ptr->password;
            break;
        }
        case QURL_URL_OPT_OPTIONS: {
            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->options);
                return QURL_OK;
            }

            storep = &uurl_ptr->options;
            break;
        }
        case QURL_URL_OPT_HOST: {
            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->host);
                return QURL_OK;
            }

            storep = &uurl_ptr->host;
            url_free(uurl_ptr->zoneid);
            break;
        }
        case QURL_URL_OPT_PORT: {
            char *endp = QOSA_NULL;
            urlencode = QOSA_FALSE;

            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->port);
                return QURL_OK;
            }

            port = qurl_strtol(part_ptr, &endp, 10);
            if ((port <= 0) || (port > 0xffff))
            {
                return QURL_ECODE_URL_BAD_PORT_NUMBER;
            }
            if (*endp)
            {
                return QURL_ECODE_URL_MALFORMED_INPUT;
            }
            storep = &uurl_ptr->port;
            break;
        }
        case QURL_URL_OPT_PATH: {
            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->path);
                return QURL_OK;
            }

            storep = &uurl_ptr->path;
            break;
        }
        case QURL_URL_OPT_QUERY: {
            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->query);
                return QURL_OK;
            }

            plusencode = urlencode;
            appendquery = (flags & QURL_URL_APPENDQUERY) ? 1 : 0;
            equalsencode = appendquery;
            storep = &uurl_ptr->query;
            break;
        }
        case QURL_URL_OPT_FRAGMENT: {
            if (part_ptr == QOSA_NULL) /*To clear resources*/
            {
                url_free(uurl_ptr->fragment);
                return QURL_OK;
            }

            storep = &uurl_ptr->fragment;
            break;
        }
        default:
            return QURL_ECODE_PARAM_INVALID;
    }

    if (!storep)
    {
        return QURL_ECODE_FAILURE; /*Should not reach this point*/
    }

    /*Write processing*/
    newp = (char *)part_ptr;
    len = qurl_strlen(part_ptr);

    if (len > QURL_CFG_URL_LEN_MAX)
    {
        return QURL_ECODE_URL_TOO_LONG;
    }

    if (urlencode) /*URL encoding is required.*/
    {
        const unsigned char *i = QOSA_NULL;
        char                *o = QOSA_NULL;
        /*Apply for sufficient space:
		 * *3: The '/' in the path part of the URL needs to be converted to "%20".
		 * +1: Security terminator.*/
        char *enc = url_malloc(len * 3 + 1);
        if (!enc)
        {
            return QURL_ECODE_NO_MEMORY;
        }
        for (i = (const unsigned char *)part_ptr, o = enc; *i; i++)
        {
            if ((*i == ' ') && plusencode)
            {
                *o = '+';
                o++;
            }
            else if (url_is_unreserved(*i) || ((*i == '/') && urlskipslash) || ((*i == '=') && equalsencode)) /*[lzm][note]This operation is used for special requirements; querying parameters may require this operation. No RFC has been found for it yet.*/
            {
                if ((*i == '=') && equalsencode)
                    /*Only need to skip the first equal sign*/
                    equalsencode = QOSA_FALSE;
                *o = *i;
                o++;
            }
            else
            {
                qurl_snprintf(o, 4, "%%%02x", *i);
                o += 3;
            }
        }
        *o = 0; /* null-terminate */
        newp = enc;
    }
    else /*No URL encoding required*/
    {
        char *p = QOSA_NULL;
        newp = qurl_strdup(part_ptr);
        if (!newp)
        {
            return QURL_ECODE_NO_MEMORY;
        }
        p = (char *)newp;
        while (*p)
        {
            /*Ensure the two xdigit characters after % must be lowercase.*/
            if ((*p == '%') && qurl_ctype_isxdigit(p[1]) && qurl_ctype_isxdigit(p[2]) && (qurl_ctype_isupper(p[1]) || qurl_ctype_isupper(p[2])))
            {
                p[1] = (char)qurl_ctype_islower(p[1]);
                p[2] = (char)qurl_ctype_islower(p[2]);
                p += 3;
            }
            else
                p++;
        }
    }

    if (appendquery)
    {
        /*Append the string to the old query. If there is no delimiter at the end of the existing query, add an "&" delimiter.*/
        qosa_size_t querylen = uurl_ptr->query ? qurl_strlen(uurl_ptr->query) : 0;
        qurl_bit_t  addamperand = querylen && (uurl_ptr->query[querylen - 1] != '&');
        if (querylen)
        {
            qosa_size_t newplen = qurl_strlen(newp);
            char       *p = url_malloc(querylen + addamperand + newplen + 1);
            if (!p)
            {
                url_free(newp);
                return QURL_ECODE_NO_MEMORY;
            }
            qurl_strcpy(p, uurl_ptr->query); /* original query */
            if (addamperand)
            {
                p[querylen] = '&';                         /* ampersand */
            }
            qurl_strcpy(&p[querylen + addamperand], newp); /* new suffix */
            url_free(newp);
            url_free(*storep);
            *storep = p;
            return QURL_OK;
        }
    }

    if (opt == QURL_URL_OPT_HOST)
    {
        if (0 == qurl_strlen(newp) && (flags & QURL_URL_NO_AUTHORITY))
        {
            /* Skip hostname check, it's allowed to be empty. */
        }
        else
        {
            if (url_hostname_check(uurl_ptr, (char *)newp))
            {
                url_free(newp);
                return QURL_ECODE_URL_MALFORMED_INPUT;
            }
        }
    }

    url_free(*storep);
    *storep = (char *)newp;

    if (port) /*If it is a port field, this also requires writing the numerical form portnum*/
    {
        uurl_ptr->portnum = port;
    }

    return QURL_OK;
}

/**
 * @brief Determine if the given URL is an absolute path (rather than a relative path), and return the scheme (lowercase) (protocol) of that URL within the specified buffer size.
 */
qurl_bit_t qurl_url_is_absolute(const char *url_ptr, char *buf_ptr, qosa_uint16_t buf_len)
{
    qosa_uint16_t i = 0;

    for (i = 0; i < buf_len && url_ptr[i]; ++i)
    {
        char s = url_ptr[i];
        if ((s == ':') && (url_ptr[i + 1] == '/'))
        {
            if (buf_ptr)
            {
                buf_ptr[i] = 0;
            }
            return QOSA_TRUE;
        }
        /* RFC 3986 3.1 explains:
		  scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
		*/
        else if (qurl_ctype_isalnum(s) || (s == '+') || (s == '-') || (s == '.'))
        {
            if (buf_ptr)
            {
                buf_ptr[i] = (char)qurl_tolower((int)s);
            }
        }
        else
        {
            break;
        }
    }
    return QOSA_FALSE;
}
