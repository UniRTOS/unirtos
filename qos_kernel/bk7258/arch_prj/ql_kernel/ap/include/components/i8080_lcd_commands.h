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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Common LCD panel commands */
#define I8080_LCD_CMD_NOP          0x00 // This command is empty command
#define I8080_LCD_CMD_SWRESET      0x01 // Software reset registers (the built-in frame buffer is not affected)
#define I8080_LCD_CMD_RDDID        0x04 // Read 24-bit display ID
#define I8080_LCD_CMD_RDDST        0x09 // Read display status
#define I8080_LCD_CMD_RDDPM        0x0A // Read display power mode
#define I8080_LCD_CMD_RDD_MADCTL   0x0B // Read display MADCTL
#define I8080_LCD_CMD_RDD_COLMOD   0x0C // Read display pixel format
#define I8080_LCD_CMD_RDDIM        0x0D // Read display image mode
#define I8080_LCD_CMD_RDDSM        0x0E // Read display signal mode
#define I8080_LCD_CMD_RDDSR        0x0F // Read display self-diagnostic result
#define I8080_LCD_CMD_SLPIN        0x10 // Go into sleep mode (DC/DC, oscillator, scanning stopped, but memory keeps content)
#define I8080_LCD_CMD_SLPOUT       0x11 // Exit sleep mode
#define I8080_LCD_CMD_PTLON        0x12 // Turns on partial display mode
#define I8080_LCD_CMD_NORON        0x13 // Turns on normal display mode
#define I8080_LCD_CMD_INVOFF       0x20 // Recover from display inversion mode
#define I8080_LCD_CMD_INVON        0x21 // Go into display inversion mode
#define I8080_LCD_CMD_GAMSET       0x26 // Select Gamma curve for current display
#define I8080_LCD_CMD_DISPOFF      0x28 // Display off (disable frame buffer output)
#define I8080_LCD_CMD_DISPON       0x29 // Display on (enable frame buffer output)
#define I8080_LCD_CMD_CASET        0x2A // Set column address
#define I8080_LCD_CMD_RASET        0x2B // Set row address
#define I8080_LCD_CMD_RAMWR        0x2C // Write frame memory
#define I8080_LCD_CMD_RAMRD        0x2E // Read frame memory
#define I8080_LCD_CMD_PTLAR        0x30 // Define the partial area
#define I8080_LCD_CMD_VSCRDEF      0x33 // Vertical scrolling definition
#define I8080_LCD_CMD_TEOFF        0x34 // Turns off tearing effect
#define I8080_LCD_CMD_TEON         0x35 // Turns on tearing effect
#define I8080_LCD_CMD_MADCTL       0x36 // Memory data access control
#define I8080_LCD_CMD_VSCSAD       0x37 // Vertical scroll start address
#define I8080_LCD_CMD_IDMOFF       0x38 // Recover from IDLE mode
#define I8080_LCD_CMD_IDMON        0x39 // Fall into IDLE mode (8 color depth is displayed)
#define I8080_LCD_CMD_COLMOD       0x3A // Defines the format of RGB picture data
#define I8080_LCD_CMD_RAMWRC       0x3C // Memory write continue
#define I8080_LCD_CMD_RAMRDC       0x3E // Memory read continue
#define I8080_LCD_CMD_STE          0x44 // Set tear scan line, tearing effect output signal when display module reaches line N
#define I8080_LCD_CMD_GDCAN        0x45 // Get scan line
#define I8080_LCD_CMD_WRDISBV      0x51 // Write display brightness
#define I8080_LCD_CMD_RDDISBV      0x52 // Read display brightness value


/*
 * @}
 */

#ifdef __cplusplus
}
#endif
