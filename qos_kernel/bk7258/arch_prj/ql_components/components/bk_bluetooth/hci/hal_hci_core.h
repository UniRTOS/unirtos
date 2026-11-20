#pragma once

#include <components/log.h>
#include "components/bluetooth/bk_dm_bluetooth.h"


int hal_hci_driver_open(void);
int hal_hci_driver_close(void);
int hal_hci_driver_secondary_controller_init(bk_bluetooth_secondary_callback_t *cb);
int hal_hci_driver_secondary_controller_deinit(void);
