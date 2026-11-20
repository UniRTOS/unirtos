#include <common/bk_include.h>
#include "volc_memory.h"
#include "zlib.h"

void * _zcalloc(voidpf opaque, unsigned items, unsigned size) {
    (void)opaque;
    return volc_calloc(items, size);
}

void  _zcfree(voidpf opaque, voidpf ptr) {
    (void)opaque;
    volc_free(ptr);
}

int volc_unzip(const char * src, int src_size, char * dst, int dst_size) {
    int ret = Z_OK;
    z_stream strm ;

    /* allocate inflate state */
    strm.zalloc = _zcalloc;
    strm.zfree = _zcfree;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        return -1;
    }

    strm.avail_in = (uInt)src_size;
    strm.next_in = (Bytef*)src;

    strm.avail_out = dst_size;
    strm.next_out = (Bytef*)dst;
    ret = inflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        inflateEnd(&strm);
        if (ret == Z_OK || ret == Z_BUF_ERROR) {
            // buffer too small
            return -2;
        } else {
            // unzip failed
            return -1;
        }
    }
    int size = dst_size - strm.avail_out;
    /* clean up and return */
    (void)inflateEnd(&strm);
    return size;
}

int volc_zip(const char * src, int src_size, char * dst, int dst_size) {
    int ret = Z_OK;
    z_stream strm;
    /* allocate deflate state */
    strm.zalloc = _zcalloc;
    strm.zfree = _zcfree;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
        return -1;

    strm.avail_in = (uInt)src_size;
    strm.next_in = (Bytef*)src;

    strm.avail_out = dst_size;
    strm.next_out = (Bytef*)dst;

    ret = deflate(&strm, Z_FINISH);    /* no bad return value */
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        return -1;
    }
    int size = dst_size - strm.avail_out;
    // pIceMessageProcessor->unzipBuffer[size] = 0;
    /* clean up and return */
    (void)deflateEnd(&strm);
    return size;
}
