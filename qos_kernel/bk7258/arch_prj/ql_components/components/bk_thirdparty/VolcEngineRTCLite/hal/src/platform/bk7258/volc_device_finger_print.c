#include "volc_device_finger_print.h"
#include "volc_type.h"
#include <components/system.h>
#include "components/bk_uid.h"
#include <common/bk_include.h>
#include <stdio.h>


uint32_t volc_get_mac_address(volc_string_t* mac_addr) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    VOLC_CHK(NULL != mac_addr, VOLC_STATUS_INVALID_ARG);
    uint8_t hwaddr[6] = {0};
    bk_get_mac(hwaddr, MAC_TYPE_BASE);
    volc_string_snprintf(mac_addr,32,"%2X%2X%2X%2X%2X%2X",hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

err_out_label:
    return ret;
}

uint32_t volc_get_uid(volc_string_t* uid) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    VOLC_CHK(NULL != uid, VOLC_STATUS_INVALID_ARG);

    unsigned char bk_uid[32] = {0};
    char bk_uid_string[70] = {0};

    bk_uid_get_data(bk_uid);
    for (int i = 0; i < 32; i++)
    {
        sprintf(bk_uid_string + i * 2, "%02x", bk_uid[i]);
    }

    volc_string_snprintf(uid,64,"%s",bk_uid_string);

err_out_label:
   return ret;
}

uint32_t volc_get_device_finger_print(volc_string_t* finger) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    VOLC_CHK(NULL != finger, VOLC_STATUS_INVALID_ARG);
    volc_get_uid(finger);
err_out_label:
    return ret;
}

uint32_t volc_generate_device_id(volc_string_t* did) {
    uint32_t ret = VOLC_STATUS_SUCCESS;
    volc_string_t device_id;
    VOLC_CHK(NULL != did, VOLC_STATUS_INVALID_ARG);
    volc_string_init(&device_id);
    volc_get_uid(&device_id);
    volc_string_set(did, volc_string_get(&device_id));
    volc_string_deinit(&device_id);
err_out_label:
    return ret;
}