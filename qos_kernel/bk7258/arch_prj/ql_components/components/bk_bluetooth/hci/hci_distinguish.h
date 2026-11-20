#pragma once
#include <stdint.h>

enum
{
    HCI_CMD_TYPE_BT,
    HCI_CMD_TYPE_BLE,
    HCI_CMD_TYPE_MIX,
    HCI_CMD_TYPE_NOT_SURE,
    HCI_CMD_TYPE_ONLY_ONE,
    HCI_CMD_TYPE_COUNT,
};

uint8_t hci_cmd_get_type(uint16_t opcode);