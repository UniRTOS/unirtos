#ifndef __HAL_VOLC_ZLIB_H__
#define __HAL_VOLC_ZLIB_H__

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

int volc_unzip(const char * src, int src_size, char * dst, int dst_size);
int volc_zip(const char * src, int src_size, char * dst, int dst_size);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_VOLC_UUID_H__ */