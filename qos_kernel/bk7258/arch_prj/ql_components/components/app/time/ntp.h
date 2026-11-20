#ifndef _NTP_H_
#define _NTP_H_

/**
 * Set timezone offset in hours for NTP local time conversion.
 * e.g. 8 for UTC+8, -5 for UTC-5. Default is 8.
 */
void ntp_set_timezone(int timezone);

/**
 * Get current timezone offset in hours.
 */
int ntp_get_timezone(void);

/**
 * Get the UTC time from NTP server
 *
 * @note this function is not reentrant
 *
 * @return >0: success, current UTC time
 *         =0: get failed
 */
time_t ntp_get_time(char *host, uint32_t *frag_val);

/**
 * Get the local time from NTP server
 *
 * @return >0: success, current local time, offset by ntp timezone
 *         =0: get failed
 */
time_t ntp_get_local_time(uint32_t *frag_val);

/**
 * Sync current local time to RTC by NTP
 *
 * @return >0: success, current local time, offset by ntp timezone
 *         =0: sync failed
 */
time_t ntp_sync_to_rtc(void);

#endif /* _NTP_H_ */
