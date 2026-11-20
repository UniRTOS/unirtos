#ifndef __BSP_NFC_H__
#define __BSP_NFC_H__

/**
 @brief NFC module initialization
 @param No parameters
 @return No return value
 */
void bk_nfc_init(void);

/**
 @brief  Search for card 
 @param reqCode -[in] Search method for cards，0x52Search method for cards, 0x52 searches for all cards compliant with the 1443A standard ，0x26 searches for cards not in a sleep state 
 @param pTagType -[out] Tag type code
                        0x4400 = Mifare_UltraLight
                        0x0400 = Mifare_One(S50)
                        0x0200 = Mifare_One(S70)
                        0x0800 = Mifare_Pro(X)
                        0x4403 = Mifare_DESFire
 @return state value，MI OK - successful；MI_ERR - failed
*/
char bk_mfrc522_request(uint8_t reqCode, uint8_t *pTagType);

/**
 @brief Anti-collision
 @param pSnr -[out] Card serial number, 4 bytes
 @return state value，MI OK - successful；MI_ERR - failed
*/
char bk_mfrc522_anticoll(uint8_t *pSnr);

/**
 @brief Select Card 
 @param pSnr -[in] Card serial number, 4 bytes
 @return state value，MI OK - successful；MI_ERR - failed
*/
char bk_mfrc522_select(uint8_t *pSnr);

/**
 @brief Verify card password 
 @param authMode -[in] Password verification mode， 0x60 verifies A key, 0x61 verifies B key
 @param addr -[in] Block address
 @param pKey -[in] Password
 @param pSnr -[in] Card serial number, 4 bytes
 @return state value，MI OK - successful；MI_ERR - failed
*/
char bk_mfrc522_authState(uint8_t authMode, uint8_t addr, uint8_t *pKey, uint8_t *pSnr);


/**
 @brief Read one block of data from an M1 card
 @param addr -[in] Block address
 @param pData -[out] The read data (16 bytes)
 @return Status value, MI_OK - success; MI_ERR - failure
*/
char bk_mfrc522_read(uint8_t addr, uint8_t *pData);

/**
 @brief Write a block of data to the M1 card.
 @param addr -[in] Block address
 @param pData -[out] Data to write, 16 bytes
 @return Status value, MI_OK - success; MI_ERR - failure
 */
char bk_mfrc522_write(uint8_t addr, uint8_t *pData);

/**
 @brief NFC module deint
 @param No parameters
 @return No return value
 */
void bk_nfc_deinit(void);


typedef uint8_t (*nfc_event_callback_t)(uint8_t event_param, void*card_id);
int nfc_event_callback_register(nfc_event_callback_t callback);

#endif //__BSP_NFC_H__

