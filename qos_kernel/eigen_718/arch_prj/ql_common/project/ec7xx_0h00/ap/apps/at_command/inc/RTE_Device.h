#ifndef __RTE_DEVICE_H
#define __RTE_DEVICE_H

#include "ec7xx.h"
#include "QosaPrjName.h"
#define RTE_CAN0_IO_MODE        IRQ_MODE

// CAN0 (Controller Area Network) [Driver_CAN0]
#define RTE_CAN0                        1

// { PAD_PIN50},  // 0 : agpio25  / 7: CAN0_RX
// { PAD_PIN51},  // 0 : agpio26  / 7: CAN0_TX
// { PAD_PIN52},  // 0 : agpio27  / 7: CAN0_STB
#if 0
#define RTE_CAN0_RX_BIT                 27
#define RTE_CAN0_RX_FUNC                PAD_MUX_ALT7

#define RTE_CAN0_TX_BIT                 28
#define RTE_CAN0_TX_FUNC                PAD_MUX_ALT7

#define RTE_CAN0_STB_BIT                29
#define RTE_CAN0_STB_FUNC               PAD_MUX_ALT7
#else

#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_CAN0_RX_BIT                 27
#else
#define RTE_CAN0_RX_BIT                 50
#endif
#define RTE_CAN0_RX_FUNC                PAD_MUX_ALT7

#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_CAN0_TX_BIT                 28
#else
#define RTE_CAN0_TX_BIT                 51
#endif
#define RTE_CAN0_TX_FUNC                PAD_MUX_ALT7

#define RTE_CAN0_STB_BIT                52
#define RTE_CAN0_STB_FUNC               PAD_MUX_ALT7
#endif

/*  Peripheral IO Mode Select, Must Configure First !!!
    Note, when receiver works in DMA_MODE, interrupt is also enabled to transfer tailing bytes.
*/

#define POLLING_MODE            0x1
#define DMA_MODE                0x2
#define IRQ_MODE                0x3
#define UNILOG_MODE             0x4

#define RTE_UART0_TX_IO_MODE    UNILOG_MODE

#ifdef TYPE_EC718M
#define RTE_UART0_RX_IO_MODE    IRQ_MODE  // Use IRQ_MODE instead of DMA_MODE coz we'll have no chance to enter psram Hybd sleep
#else
#define RTE_UART0_RX_IO_MODE    DMA_MODE
#define USART0_RX_TRIG_LVL      (30)
#endif

#define RTE_UART1_TX_IO_MODE    DMA_MODE
#define RTE_UART1_RX_IO_MODE    DMA_MODE

#define RTE_UART2_TX_IO_MODE    POLLING_MODE
#define RTE_UART2_RX_IO_MODE    DMA_MODE

#define RTE_UART3_TX_IO_MODE    DMA_MODE
#define RTE_UART3_RX_IO_MODE    DMA_MODE

#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_SPI0_IO_MODE        DMA_MODE
#define RTE_SPI1_IO_MODE        DMA_MODE
#else /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_SPI0_IO_MODE        POLLING_MODE
#define RTE_SPI1_IO_MODE        POLLING_MODE
#endif /* UNIRTOS_PLATFORM_MODIFY */

#define RTE_I2C0_IO_MODE        POLLING_MODE
#define RTE_I2C1_IO_MODE        POLLING_MODE

#ifdef UNIRTOS_PLATFORM_MODIFY
// CSPI0 Configuration
#define RTE_CSPI0                       1

// DMA CSPI0 Request ID
#define RTE_CSPI0_DMA_RX_REQID          DMA_REQUEST_USP0_RX

// CSPI1 Configuration
#define RTE_CSPI1                       1

// DMA CSPI1 Request ID
#define RTE_CSPI1_DMA_RX_REQID          DMA_REQUEST_USP1_RX


// LSPI2 Configuration
#define RTE_LSPI2                       1
#endif /* UNIRTOS_PLATFORM_MODIFY */

// I2C0 (Inter-integrated Circuit Interface) [Driver_I2C0]
// Configuration settings for Driver_I2C0 in component ::Drivers:I2C
#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_I2C0                        1
#else /* UNIRTOS_PLATFORM_MODIFY */
#if (defined MID_FEATURE_MODE) || (defined FEATURE_AUDIO_ENABLE)
#define RTE_I2C0                        1
#else
#define RTE_I2C0                        0
#endif
#endif /* UNIRTOS_PLATFORM_MODIFY */

// { PAD_PIN30},  // 0 : gpio15  / 2: I2C0_SCL
// { PAD_PIN29},  // 0 : gpio14  / 2: I2C0_SDA
#define RTE_I2C0_SCL_BIT                30
#define RTE_I2C0_SCL_FUNC               PAD_MUX_ALT2

#define RTE_I2C0_SDA_BIT                29
#define RTE_I2C0_SDA_FUNC               PAD_MUX_ALT2

// I2C1 (Inter-integrated Circuit Interface) [Driver_I2C1]
// Configuration settings for Driver_I2C1 in component ::Drivers:I2C
#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_I2C1                        1
#else /* UNIRTOS_PLATFORM_MODIFY */
#if (defined MID_FEATURE_MODE)
#define RTE_I2C1                        1
#else
#define RTE_I2C1                        0
#endif
#endif /* UNIRTOS_PLATFORM_MODIFY */


#ifdef UNIRTOS_PLATFORM_MODIFY
#ifdef QL_AP_LIB_IMS
// { PAD_PIN24},  // 0 : gpio9  / 2: I2C1_SCL
// { PAD_PIN23},  // 0 : gpio8  / 2: I2C1_SDA
#define RTE_I2C1_SCL_BIT                24
#define RTE_I2C1_SCL_FUNC               PAD_MUX_ALT2

#define RTE_I2C1_SDA_BIT                23
#define RTE_I2C1_SDA_FUNC               PAD_MUX_ALT2
#else /* QL_AP_LIB_IMS */
// { PAD_PIN20},  // 0 : gpio5  / 2 : I2C1 SCL
// { PAD_PIN19},  // 0 : gpio4  / 2 : I2C1 SDA
#define RTE_I2C1_SCL_BIT                20
#define RTE_I2C1_SCL_FUNC               PAD_MUX_ALT2

#define RTE_I2C1_SDA_BIT                19
#define RTE_I2C1_SDA_FUNC               PAD_MUX_ALT2
#endif /* QL_AP_LIB_IMS */
#else /* UNIRTOS_PLATFORM_MODIFY */
// { PAD_PIN20},  // 0 : gpio5  / 2 : I2C1 SCL
// { PAD_PIN19},  // 0 : gpio4  / 2 : I2C1 SDA
#define RTE_I2C1_SCL_BIT                20
#define RTE_I2C1_SCL_FUNC               PAD_MUX_ALT2

#define RTE_I2C1_SDA_BIT                19
#define RTE_I2C1_SDA_FUNC               PAD_MUX_ALT2
#endif /* UNIRTOS_PLATFORM_MODIFY */

// UART0 (Universal asynchronous receiver transmitter) [Driver_USART0]
// Configuration settings for Driver_USART0 in component ::Drivers:USART
#define RTE_UART0                       1
#define RTE_UART0_CTS_PIN_EN            0
#define RTE_UART0_RTS_PIN_EN            0

#if defined CHIP_EC718
// { PAD_PIN42},  // 0 : gpio36 / 3 : UART0 RTSn
// { PAD_PIN43},  // 0 : gpio37 / 3 : UART0 CTSn
// { PAD_PIN31},  // 0 : gpio16 / 1 : UART0 RXD
// { PAD_PIN32},  // 0 : gpio17 / 1 : UART0 TXD
#define RTE_UART0_RTS_BIT               42
#define RTE_UART0_RTS_FUNC              PAD_MUX_ALT3

#define RTE_UART0_CTS_BIT               43
#define RTE_UART0_CTS_FUNC              PAD_MUX_ALT3

#define RTE_UART0_RX_BIT                31
#define RTE_UART0_RX_FUNC               PAD_MUX_ALT1

#define RTE_UART0_TX_BIT                32
#define RTE_UART0_TX_FUNC               PAD_MUX_ALT1
#elif defined CHIP_EC716
// { PAD_PIN14},  // 0 : gpio2 / 5 : UART0 RTSn
// { PAD_PIN15},  // 0 : gpio3 / 5 : UART0 CTSn
// { PAD_PIN18},  // 0 : gpio6 / 1 : UART0 RXD
// { PAD_PIN19},  // 0 : gpio7 / 1 : UART0 TXD
#define RTE_UART0_RTS_BIT               14
#define RTE_UART0_RTS_FUNC              PAD_MUX_ALT5

#define RTE_UART0_CTS_BIT               15
#define RTE_UART0_CTS_FUNC              PAD_MUX_ALT5

#define RTE_UART0_RX_BIT                18
#define RTE_UART0_RX_FUNC               PAD_MUX_ALT1

#define RTE_UART0_TX_BIT                19
#define RTE_UART0_TX_FUNC               PAD_MUX_ALT1
#endif
// DMA
//  Tx
//    Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART0_DMA_TX_REQID          DMA_REQUEST_USART0_TX
//  Rx
//    Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART0_DMA_RX_REQID          DMA_REQUEST_USART0_RX

// UART1 (Universal asynchronous receiver transmitter) [Driver_USART1]
// Configuration settings for Driver_USART1 in component ::Drivers:USART
#define RTE_UART1                       1
#define RTE_UART1_CTS_PIN_EN            1
#define RTE_UART1_RTS_PIN_EN            1

#if defined CHIP_EC718
// { PAD_PIN27},  // 0 : gpio12  / 2 : UART1 RTS
// { PAD_PIN28},  // 0 : gpio13  / 2 : UART1 CTS
// { PAD_PIN33},  // 0 : gpio18  / 1 : UART1 RXD
// { PAD_PIN34},  // 0 : gpio19  / 1 : UART1 TXD
#ifdef UNIRTOS_COMMON_MODIFY
#define RTE_UART1_RTS_BIT               16
#define RTE_UART1_RTS_FUNC              PAD_MUX_ALT3
#define RTE_UART1_CTS_BIT               17
#define RTE_UART1_CTS_FUNC              PAD_MUX_ALT3
#else /* UNIRTOS_COMMON_MODIFY */
#define RTE_UART1_RTS_BIT               27
#define RTE_UART1_RTS_FUNC              PAD_MUX_ALT2

#define RTE_UART1_CTS_BIT               28
#define RTE_UART1_CTS_FUNC              PAD_MUX_ALT2
#endif /* UNIRTOS_COMMON_MODIFY */

#define RTE_UART1_RX_BIT                33
#define RTE_UART1_RX_FUNC               PAD_MUX_ALT1

#define RTE_UART1_TX_BIT                34
#define RTE_UART1_TX_FUNC               PAD_MUX_ALT1
#elif defined CHIP_EC716
// { PAD_PIN16},  // 0 : gpio4  / 5 : UART1 RTS
// { PAD_PIN17},  // 0 : gpio5  / 5 : UART1 CTS
// { PAD_PIN20},  // 0 : gpio8  / 1 : UART1 RXD
// { PAD_PIN21},  // 0 : gpio9  / 1 : UART1 TXD
#ifdef UNIRTOS_COMMON_MODIFY
#define RTE_UART1_RTS_BIT               11
#else /* UNIRTOS_COMMON_MODIFY */
#define RTE_UART1_RTS_BIT               16
#endif /* UNIRTOS_COMMON_MODIFY */
#define RTE_UART1_RTS_FUNC              PAD_MUX_ALT5

#ifdef UNIRTOS_COMMON_MODIFY
#define RTE_UART1_CTS_BIT               10
#else /* UNIRTOS_COMMON_MODIFY */
#define RTE_UART1_CTS_BIT               17
#endif /* UNIRTOS_COMMON_MODIFY */
#define RTE_UART1_CTS_FUNC              PAD_MUX_ALT5

#define RTE_UART1_RX_BIT                20
#define RTE_UART1_RX_FUNC               PAD_MUX_ALT1

#define RTE_UART1_TX_BIT                21
#define RTE_UART1_TX_FUNC               PAD_MUX_ALT1
#endif
// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART1_DMA_TX_REQID          DMA_REQUEST_USART1_TX
//   Rx
//     Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART1_DMA_RX_REQID          DMA_REQUEST_USART1_RX

// UART2 (Universal asynchronous receiver transmitter) [Driver_USART2]
// Configuration settings for Driver_USART2 in component ::Drivers:USART
#define RTE_UART2                       0

// { PAD_PIN25},  // 0 : gpio10  / 3 : UART2 RXD
// { PAD_PIN26},  // 0 : gpio11  / 3 : UART2 TXD
// quec add 
#if defined(__QUECTEL_PROJECT_EG800ZCN_LG__) || defined(__QUECTEL_PROJECT_EG800ZEU_LG__)
#define RTE_UART2_RX_BIT                21
#define RTE_UART2_RX_FUNC               PAD_MUX_ALT2

#define RTE_UART2_TX_BIT                22
#define RTE_UART2_TX_FUNC               PAD_MUX_ALT2

#else  /* defined(__QUECTEL_PROJECT_EG800ZCN_LG__) || defined(__QUECTEL_PROJECT_EG800ZEU_LG__) */
#define RTE_UART2_RX_BIT                25
#define RTE_UART2_RX_FUNC               PAD_MUX_ALT3

#define RTE_UART2_TX_BIT                26
#define RTE_UART2_TX_FUNC               PAD_MUX_ALT3
#endif /* defined(__QUECTEL_PROJECT_EG800ZCN_LG__) || defined(__QUECTEL_PROJECT_EG800ZEU_LG__) */

#define RTE_UART3_RX_BIT                29
#define RTE_UART3_RX_FUNC               PAD_MUX_ALT3

#define RTE_UART3_TX_BIT                30
#define RTE_UART3_TX_FUNC               PAD_MUX_ALT3

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART2_DMA_TX_REQID          DMA_REQUEST_USART2_TX
//   Rx
//     Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART2_DMA_RX_REQID          DMA_REQUEST_USART2_RX

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART3_DMA_TX_REQID          DMA_REQUEST_USART3_TX
//   Rx
//     Channel    <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_UART3_DMA_RX_REQID          DMA_REQUEST_USART3_RX

// SPI0 (Serial Peripheral Interface) [Driver_SPI0]
// Configuration settings for Driver_SPI0 in component ::Drivers:SPI
#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_SPI0                        1
#else /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_SPI0                        0
#endif /* UNIRTOS_PLATFORM_MODIFY */

// { PAD_PIN23},  // 0 : gpio8  / 1 : SPI0 SSn
// { PAD_PIN24},  // 0 : gpio9  / 1 : SPI0 MOSI
// { PAD_PIN25},  // 0 : gpio10 / 1 : SPI0 MISO
// { PAD_PIN26},  // 0 : gpio11 / 1 : SPI0 SCLK
#define RTE_SPI0_SSN_BIT                23
#define RTE_SPI0_SSN_FUNC               PAD_MUX_ALT1

#define RTE_SPI0_MOSI_BIT               24
#define RTE_SPI0_MOSI_FUNC              PAD_MUX_ALT1

#define RTE_SPI0_MISO_BIT               25
#define RTE_SPI0_MISO_FUNC              PAD_MUX_ALT1

#define RTE_SPI0_SCLK_BIT               26
#define RTE_SPI0_SCLK_FUNC              PAD_MUX_ALT1

#define RTE_SPI0_SSN_GPIO_INSTANCE      0
#define RTE_SPI0_SSN_GPIO_INDEX         8

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI0_DMA_TX_REQID           DMA_REQUEST_SPI0_TX

//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI0_DMA_RX_REQID           DMA_REQUEST_SPI0_RX

// SPI1 (Serial Peripheral Interface) [Driver_SPI1]
// Configuration settings for Driver_SPI1 in component ::Drivers:SPI
#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_SPI1                        1
#else /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_SPI1                        0
#endif /* UNIRTOS_PLATFORM_MODIFY */

// { PAD_PIN27},  // 0 : gpio12  / 1 : SPI1 SSn
// { PAD_PIN28},  // 0 : gpio13  / 1 : SPI1 MOSI
// { PAD_PIN29},  // 0 : gpio14  / 1 : SPI1 MISO
// { PAD_PIN30},  // 0 : gpio15  / 1 : SPI1 SCLK
// { PAD_PIN26},  // 0 : gpio11  / 2 : SPI1 SSn1
#define RTE_SPI1_SSN_BIT                27
#define RTE_SPI1_SSN_FUNC               PAD_MUX_ALT1

#define RTE_SPI1_MOSI_BIT               28
#define RTE_SPI1_MOSI_FUNC              PAD_MUX_ALT1

#define RTE_SPI1_MISO_BIT               29
#define RTE_SPI1_MISO_FUNC              PAD_MUX_ALT1

#define RTE_SPI1_SCLK_BIT               30
#define RTE_SPI1_SCLK_FUNC              PAD_MUX_ALT1

#define RTE_SPI1_SSN_GPIO_INSTANCE      0
#define RTE_SPI1_SSN_GPIO_INDEX         12

#define RTE_SPI1_SSN1_BIT               26
#define RTE_SPI1_SSN1_FUNC              PAD_MUX_ALT2

// DMA
//   Tx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI1_DMA_TX_REQID           DMA_REQUEST_SPI1_TX

//   Rx
//     Channel     <0=>0 <1=>1 <2=>2 <3=>3 <4=>4 <5=>5 <6=>6 <7=>7
#define RTE_SPI1_DMA_RX_REQID           DMA_REQUEST_SPI1_RX


// PWM0 Controller [Driver_PWM0]
// Configuration settings for Driver_PWM0 in component ::Drivers:PWM
#define RTE_PWM                         1

#define EFUSE_INIT_MODE POLLING_MODE
#define L2CTLS_INIT_MODE POLLING_MODE

#define FLASH_BARE_RW_MODE 1




/*
static configuration for USB/UART relatded feature
    RTE_USB_EN:      whether init USB stack
    RTE_ONE_UART_AT: enable one UART AT
    RTE_ONE_UART_AT: enable two UART AT
    RTE_ETHER_EN   : whehter RNDIS/ECM feature is enabled
    RTE_PPP_EN     : whehter PPPOS feature is enabled
    RTE_OPAQ_EN    : whehter OPAQOS feature is enabled
*/
#if (defined OPEN_CPU_MODE)
/* device */
#define RTE_SPI_EN                      0
#define RTE_USB_EN                      1
#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_ONE_UART_AT                 0
#define RTE_TWO_UART_AT                 1
#else /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_ONE_UART_AT                 1
#define RTE_TWO_UART_AT                 0
#endif /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_SCOM_EN                     0
/* feature */
#define RTE_MUX_EN                      0
#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_OPAQ_EN                     1
#else /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_OPAQ_EN                     0
#endif /* UNIRTOS_PLATFORM_MODIFY */

#ifdef UNIRTOS_PLATFORM_MODIFY
#if (RTE_USB_EN == 1)
#define RTE_ETHER_EN                    1              /* 0/1: to ctrl eth(rndis/ecm) independently! */
#ifdef FEATURE_USB_CCID_ENABLE
#define RTE_CCID_EN                     1              /* 0/1: to ctrl ccid independently! */
#else
#define RTE_CCID_EN                     0              /* 0/1: to ctrl ccid independently! */
#endif
#else
#define RTE_ETHER_EN                    RTE_USB_EN     /* must be the same(disabled) */
#define RTE_CCID_EN                     RTE_USB_EN     /* 0/1: to ctrl ccid independently! */
#endif
#else /* UNIRTOS_PLATFORM_MODIFY */
#if ((defined MID_FEATURE_MODE) || (defined FEATURE_MOREROM_ENABLE)) || (defined FEATURE_IOTAT_ENABLE)
#define RTE_ETHER_EN                    0
#elif (defined FEATURE_MORERAM_ENABLE)
#ifdef TYPE_EC716E
#define RTE_ETHER_EN                    1
#else
#define RTE_ETHER_EN                    0
#endif
#else
#define RTE_ETHER_EN                    1
#endif
#endif /* UNIRTOS_PLATFORM_MODIFY */

#else
/* device */
#define RTE_SPI_EN                      0
#define RTE_USB_EN                      1
#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_ONE_UART_AT                 0
#define RTE_TWO_UART_AT                 1
#else /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_ONE_UART_AT                 1
#define RTE_TWO_UART_AT                 0
#endif /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_SCOM_EN                     0

/* feature */
#define RTE_MUX_EN                      0
#ifdef UNIRTOS_PLATFORM_MODIFY
#define RTE_OPAQ_EN                     1
#else /* UNIRTOS_PLATFORM_MODIFY */
#define RTE_OPAQ_EN                     0
#endif /* UNIRTOS_PLATFORM_MODIFY */
#if (RTE_USB_EN == 1)
#define RTE_ETHER_EN                    1              /* 0/1: to ctrl eth(rndis/ecm) independently! */
#ifdef FEATURE_USB_CCID_ENABLE
#define RTE_CCID_EN                     1              /* 0/1: to ctrl ccid independently! */
#else
#define RTE_CCID_EN                     0              /* 0/1: to ctrl ccid independently! */
#endif
#else
#define RTE_ETHER_EN                    RTE_USB_EN     /* must be the same(disabled) */
#define RTE_CCID_EN                     RTE_USB_EN     /* 0/1: to ctrl ccid independently! */
#endif

#endif

#ifdef FEATURE_PPP_ENABLE
#define RTE_PPP_EN                      1
#else
#define RTE_PPP_EN                      0
#endif

#if (RTE_SPI_EN == 1)
#define RTE_EOS_EN                      0
#define RTE_IPOS_EN                     0
#else
#define RTE_EOS_EN                      RTE_SPI_EN
#define RTE_IPOS_EN                     RTE_SPI_EN
#endif

#define RTE_ETH_LAN_EN                 (RTE_ETHER_EN || RTE_EOS_EN)

/* to be compatible with old style */
#define RTE_RNDIS_EN                    RTE_ETHER_EN

#if (defined FEATURE_AUDIO_ENABLE)
#define RTE_AUDIO_EN                    1
#define VOLTE_EC_OWN_BOARD_SUPPORT      1
#else
#define RTE_AUDIO_EN                    0
#endif

#if (RTE_IPOS_EN == 1)
#undef  RTE_OPAQ_EN
//To Ensure the macro 'RTE_OPAQ_EN' is enabled!
#define RTE_OPAQ_EN                     1
#endif

#if (RTE_ONE_UART_AT == 1 && RTE_TWO_UART_AT == 1)
#error "RTE_ONE_UART_AT & RTE_TWO_UART_AT can NOT be enabled at the same time!"

#else
#if ((RTE_ONE_UART_AT == 1) || (RTE_TWO_UART_AT == 1))
#undef  RTE_UART0
#define RTE_UART0                       1
#undef  RTE_UART1
#define RTE_UART1                       1
#undef  RTE_UART2
#undef  RTE_UART3
#define RTE_UART3                       1
#if (RTE_TWO_UART_AT == 1)
#define RTE_UART2                       1
#else
#define RTE_UART2                       0
#endif
#endif
#endif

/* to enable external thermal */
#define EXTERNAL_NTC_EXIST              0

#define RTE_LPUART_EN                   1

#if (RTE_UART1 == 1)
#if defined CHIP_EC716
#define UART1_DTR_PAD_INDEX             23     // GPIO11
#define UART1_DTR_GPIO_INSTANCE         0
#define UART1_DTR_GPIO_PIN              11

#define UART1_RI_PAD_INDEX              26     // AONIO 4 = GPIO14
#define UART1_RI_GPIO_INSTANCE          0
#define UART1_RI_GPIO_PIN               14
#define UART1_RI_PWM_INSTANCE           5
#define UART1_RI_PWM_CLK_ID             FCLK_TIMER5
#define UART1_RI_PWM_CLK_SEL            FCLK_TIMER5_SEL_26M

#define UART1_DCD_PAD_INDEX             27     // AONIO 5 = GPIO15
#define UART1_DCD_GPIO_INSTANCE         0
#define UART1_DCD_GPIO_PIN              15
#else
#define UART1_DTR_PAD_INDEX             26     // GPIO11
#define UART1_DTR_GPIO_INSTANCE         0
#define UART1_DTR_GPIO_PIN              11

#define UART1_RI_PAD_INDEX              49     // AONIO 4 = GPIO24
#define UART1_RI_GPIO_INSTANCE          1
#define UART1_RI_GPIO_PIN               8
#define UART1_RI_PWM_INSTANCE           1
#define UART1_RI_PWM_CLK_ID             FCLK_TIMER1
#define UART1_RI_PWM_CLK_SEL            FCLK_TIMER1_SEL_26M

#define UART1_DCD_PAD_INDEX             50     // AONIO 5 = GPIO25
#define UART1_DCD_GPIO_INSTANCE         1
#define UART1_DCD_GPIO_PIN              9
#endif
#endif

#if (RTE_UART2 == 1)
#define UART2_DTR_PAD_INDEX             25     // GPIO10
#define UART2_DTR_GPIO_INSTANCE         0
#define UART2_DTR_GPIO_PIN              10

#define UART2_RI_PAD_INDEX              48     // AONIO 3 = GPIO23
#define UART2_RI_GPIO_INSTANCE          1
#define UART2_RI_GPIO_PIN               7
#define UART2_RI_PWM_INSTANCE           0
#define UART2_RI_PWM_CLK_ID             FCLK_TIMER0
#define UART2_RI_PWM_CLK_SEL            FCLK_TIMER0_SEL_26M

#define UART2_DCD_PAD_INDEX             52    // AONIO 7 = GPIO27
#define UART2_DCD_GPIO_INSTANCE         1
#define UART2_DCD_GPIO_PIN              11
#endif


#define NETLIGHT_PAD_INDEX              51     // AONIO 6 = GPIO26
#define NETLIGHT_PAD_ALT_FUNC           PAD_MUX_ALT5
#define NETLIGHT_PWM_INSTANCE           3

// //USIM1 OPTION1
// #if defined CHIP_EC718
// #define USIM1_URST_OP1_PAD_INDEX             19     // GPIO4
// #define USIM1_URST_OP1_GPIO_INSTANCE         0
// #define USIM1_URST_OP1_GPIO_PIN              4
// #define USIM1_UCLK_OP1_PAD_INDEX             20     // GPIO5
// #define USIM1_UCLK_OP1_GPIO_INSTANCE         0
// #define USIM1_UCLK_OP1_GPIO_PIN              5
// #define USIM1_UIO_OP1_PAD_INDEX              21     // GPIO6
// #define USIM1_UIO_OP1_GPIO_INSTANCE          0
// #define USIM1_UIO_OP1_GPIO_PIN               6
// #elif defined CHIP_EC716
// #define USIM1_URST_OP1_PAD_INDEX             13     // GPIO1
// #define USIM1_URST_OP1_GPIO_INSTANCE         0
// #define USIM1_URST_OP1_GPIO_PIN              1
// #define USIM1_UCLK_OP1_PAD_INDEX             14     // GPIO2
// #define USIM1_UCLK_OP1_GPIO_INSTANCE         0
// #define USIM1_UCLK_OP1_GPIO_PIN              2
// #define USIM1_UIO_OP1_PAD_INDEX              15     // GPIO3
// #define USIM1_UIO_OP1_GPIO_INSTANCE          0
// #define USIM1_UIO_OP1_GPIO_PIN               3
// #endif
#ifdef UNIRTOS_PLATFORM_MODIFY

// 获取USIM1_URST_OP1_PAD_INDEX的值
int get_usim1_urst_op1_pad_index(void);
// 获取USIM1_URST_OP1_GPIO_INSTANCE的值
int get_usim1_urst_op1_gpio_instance(void);
// 获取USIM1_URST_OP1_GPIO_PIN的值
int get_usim1_urst_op1_gpio_pin(void);
// 获取USIM1_UCLK_OP1_PAD_INDEX的值
int get_usim1_uclk_op1_pad_index(void);
// 获取USIM1_UCLK_OP1_GPIO_INSTANCE的值
int get_usim1_uclk_op1_gpio_instance(void);
// 获取USIM1_UCLK_OP1_GPIO_PIN的值
int get_usim1_uclk_op1_gpio_pin(void);
// 获取USIM1_UIO_OP1_PAD_INDEX的值
int get_usim1_uio_op1_pad_index(void);
// 获取USIM1_UIO_OP1_GPIO_INSTANCE的值
int get_usim1_uio_op1_gpio_instance(void);
// 获取USIM1_UIO_OP1_GPIO_PIN的值
int get_usim1_uio_op1_gpio_pin(void);

#endif /* #ifdef UNIRTOS_PLATFORM_MODIFY */

//USIM1 OPTION2
#define USIM1_UIO_OP2_PAD_INDEX              27     // GPIO12
#define USIM1_UIO_OP2_GPIO_INSTANCE          0
#define USIM1_UIO_OP2_GPIO_PIN               12
#define USIM1_URST_OP2_PAD_INDEX             28     // GPIO13
#define USIM1_URST_OP2_GPIO_INSTANCE         0
#define USIM1_URST_OP2_GPIO_PIN              13
#define USIM1_UCLK_OP2_PAD_INDEX             29     // GPIO14
#define USIM1_UCLK_OP2_GPIO_INSTANCE         0
#define USIM1_UCLK_OP2_GPIO_PIN              14

//USIM1 clock latched by AONIO, for example, use AONIO-6 test on EVB
#if defined CHIP_EC718
#define AONIO_6_PAD_INDEX        51     // AONIO 6 = GPIO26
#define AONIO_6_GPIO_INSTANCE    1
#define AONIO_6_GPIO_PIN         10
#elif defined CHIP_EC716
#define AONIO_6_PAD_INDEX        28     // AONIO 6 = GPIO16
#define AONIO_6_GPIO_INSTANCE    1
#define AONIO_6_GPIO_PIN         0 //GPIO16, (16 % 16)
#endif

#ifdef SIM_HOT_SWAP_FEATURE
#define TIMER_INSTANCE_4           4
#endif


//////////////////////////////////////////////////////////////////////////////////////////////
// I2S Setting field Start
// All the I2S's parameters that need user to set are all put here
//////////////////////////////////////////////////////////////////////////////////////////////
// I2S0 Configuration
#define RTE_I2S0                        1

#define RTE_I2S0_MCLK_PAD_ADDR          39
#define RTE_I2S0_MCLK_FUNC              PAD_MUX_ALT1

#define RTE_I2S0_BCLK_PAD_ADDR          35
#define RTE_I2S0_BCLK_FUNC              PAD_MUX_ALT1

#define RTE_I2S0_LRCK_PAD_ADDR          36
#define RTE_I2S0_LRCK_FUNC              PAD_MUX_ALT1

#define RTE_I2S0_DIN_PAD_ADDR           37
#define RTE_I2S0_DIN_FUNC               PAD_MUX_ALT1

#define RTE_I2S0_DOUT_PAD_ADDR          38
#define RTE_I2S0_DOUT_FUNC              PAD_MUX_ALT1

// I2S1 Configuration
#define RTE_I2S1                        0
#define RTE_I2S1_IO_MODE                DMA_MODE

#define RTE_I2S1_MCLK_PAD_ADDR          18
#define RTE_I2S1_MCLK_FUNC              PAD_MUX_ALT1

#define RTE_I2S1_BCLK_PAD_ADDR          19
#define RTE_I2S1_BCLK_FUNC              PAD_MUX_ALT1

#define RTE_I2S1_LRCK_PAD_ADDR          20
#define RTE_I2S1_LRCK_FUNC              PAD_MUX_ALT1

#define RTE_I2S1_DIN_PAD_ADDR           21
#define RTE_I2S1_DIN_FUNC               PAD_MUX_ALT1

#define RTE_I2S1_DOUT_PAD_ADDR          22
#define RTE_I2S1_DOUT_FUNC              PAD_MUX_ALT1

//////////////////////////////////////////////////////////////////////////////////////////////
// I2S Setting field End
//////////////////////////////////////////////////////////////////////////////////////////////



#endif  /* __RTE_DEVICE_H */
