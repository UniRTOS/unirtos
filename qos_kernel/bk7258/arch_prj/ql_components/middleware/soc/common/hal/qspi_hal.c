// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "qspi_hal.h"
#include "qspi_ll.h"
#include <driver/hal/hal_qspi_types.h>

static uint32_t s_current_sw_op = 0;

bk_err_t qspi_hal_init(qspi_hal_t *hal)
{
	hal->hw = (qspi_hw_t *)QSPI_LL_REG_BASE(hal->id);
	qspi_ll_init(hal->hw);
	return BK_OK;
}

bool qspi_hal_is_cur_sw_op_write_data(void)
{
	return (s_current_sw_op == QSPI_WRITE);
}

bool qspi_hal_is_cur_sw_op_read_data(void)
{
	return (s_current_sw_op == QSPI_READ);
}

/* Program the address bytes (MSB first) into cmd2.. of the send (cmd_c) path. */
static bk_err_t qspi_hal_set_send_addr(qspi_hal_t *hal, uint32_t addr, uint32_t addr_len)
{
	if (addr_len == 0) {
		return BK_OK;
	}
	if (addr_len > 4) {
		return BK_FAIL;
	}

	uint32_t shift = (addr_len - 1) * 8;
	qspi_ll_set_cmd_c_cmd2(hal->hw, (addr >> shift) & QSPI_F_CMD_VALUE_M);
	if (addr_len > 1) {
		shift -= 8;
		qspi_ll_set_cmd_c_cmd3(hal->hw, (addr >> shift) & QSPI_F_CMD_VALUE_M);
	}
	if (addr_len > 2) {
		shift -= 8;
		qspi_ll_set_cmd_c_cmd4(hal->hw, (addr >> shift) & QSPI_F_CMD_VALUE_M);
	}
	if (addr_len > 3) {
		qspi_ll_set_cmd_c_cmd5(hal->hw, addr & QSPI_F_CMD_VALUE_M);
	}
	return BK_OK;
}

/* Set the per-byte line config for the send address; unused slots are disabled
 * so the controller stops the command phase right after the address. */
static bk_err_t qspi_hal_set_send_addr_line(qspi_hal_t *hal, qspi_wire_mode_t wire_mode, uint32_t addr_len)
{
	if (addr_len > 4) {
		return BK_FAIL;
	}

	qspi_ll_set_cmd_c_cmd2_line(hal->hw, addr_len > 0 ? wire_mode : QSPI_CMD_DISABLE);
	qspi_ll_set_cmd_c_cmd3_line(hal->hw, addr_len > 1 ? wire_mode : QSPI_CMD_DISABLE);
	qspi_ll_set_cmd_c_cmd4_line(hal->hw, addr_len > 2 ? wire_mode : QSPI_CMD_DISABLE);
	qspi_ll_set_cmd_c_cmd5_line(hal->hw, addr_len > 3 ? wire_mode : QSPI_CMD_DISABLE);
	qspi_ll_set_cmd_c_cmd6_line(hal->hw, QSPI_CMD_DISABLE);
	return BK_OK;
}

/* Program the address bytes (MSB first) into cmd2.. of the recv (cmd_d) path. */
static bk_err_t qspi_hal_set_recv_addr(qspi_hal_t *hal, uint32_t addr, uint32_t addr_len)
{
	if (addr_len == 0) {
		return BK_OK;
	}
	if (addr_len > 4) {
		return BK_FAIL;
	}

	uint32_t shift = (addr_len - 1) * 8;
	qspi_ll_set_cmd_d_cmd2(hal->hw, (addr >> shift) & QSPI_F_CMD_VALUE_M);
	if (addr_len > 1) {
		shift -= 8;
		qspi_ll_set_cmd_d_cmd3(hal->hw, (addr >> shift) & QSPI_F_CMD_VALUE_M);
	}
	if (addr_len > 2) {
		shift -= 8;
		qspi_ll_set_cmd_d_cmd4(hal->hw, (addr >> shift) & QSPI_F_CMD_VALUE_M);
	}
	if (addr_len > 3) {
		qspi_ll_set_cmd_d_cmd5(hal->hw, addr & QSPI_F_CMD_VALUE_M);
	}
	return BK_OK;
}

/* Set the per-byte line config for the recv address; unused slots are disabled. */
static bk_err_t qspi_hal_set_recv_addr_line(qspi_hal_t *hal, qspi_wire_mode_t wire_mode, uint32_t addr_len)
{
	if (addr_len > 4) {
		return BK_FAIL;
	}

	qspi_ll_set_cmd_d_cmd2_line(hal->hw, addr_len > 0 ? wire_mode : QSPI_CMD_DISABLE);
	qspi_ll_set_cmd_d_cmd3_line(hal->hw, addr_len > 1 ? wire_mode : QSPI_CMD_DISABLE);
	qspi_ll_set_cmd_d_cmd4_line(hal->hw, addr_len > 2 ? wire_mode : QSPI_CMD_DISABLE);
	qspi_ll_set_cmd_d_cmd5_line(hal->hw, addr_len > 3 ? wire_mode : QSPI_CMD_DISABLE);
	qspi_ll_set_cmd_d_cmd6_line(hal->hw, QSPI_CMD_DISABLE);
	return BK_OK;
}

/* Set a single recv command byte (value + line) at the given cmd slot index. */
static bk_err_t qspi_hal_set_recv_cmd_byte(qspi_hal_t *hal, uint32_t index, uint8_t value, qspi_wire_mode_t wire_mode)
{
	switch (index) {
	case 2: qspi_ll_set_cmd_d_cmd2(hal->hw, value); qspi_ll_set_cmd_d_cmd2_line(hal->hw, wire_mode); break;
	case 3: qspi_ll_set_cmd_d_cmd3(hal->hw, value); qspi_ll_set_cmd_d_cmd3_line(hal->hw, wire_mode); break;
	case 4: qspi_ll_set_cmd_d_cmd4(hal->hw, value); qspi_ll_set_cmd_d_cmd4_line(hal->hw, wire_mode); break;
	case 5: qspi_ll_set_cmd_d_cmd5(hal->hw, value); qspi_ll_set_cmd_d_cmd5_line(hal->hw, wire_mode); break;
	case 6: qspi_ll_set_cmd_d_cmd6(hal->hw, value); qspi_ll_set_cmd_d_cmd6_line(hal->hw, wire_mode); break;
	case 7: qspi_ll_set_cmd_d_cmd7(hal->hw, value); qspi_ll_set_cmd_d_cmd7_line(hal->hw, wire_mode); break;
	case 8: qspi_ll_set_cmd_d_cmd8(hal->hw, value); qspi_ll_set_cmd_d_cmd8_line(hal->hw, wire_mode); break;
	default: return BK_FAIL;
	}
	return BK_OK;
}

/*
 * Issue an indirect write command (cmd_c). The opcode is always single-wire,
 * the address follows at cmd->addr_wire_mode; write data goes through the
 * FIFO-fed data phase.
 */
static bk_err_t qspi_hal_send_command(qspi_hal_t *hal, const qspi_cmd_t *cmd)
{
	qspi_ll_set_cmd_c_l(hal->hw, 0);
	qspi_ll_set_cmd_c_h(hal->hw, 0);
	qspi_ll_set_cmd_c_cfg1(hal->hw, 0);
	qspi_ll_set_cmd_c_cfg2(hal->hw, 0);

	qspi_ll_set_cmd_c_cmd1(hal->hw, cmd->cmd);
	qspi_ll_set_cmd_c_cmd1_line(hal->hw, QSPI_1WIRE);
	BK_RETURN_ON_ERR(qspi_hal_set_send_addr(hal, cmd->addr, cmd->addr_len));
	BK_RETURN_ON_ERR(qspi_hal_set_send_addr_line(hal, cmd->addr_wire_mode, cmd->addr_len));

	qspi_ll_set_cmd_c_data_length(hal->hw, cmd->data_len);
	qspi_ll_set_cmd_c_data_line(hal->hw, cmd->data_wire_mode);
	qspi_ll_set_cmd_c_dummy_clock(hal->hw, cmd->dummy_cycle);
	qspi_ll_set_cmd_c_dummy_mode(hal->hw, cmd->dummy_mode);
	qspi_ll_cmd_c_start(hal->hw);
	qspi_ll_wait_cmd_done(hal->hw);
	return BK_OK;
}

/*
 * Issue an indirect read command (cmd_d). When the address itself is clocked on
 * four wires (NOR quad I/O read), one extra 4-wire mode byte is appended after
 * the address (cfg1 == 0xea8 for a 3-byte-address quad read).
 */
static bk_err_t qspi_hal_recv_command(qspi_hal_t *hal, const qspi_cmd_t *cmd)
{
	qspi_ll_set_cmd_d_l(hal->hw, 0);
	qspi_ll_set_cmd_d_h(hal->hw, 0);
	qspi_ll_set_cmd_d_cfg1(hal->hw, 0);
	qspi_ll_set_cmd_d_cfg2(hal->hw, 0);

	qspi_ll_set_cmd_d_cmd1(hal->hw, cmd->cmd);
	qspi_ll_set_cmd_d_cmd1_line(hal->hw, QSPI_1WIRE);
	BK_RETURN_ON_ERR(qspi_hal_set_recv_addr(hal, cmd->addr, cmd->addr_len));
	BK_RETURN_ON_ERR(qspi_hal_set_recv_addr_line(hal, cmd->addr_wire_mode, cmd->addr_len));

	if (cmd->addr_wire_mode == QSPI_4WIRE && cmd->addr_len > 0) {
		BK_RETURN_ON_ERR(qspi_hal_set_recv_cmd_byte(hal, 2 + cmd->addr_len, 0, QSPI_4WIRE));
	}

	qspi_ll_set_cmd_d_data_length(hal->hw, cmd->data_len);
	qspi_ll_set_cmd_d_data_line(hal->hw, cmd->data_wire_mode);
	qspi_ll_set_cmd_d_dummy_clock(hal->hw, cmd->dummy_cycle);
	qspi_ll_set_cmd_d_dummy_mode(hal->hw, cmd->dummy_mode);
	qspi_ll_cmd_d_start(hal->hw);
	qspi_ll_wait_cmd_done(hal->hw);
	return BK_OK;
}

bk_err_t qspi_hal_command(qspi_hal_t *hal, const qspi_cmd_t *cmd)
{
	s_current_sw_op = cmd->op;

	/* NOR indirect flash commands are built here via the generic send/recv
	 * path; PSRAM and memory-mapped commands still go through qspi_ll. */
	if (cmd->device == QSPI_FLASH && cmd->work_mode != MEMORY_MAPPED_MODE) {
		if (cmd->op == QSPI_WRITE) {
			return qspi_hal_send_command(hal, cmd);
		} else if (cmd->op == QSPI_READ) {
			return qspi_hal_recv_command(hal, cmd);
		}
		return BK_FAIL;
	}

	qspi_ll_init_command(hal->hw, cmd);
	return BK_OK;
}

bk_err_t qspi_hal_direct_write(qspi_id_t id, uint32_t base_addr, const void *data, uint32_t size)
{
	qspi_ll_direct_write(id, base_addr, data, size);
	return BK_OK;
}

bk_err_t qspi_hal_direct_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	qspi_ll_direct_read(id, base_addr, data, size);
	return BK_OK;
}

bk_err_t qspi_hal_io_write(qspi_hal_t *hal, const void *data, uint32_t size)
{
	qspi_ll_io_write(hal->hw, data, size);
	return BK_OK;
}

bk_err_t qspi_hal_io_read(qspi_hal_t *hal, void *data, uint32_t size)
{
	qspi_ll_io_read(hal->hw, data, size);
	return BK_OK;
}

/* qspi oled start */
bk_err_t qspi_hal_set_cmd_a_l(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_a_l(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_a_h(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_a_h(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_a_cfg1(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_a_cfg1(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_a_cfg2(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_a_cfg2(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_c_l(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_c_l(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_get_cmd_c_l(qspi_hal_t *hal)
{
	return qspi_ll_get_cmd_c_l(hal->hw);
}

bk_err_t qspi_hal_set_cmd_c_h(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_c_h(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_get_cmd_c_h(qspi_hal_t *hal)
{
	return qspi_ll_get_cmd_c_h(hal->hw);
}

bk_err_t qspi_hal_set_cmd_c_cfg1(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_c_cfg1(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_c_cfg2(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_c_cfg2(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_cmd_c_start(qspi_hal_t *hal)
{
	qspi_ll_cmd_c_start(hal->hw);
	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_d_l(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_d_l(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_get_cmd_d_l(qspi_hal_t *hal)
{
	return qspi_ll_get_cmd_d_l(hal->hw);
}

bk_err_t qspi_hal_set_cmd_d_h(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_d_h(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_get_cmd_d_h(qspi_hal_t *hal)
{
	return qspi_ll_get_cmd_d_h(hal->hw);
}

bk_err_t qspi_hal_set_cmd_d_cfg1(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_d_cfg1(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_d_cfg2(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_d_cfg2(hal->hw, cmd_value);
	return BK_OK;
}

bk_err_t qspi_hal_cmd_d_start(qspi_hal_t *hal)
{
	qspi_ll_cmd_d_start(hal->hw);
	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_d_data_line(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_d_data_line(hal->hw, cmd_value);

	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_d_data_length(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_d_data_length(hal->hw, cmd_value);

	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_d_dummy_clock(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_d_dummy_clock(hal->hw, cmd_value);

	return BK_OK;
}

bk_err_t qspi_hal_set_cmd_d_dummy_mode(qspi_hal_t *hal, uint32_t cmd_value)
{
	qspi_ll_set_cmd_d_dummy_mode(hal->hw, cmd_value);

	return BK_OK;
}

bk_err_t qspi_hal_wait_cmd_done(qspi_hal_t *hal)
{
	qspi_ll_wait_cmd_done(hal->hw);
	return BK_OK;
}

bk_err_t qspi_hal_force_spi_cs_low_enable(qspi_hal_t *hal)
{
	qspi_ll_force_spi_cs_low_enable(hal->hw);
	return BK_OK;
}

bk_err_t qspi_hal_force_spi_cs_low_disable(qspi_hal_t *hal)
{
	qspi_ll_force_spi_cs_low_disable(hal->hw);
	return BK_OK;
}

bk_err_t qspi_hal_disable_cmd_sck_enable(qspi_hal_t *hal)
{
	qspi_ll_disable_cmd_sck_enable(hal->hw);
	return BK_OK;
}

bk_err_t qspi_hal_disable_cmd_sck_disable(qspi_hal_t *hal)
{
	qspi_ll_disable_cmd_sck_disable(hal->hw);
	return BK_OK;
}

#if (CONFIG_SOC_BK7236XX)
bk_err_t qspi_hal_enable_soft_reset(qspi_hal_t *hal)
{
    qspi_ll_enable_soft_reset(hal->hw);

    return BK_OK;
}

bk_err_t qspi_hal_disable_soft_reset(qspi_hal_t *hal)
{
    qspi_ll_disable_soft_reset(hal->hw);
    return BK_OK;
}

bk_err_t qspi_hal_io_cpu_mem_select(qspi_hal_t *hal, uint32_t cmd_value)
{
    qspi_ll_select_io_cpu_mem(hal->hw, cmd_value);

    return BK_OK;
}

bk_err_t qspi_hal_set_cmd_last_addr(qspi_hal_t *hal, uint32_t cmd_value)
{
    qspi_ll_set_cmd_last_addr(hal->hw, cmd_value);

    return BK_OK;
}

bk_err_t qspi_hal_set_lcd_head_cmd0(qspi_hal_t *hal, uint32_t cmd_value)
{
    qspi_ll_set_lcd_head_cmd0(hal->hw, cmd_value);

    return BK_OK;
}

bk_err_t qspi_hal_set_lcd_head_cmd1(qspi_hal_t *hal, uint32_t cmd_value)
{
    qspi_ll_set_lcd_head_cmd1(hal->hw, cmd_value);

    return BK_OK;
}

bk_err_t qspi_hal_set_lcd_head_cmd2(qspi_hal_t *hal, uint32_t cmd_value)
{
    qspi_ll_set_lcd_head_cmd2(hal->hw, cmd_value);

    return BK_OK;
}

bk_err_t qspi_hal_set_lcd_head_cmd3(qspi_hal_t *hal, uint32_t cmd_value)
{
    qspi_ll_set_lcd_head_cmd3(hal->hw, cmd_value);

    return BK_OK;
}

bk_err_t qspi_hal_set_lcd_head_resolution(qspi_hal_t *hal, uint32_t hcnt_value, uint32_t vcnt_value)
{
    qspi_ll_set_lcd_head_hcnt(hal->hw, hcnt_value);
    qspi_ll_set_lcd_head_vcnt(hal->hw, vcnt_value);

    return BK_OK;
}

bk_err_t qspi_hal_clear_lcd_head(qspi_hal_t *hal, uint32_t enable)
{
    qspi_ll_clear_lcd_head(hal->hw, enable);

    return BK_OK;
}

bk_err_t qspi_hal_enable_lcd_head_selection_without_ram(qspi_hal_t *hal)
{
    qspi_ll_lcd_head_enable(hal->hw);

    return BK_OK;
}

bk_err_t qspi_hal_disable_lcd_head_selection_without_ram(qspi_hal_t *hal)
{
    qspi_ll_lcd_head_disable(hal->hw);

    return BK_OK;
}

bk_err_t qspi_hal_set_lcd_head_len(qspi_hal_t *hal, uint32_t length)
{
    qspi_ll_set_lcd_head_length(hal->hw, length);

    return BK_OK;
}

bk_err_t qspi_hal_set_lcd_head_dly(qspi_hal_t *hal, uint32_t delay_cnt)
{
    qspi_ll_set_lcd_head_interval(hal->hw, delay_cnt);

    return BK_OK;
}
#endif

/* qspi oled end */


