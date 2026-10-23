       
       
       
       
       
       
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
__MSP_STACK_SIZE = (4 << 10);
__MIN_HEAP_SIZE = (32 << 10);
__MPU_PROTECT_SIZE = 0x20;
__AP_APP_IRAM_OFFSET = 0x20000000;
__AP_APP_IRAM_BASE = 0x2802B000 - __AP_APP_IRAM_OFFSET;
__AP_APP_IRAM_SIZE = 0x00043C00;
MEMORY
{
  FLASH (rx) : ORIGIN = ((0x02000000 + (0)) + ((0x00176000) / 34 * 32)), LENGTH = ((0x002ca000) / 34 * 32)
  IRAM (rx) : ORIGIN = __AP_APP_IRAM_BASE, LENGTH = __AP_APP_IRAM_SIZE
  SPINLOCK (rwx) : ORIGIN = 0x28000000, LENGTH = 0x00010000
  RAM (rwx) : ORIGIN = 0x2802B000, LENGTH = 0x00043C00
  SWAP (rwx) : ORIGIN = 0x2809F800, LENGTH = 0x00000800
  ITCM0 (rwx) : ORIGIN = (0x00000000 + (0)) + __MPU_PROTECT_SIZE, LENGTH = 0x4000 - __MPU_PROTECT_SIZE
  DTCM0 (rwx) : ORIGIN = (0x20000000 + (0)) + (4), LENGTH = 0x4000 - (4)
  ITCM1 (rwx) : ORIGIN = (0x00000000 + (0)) + __MPU_PROTECT_SIZE, LENGTH = 0x4000 - __MPU_PROTECT_SIZE
  DTCM1 (rwx) : ORIGIN = (0x20000000 + (0)) + (4), LENGTH = 0x4000 - (4)
  ITCM2 (rwx) : ORIGIN = (0x00000000 + (0)) + __MPU_PROTECT_SIZE, LENGTH = 0x4000 - __MPU_PROTECT_SIZE
  DTCM2 (rwx) : ORIGIN = (0x20000000 + (0)) + (4), LENGTH = 0x4000 - (4)
  PSRAM_SLAB_USER (rwx) : ORIGIN = 0x60000000, LENGTH = 0x00019000
  PSRAM_SLAB_AUDIO (rwx) : ORIGIN = 0x60019000, LENGTH = 0x00019000
  PSRAM_SLAB_ENCODE (rwx) : ORIGIN = 0x60032000, LENGTH = 0x0015E000
  PSRAM_SLAB_DISPLAY (rwx) : ORIGIN = 0x60190000, LENGTH = 0x00570000
  PSRAM_HEAP (rwx) : ORIGIN = 0x60720000, LENGTH = 0x002E0000
  PSRAM_SECTION (rwx) : ORIGIN = 0x60A00000, LENGTH = 0x00600000
}
ENTRY(Reset_Handler_Cpu0)
SECTIONS
{
    ASSERT((. == ALIGN(512)), "vector table address align fault.")
    .vectors :
    {
        __vector_core0_table = .;
        KEEP(*(.vectors_core0))
        . = ALIGN(512);
        __vector_core1_table = .;
        KEEP(*(.vectors_core1))
        . = ALIGN(512);
        __vector_core2_table = .;
        KEEP(*(.vectors_core2))
        . = ALIGN(512);
    } > FLASH
    .gnu.sgstubs ALIGN(32) : ALIGN(32)
    {
        *(.gnu.sgstubs*)
        . = ALIGN(32);
    } > FLASH
    .text :
    {
        . = ALIGN(4);
        _stext = .;
        . = ALIGN(4);
        __devconfig_start = .;
        *(".devconfig.*")
        KEEP(*(SORT_BY_NAME(".devconfig*")))
        __devconfig_end = .;
        . = ALIGN(4);
        __apps_start = .;
        KEEP (*(.apps_data))
        __apps_end = .;
        . = ALIGN(4);
        __wifi_dp_code_start = .;
        *libiperf.a:*(.text*)
        *liblwip_intf_v2_1.a:*(.text*)
        *libbk_wifi_driver.a:*(.text*)
        *libbk_wifi.a:*(.text*)
        __wifi_dp_code_end = .;
        _etext = .;
        . = ALIGN(4);
    } > FLASH
    .a_device_null :
    {
      KEEP(*(.a_deviceobj_null))
    } > FLASH
    .a_devices :
    {
      __device_start = .;
      KEEP(*(.a_deviceobj_*))
      __device_end = .;
    } > FLASH
    .a_init_entries :
    {
      __a_init_start = .;
      KEEP(*(.a_init_entry_*))
      __a_init_end = .;
    } > FLASH
    .avdk_ipc :
    {
      __start_ipc_chan_reg = .;
      KEEP(*(.ipc_chan_reg*))
      __stop_ipc_chan_reg = .;
    } > FLASH
    .sensor_detect :
    {
      __camera_sensor_detect_array_start = .;
      KEEP(*(.camera_sensor_detect_function_list.*))
      __camera_sensor_detect_array_end = .;
    } > FLASH
    .tp_detect :
    {
      __tp_sensor_detect_array_start = .;
      KEEP(*(.tp_sensor_detect_function_list.*))
      __tp_sensor_detect_array_end = .;
    } > FLASH
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH
    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;
    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        LONG (__iram_flash_begin)
        LONG (__iram_start__)
        LONG ((__iram_end__ - __iram_start__) / 4)
        LONG (__data_flash_begin)
        LONG (__data_start__)
        LONG ((__data_end__ - __data_start__) / 4)
        LONG (__flash_psram_data)
        LONG (__psram_data_start__)
        LONG ((__psram_data_end__ - __psram_data_start__) / 4)
        LONG (__spinlock_begin)
        LONG (_spinlock_start)
        LONG ((_spinlock_end - _spinlock_start) / 4)
        __copy_table_end__ = .;
    } > FLASH
    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (_bss_start)
        LONG ((_bss_end - _bss_start) / 4)
        LONG (_heap_start)
        LONG ((_heap_end - _heap_start) / 4)
        LONG (__psram_bss_start__)
        LONG ((__psram_bss_end__ - __psram_bss_start__) / 4)
        __zero_table_end__ = .;
    } > FLASH
    .tcm.copy.table :
    {
        . = ALIGN(4);
        __tcm_copy_table_start__ = .;
        LONG (tcm_cpu0_core_id)
        LONG (__itcm_cpu0_text)
        LONG (__itcm_cpu0_start__)
        LONG ((__itcm_cpu0_end__ - __itcm_cpu0_start__) / 4)
        LONG (tcm_cpu0_core_id)
        LONG (__dtcm_cpu0_content)
        LONG (__dtcm_cpu0_start__)
        LONG ((__dtcm_cpu0_end__ - __dtcm_cpu0_start__) / 4)
        LONG (tcm_cpu1_core_id)
        LONG (__itcm_cpu1_text)
        LONG (__itcm_cpu1_start__)
        LONG ((__itcm_cpu1_end__ - __itcm_cpu1_start__) / 4)
        LONG (tcm_cpu1_core_id)
        LONG (__dtcm_cpu1_content)
        LONG (__dtcm_cpu1_start__)
        LONG ((__dtcm_cpu1_end__ - __dtcm_cpu1_start__) / 4)
        LONG (tcm_cpu2_core_id)
        LONG (__itcm_cpu2_text)
        LONG (__itcm_cpu2_start__)
        LONG ((__itcm_cpu2_end__ - __itcm_cpu2_start__) / 4)
        LONG (tcm_cpu2_core_id)
        LONG (__dtcm_cpu2_content)
        LONG (__dtcm_cpu2_start__)
        LONG ((__dtcm_cpu2_end__ - __dtcm_cpu2_start__) / 4)
        __tcm_copy_table_end__ = .;
    } > FLASH
 tcm_cpu0_core_id = (0);
    .itcm_cpu0 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu0_text = LOADADDR(.itcm_cpu0));
        __itcm_cpu0_start__ = .;
        KEEP(*(.null_trap_handler))
        *(.itcm_cpu0)
        . = ALIGN(4);
        __itcm_cpu0_end__ = .;
    } > ITCM0 AT > FLASH
    .dtcm_cpu0 :
    {
        . = ALIGN(4);
        PROVIDE(__dtcm_cpu0_content = LOADADDR(.dtcm_cpu0));
        __dtcm_cpu0_start__ = .;
        *(.dtcm_cpu0)
        . = ALIGN(4);
        __dtcm_cpu0_end__ = .;
    } > DTCM0 AT > FLASH
 tcm_cpu1_core_id = (1);
    .itcm_cpu1 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu1_text = LOADADDR(.itcm_cpu1));
        __itcm_cpu1_start__ = .;
        KEEP(*(.null_trap_handler))
        *(.itcm_cpu1)
        . = ALIGN(4);
        __itcm_cpu1_end__ = .;
    } > ITCM1 AT > FLASH
    .dtcm_cpu1 :
    {
        . = ALIGN(4);
        PROVIDE(__dtcm_cpu1_content = LOADADDR(.dtcm_cpu1));
        __dtcm_cpu1_start__ = .;
        *(.dtcm_cpu1)
        . = ALIGN(4);
        __dtcm_cpu1_end__ = .;
    } > DTCM1 AT > FLASH
 tcm_cpu2_core_id = (2);
    .itcm_cpu2 :
    {
        . = ALIGN(4);
        PROVIDE(__itcm_cpu2_text = LOADADDR(.itcm_cpu2));
        __itcm_cpu2_start__ = .;
        KEEP(*(.null_trap_handler))
        *(.itcm_cpu2)
        . = ALIGN(4);
        __itcm_cpu2_end__ = .;
    } > ITCM2 AT > FLASH
    .dtcm_cpu2 :
    {
        . = ALIGN(4);
        PROVIDE(__dtcm_cpu2_content = LOADADDR(.dtcm_cpu2));
        __dtcm_cpu2_start__ = .;
        *(.dtcm_cpu2)
        . = ALIGN(4);
        __dtcm_cpu2_end__ = .;
    } > DTCM2 AT > FLASH
    .spinlock :
    {
        . = ALIGN(4);
        PROVIDE(__spinlock_begin = LOADADDR(.spinlock));
        _spinlock_start = .;
        *(.sram_spinlock_section)
        . = ALIGN(4);
         _spinlock_end = .;
    } > SPINLOCK AT > FLASH
    .iram :
    {
        . = ALIGN(512);
        PROVIDE(__iram_flash_begin = LOADADDR(.iram));
        __iram_start__ = .;
        . = ALIGN(512);
        __vector_iram_table = .;
        KEEP(*(.vectors_iram))
        . = ALIGN(8);
        *(.itcm)
        *(.iram)
        *(.interrupt)
        . = ALIGN(4);
        __iram_end__ = .;
    } > IRAM AT > FLASH
    .ramcode :
    {
        . = . + SIZEOF(.iram);
        . = ALIGN(4);
    } > RAM AT > FLASH
    .data :
    {
        PROVIDE(__etext = LOADADDR(.data));
        PROVIDE(__data_flash_begin = LOADADDR(.data));
        . = ALIGN(4);
        __data_start__ = .;
        *(.dtcm)
        *(.dtcm_section*)
        *(.dtcm_sec_data*)
        *(.data)
        *(".data.*")
        *(.sdata)
        *(.gnu.linkonce.d*)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT > FLASH
    _data_flash_begin = __data_flash_begin;
    _data_ram_begin = __data_start__;
    _data_ram_end = .;
    s_rom_end = LOADADDR(.data) + SIZEOF(.data);
    .uninitialized (NOLOAD):
    {
        . = ALIGN(32);
        __uninitialized_start = .;
        *(.uninitialized)
        *(".uninitialized.*")
        KEEP(*(.keep.uninitialized))
        . = ALIGN(32);
        __uninitialized_end = .;
    } > RAM
    .bss (NOLOAD):
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM
    bss_size = _bss_end - _bss_start;
 _msp_total_size = __MSP_STACK_SIZE * 2;
    .heap (COPY) :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = . + (ORIGIN(RAM) + LENGTH(RAM) - _msp_total_size - _heap_start - 8);
        . = ALIGN(8);
        _heap_end = .;
    } > RAM
    .stack_cpu1 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE*2) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu1 = .;
        __StackLimitCpu1 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCpu1 = .;
        _estack_cpu1 = .;
    } > RAM
    .stack_cpu0 (ORIGIN(RAM) + LENGTH(RAM) - __MSP_STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);
        _sstack_cpu0 = .;
        __StackLimitCpu0 = .;
        . = . + __MSP_STACK_SIZE;
        . = ALIGN(8);
        __StackTopCpu0 = .;
        _estack_cpu0 = .;
    } > RAM
    .swap ORIGIN(SWAP):
    {
        . = ALIGN(8);
        _swap_start = .;
        * (.swap_data)
        * (.swap_data*)
        . = ALIGN(4);
        _swap_end = .;
    } > SWAP AT > FLASH
    .psram.data :
    {
        . = ALIGN(4);
        PROVIDE(__flash_psram_data = LOADADDR(.psram.data));
        __psram_data_start__ = .;
        *(.psram.data)
        *(.psram.data*)
        . = ALIGN(4);
        __psram_data_end__ = .;
    } > PSRAM_SECTION AT > FLASH
    .psram.bss (NOLOAD):
    {
        . = ALIGN(4);
        __psram_bss_start__ = .;
        *(.psram.bss)
        *(.psram.bss*)
        *(COMMON)
        . = ALIGN(4);
        __psram_bss_end__ = .;
    } > PSRAM_SECTION
    .LOAD_APP_TABLE :
    {
        . = ALIGN(4);
        PROVIDE( __quec_os_app_table_begin = LOADADDR(.LOAD_APP_TABLE));
        __quec_os_app_table_start = .;
        KEEP(*(.LOAD_APP_TABLE*))
        __quec_os_app_table_end = .;
        . = ALIGN(4);
    } > FLASH
    __quec_os_app_table_length = SIZEOF(.LOAD_APP_TABLE);
    .unirtos_app_init :
    {
        . = ALIGN(4);
        __unirtos_app_init_start = .;
        KEEP(*(.unirtos_app_init*))
        __unirtos_app_init_end = .;
        . = ALIGN(4);
    } > FLASH
    __MSPTop = ORIGIN(RAM) + LENGTH(RAM);
    __MSPLimit = __MSPTop - __MSP_STACK_SIZE;
    ASSERT((s_rom_end < ORIGIN(FLASH) + LENGTH(FLASH)), "ROM overflow!!!")
    ASSERT(((ORIGIN(RAM) + LENGTH(RAM)) > (_heap_start + __MSP_STACK_SIZE + 8)), "Stack overflowed with bss")
    ASSERT(((_heap_end - _heap_start) >= __MIN_HEAP_SIZE), "Heap smaller than minimize size 32K!!!")
}
