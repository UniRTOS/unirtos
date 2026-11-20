#include "cli.h"
#include "sdkconfig.h"

#if CONFIG_SOC_BK7236XX
#include "dwt.h"
extern void bk_delay_us(UINT32 us);
void smp_arch_dwt_trap_write(uint32_t addr, uint32_t data);
void smp_dwt_set_data_write(uint32_t addr);
void smp_arch_dwt_trap_disable(void);

static void dwt_command_usage(void)
{
    BK_LOGD(NULL, "dwtdw data_address\r\n");
    BK_LOGD(NULL, "     data_address: write this data address, hex format\r\n");
    BK_LOGD(NULL, "dwtdd data_address data_value\r\n");
    BK_LOGD(NULL, "     data_address: watch this data address, hex format\r\n");
    BK_LOGD(NULL, "     data_value: match the data value with watching this data address, hex format\r\n");
    BK_LOGD(NULL, "dwtf ms\r\n");
    BK_LOGD(NULL, "     ms: measure window in milliseconds using bk_delay_us()\r\n");
}

static void dwti_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t instruction_addr;

    if (2 != argc) {
        dwt_command_usage();
        return;
    }

    instruction_addr = strtoll(argv[1], NULL, 16);
    dwt_set_instruction_address(instruction_addr);
}

static void dwtdw_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t data_addr;

    if (2 != argc) {
        dwt_command_usage();
        return;
    }

    data_addr = strtoll(argv[1], NULL, 16);
    smp_dwt_set_data_write(data_addr);
}

static void dwtdd_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t addr, value;

    if (3 != argc) {
        dwt_command_usage();
        return;
    }

    addr = strtoll(argv[1], NULL, 16);
    value = strtoll(argv[2], NULL, 16);

    smp_arch_dwt_trap_write(addr, value);
}


static void dwtf_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t measure_ms;
    uint32_t start, end;
    uint32_t cycles;
    uint64_t freq_hz;
    uint32_t freq_mhz;

    if (2 != argc) {
        dwt_command_usage();
        return;
    }

    measure_ms = strtoul(argv[1], NULL, 10);
    if (measure_ms == 0U) {
        BK_LOGD(NULL, "invalid ms: %s\r\n", argv[1]);
        return;
    }

    dwt_init_cycle_counter();

    start = dwt_get_cycle_counter_val();
    bk_delay_us(measure_ms * 1000U);
    end = dwt_get_cycle_counter_val();

    cycles = end - start; /* unsigned handles wrap-around */

    /* freq_hz = cycles / (measure_ms/1000.0) = cycles * 1000 / measure_ms */
    freq_hz = ((uint64_t)cycles * 1000ULL) / (uint64_t)measure_ms;

    freq_mhz = (uint32_t)(freq_hz / 1000000ULL);
    BK_LOGD(NULL, "dwtf: ms=%u, cycles=%u, freq=%u MHz\r\n",
            measure_ms, cycles, freq_mhz);
}


#define DWT_CMD_CNT (sizeof(s_dwt_commands) / sizeof(struct cli_command))
static const struct cli_command s_dwt_commands[] = {
    {"dwtdw", "dwtdw data_address", dwtdw_Command},
    {"dwtdd", "dwtdd data_address data_value", dwtdd_Command},
    {"dwtf", "dwtf ms (measure CPU freq)", dwtf_Command},
};

int cli_dwt_init(void)
{
    return cli_register_commands(s_dwt_commands, DWT_CMD_CNT);
}
#endif // CONFIG_SOC_BK7236XX

