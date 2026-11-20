/*********************************************************************
 * INCLUDES
 */
#include "os/os.h"
#include "os/mem.h"
#include <string.h>
#include <components/log.h>
#include "board_uart.h"
#include "board_mfrc522.h"
// #include "components/bk_nfc.h"

uint8_t bk_mfrc522_comm_PICC(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData,uint8_t *backLen,uint8_t *validBits,uint8_t rxAlign)
{ 
    uint8_t waitFor = 0x00;
    uint8_t _validBits = 0;
    uint8_t h;
    uint8_t n;

    // Prepare values for MFRC522_REG_BIT_FRAMING
    uint8_t txLastBits = validBits ? *validBits:0;
    uint8_t bitFraming = (rxAlign << 4) + txLastBits;                 // RxAlign = MFRC522_REG_BIT_FRAMING[6..4]. TxLastBits = MFRC522_REG_BIT_FRAMING[2..0]
    
    switch(command)
    {
    case MFRC522_PCD_AUTHENT:
        waitFor = 0x10;
        break;
    case MFRC522_PCD_TRANSCEIVE:
        waitFor = 0x30;
        break;
    default:
        break;
    }

    //bk_mfrc522_write_rawRc(MFRC522_REG_COMIEN, irqEn | 0x80);
    bk_mfrc522_write_rawRc(MFRC522_REG_COMMAND, MFRC522_PCD_IDLE);    // Stop any active command.
    bk_mfrc522_write_rawRc(MFRC522_REG_COMIRQ, 0x7F);                 // Clear all seven interrupt request bits
    bk_mfrc522_write_rawRc(MFRC522_REG_FIFO_LEVEL, 0x80);             // FlushBuffer = 1, FIFO initialization
    int sendData_l = 0;

    for(sendData_l = 0; sendData_l<sendLen;sendData_l++ )
    {
    	bk_mfrc522_write_rawRc(MFRC522_REG_FIFO_DATA, sendData[sendData_l]);
    }

    bk_mfrc522_write_rawRc(MFRC522_REG_BIT_FRAMING, bitFraming);       // Bit adjustments
    bk_mfrc522_write_rawRc(MFRC522_REG_COMMAND, command);             // Execute the command
    if (command == MFRC522_PCD_TRANSCEIVE) 
    {
         bk_mfrc522_set_bit_mask(MFRC522_REG_BIT_FRAMING, 0x80);    // StartSend=1, transmission of data starts
    }

    int i = 600; 
    do
    {
        n = bk_mfrc522_read_rawRc(MFRC522_REG_COMIRQ); 
        i--;
    }
    while((i != 0) && !(n & 0x01) && !(n & waitFor)); 
    bk_mfrc522_clear_bit_mask(MFRC522_REG_BIT_FRAMING, 0x80);

    MFRC522_LOGD("%s , %d ,i :0x%x, n :0x%x\r\n", __FUNCTION__,__LINE__,i, n);
    // Stop now if any errors except collisions were detected.
    uint8_t errorRegValue =  bk_mfrc522_read_rawRc(MFRC522_REG_ERROR); // ErrorReg[7..0] bits are: WrErr TempErr reserved BufferOvfl CollErr CRCErr ParityErr ProtocolErr
    if (errorRegValue & 0x13) {  // BufferOvfl ParityErr ProtocolErr

    	MFRC522_LOGE("status error\r\n");
    	return MI_STATUS_ERROR;
    }

    // If the caller wants data back, get it from the MFRC522.
    if (backData && backLen) {

         h =  bk_mfrc522_read_rawRc( MFRC522_REG_FIFO_LEVEL);    // Number of bytes in the FIFO
        if (h > *backLen) 
        {
        	MFRC522_LOGE("no room....\r\n");
            return MI_STATUS_NO_ROOM;
        }
        *backLen = h;                                           // Number of bytes returned
        int k;
        for(k=0 ; k<h;k++)
        {
        	*(backData+k) =  bk_mfrc522_read_rawRc( MFRC522_REG_FIFO_DATA);
        }

        _validBits =  bk_mfrc522_read_rawRc( MFRC522_REG_CONTROL) & 0x07;       // RxLastBits[2:0] indicates the number of valid bits in the last received uint8_t. If this value is 000b, the whole uint8_t is valid.
        if (validBits) {
            *validBits = _validBits;
        }
        //int i;

    }

    // Tell about collisions
    if (errorRegValue & 0x08) {     // CollErr
    	MFRC522_LOGE("status collision\r\n");
        return MI_STATUS_COLLISION;
    }
    return MI_STATUS_OK;
}


/**
 * Transmits SELECT/ANTICOLLISION commands to select a single PICC.
 * Before calling this function the PICCs must be placed in the READY(*) state by calling PICC_RequestA() or PICC_WakeupA().
 * On success:
 * 		- The chosen PICC is in state ACTIVE(*) and all other PICCs have returned to state IDLE/HALT. (Figure 7 of the ISO/IEC 14443-3 draft.)
 * 		- The UID size and value of the chosen PICC is returned in *uid along with the SAK.
 *
 * A PICC UID consists of 4, 7 or 10 bytes.
 * Only 4 bytes can be specified in a SELECT command, so for the longer UIDs two or three iterations are used:
 * 		UID size	Number of UID bytes		Cascade levels		Example of PICC
 * 		========	===================		==============		===============
 * 		single				 4						1				MIFARE Classic
 * 		double				 7						2				MIFARE Ultralight
 * 		triple				10						3				Not currently in use?
 *
 * @return STATUS_OK on success, STATUS_??? otherwise.
 */
// Description of bytes 2-5: (Section 6.5.4 of the ISO/IEC 14443-3 draft: UID contents and cascade levels)
//		UID size	Cascade level	Byte2	Byte3	Byte4	Byte5
//		========	=============	=====	=====	=====	=====
//		 4 bytes		1			uid0	uid1	uid2	uid3
//		 7 bytes		1			CT		uid0	uid1	uid2
//						2			uid3	uid4	uid5	uid6
//		10 bytes		1			CT		uid0	uid1	uid2
//						2			CT		uid3	uid4	uid5
//						3			uid6	uid7	uid8	uid9
///< Pointer to Uid struct. Normally output, but can also be used to supply a known UID.
///< The number of known UID bits supplied in *uid. Normally 0. If set you must also supply uid->size.

uint8_t bk_mfrc522_read_card_id(uid_num_t *uid,uint8_t validBits) 
{
	uint8_t uidComplete;
	uint8_t selectDone;
	uint8_t useCascadeTag;
	uint8_t cascadeLevel = 1;
	uint8_t result;
	uint8_t count;
	uint8_t checkBit;
	uint8_t index;
	uint8_t uidIndex;
	int8_t  currentLevelKnownBits;
	uint8_t buffer[9];
	uint8_t bufferUsed;
	uint8_t rxAlign;
	uint8_t txLastBits;
	uint8_t *responseBuffer = NULL;
	uint8_t responseLength;

	bk_mfrc522_clear_bit_mask(MFRC522_REG_COLL, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.
	// Repeat Cascade Level loop until we have a complete UID.
	uidComplete = false;
	while (!uidComplete){
		// Set the Cascade Level in the SEL byte, find out if we need to use the Cascade Tag in byte 2.
		switch (cascadeLevel) {
			case 1:
				buffer[0] = MFRC522_PICC_CMD_SEL_CL1;
				uidIndex = 0;
				useCascadeTag = validBits && uid->size > 4;	// When we know that the UID has more than 4 bytes
				break;

			case 2:
				buffer[0] = MFRC522_PICC_CMD_SEL_CL2;
				uidIndex = 3;
				useCascadeTag = validBits && uid->size > 7;	// When we know that the UID has more than 7 bytes
				break;

			case 3:
				buffer[0] = MFRC522_PICC_CMD_SEL_CL3;
				uidIndex = 6;
				useCascadeTag = false;						// Never used in CL3.
				break;

			default:
				return MI_STATUS_INTERNAL_ERROR;
				break;
		}

		// How many UID bits are known in this Cascade Level?
		currentLevelKnownBits = validBits - (8 * uidIndex);
		if (currentLevelKnownBits < 0)
        {
			currentLevelKnownBits = 0;
		}
		// Copy the known bits from uid->uidByte[] to buffer[]
		index = 2; // destination index in buffer[]
		if (useCascadeTag)
        {
			buffer[index++] = MFRC522_PICC_CMD_CT;
		}

		uint8_t bytesToCopy = currentLevelKnownBits / 8 + (currentLevelKnownBits % 8 ? 1 : 0); // The number of bytes needed to represent the known bits for this level.
		if (bytesToCopy) {

			uint8_t maxBytes = useCascadeTag ? 3 : 4; // Max 4 bytes in each Cascade Level. Only 3 left if we use the Cascade Tag
			if (bytesToCopy > maxBytes)
            {
				bytesToCopy = maxBytes;
			}
			for (count = 0; count < bytesToCopy; count++)
            {
				buffer[index] = uid->uidByte[uidIndex + count];
			}
		}
		// Now that the data has been copied we need to include the 8 bits in CT in currentLevelKnownBits
		if (useCascadeTag)
        {
			currentLevelKnownBits += 8;
		}

		// Repeat anti collision loop until we can transmit all UID bits + BCC and receive a SAK - max 32 iterations.
		selectDone = false;
		while (!selectDone) 
        {
			// Find out how many bits and bytes to send and receive.
			if (currentLevelKnownBits >= 32)
            { // All UID bits in this Cascade Level are known. This is a SELECT.
				buffer[1] = 0x70; // NVB - Number of Valid Bits: Seven whole bytes
				buffer[6] = buffer[2] ^ buffer[3] ^ buffer[4] ^ buffer[5]; 	// Calculate BCC - Block Check Character
				
                bk_mfrc522_calulate_crc(buffer, 7, &buffer[7]); // Calculate CRC_A
				txLastBits		= 0;          // 0 => All 8 bits are valid.
				bufferUsed		= 9;
				responseBuffer	= &buffer[6]; // Store response in the last 3 bytes of buffer (BCC and CRC_A - not needed after tx)
				responseLength	= 3;
			}
			else
            { // This is an ANTICOLLISION.;
				txLastBits		= currentLevelKnownBits % 8;
				count			= currentLevelKnownBits / 8;    // Number of whole bytes in the UID part.
				index			= 2 + count;                    // Number of whole bytes: SEL + NVB + UIDs
				buffer[1]		= (index << 4) + txLastBits;    // NVB - Number of Valid Bits
				bufferUsed		= index + (txLastBits ? 1 : 0);
				responseBuffer	= &buffer[index];               // Store response in the unused part of buffer
				responseLength	= sizeof(buffer) - index;
			}
			// Set bit adjustments
			rxAlign = txLastBits;    // Having a separate variable is overkill. But it makes the next line easier to read.
			 bk_mfrc522_write_rawRc(MFRC522_REG_BIT_FRAMING, (rxAlign << 4) + txLastBits);	// RxAlign = MFRC522_REG_BIT_FRAMING[6..4]. TxLastBits = MFRC522_REG_BIT_FRAMING[2..0]
			// Transmit the buffer and receive the response.

			//result = PCD_TransceiveData(buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign,0);
            result =  bk_mfrc522_comm_PICC(MFRC522_PCD_TRANSCEIVE, buffer, bufferUsed, responseBuffer, &responseLength, &txLastBits, rxAlign);
			if (result == MI_STATUS_COLLISION)
			{ // More than one PICC in the field => collision.
				uint8_t valueOfMFRC522_REG_COLL = bk_mfrc522_read_rawRc(MFRC522_REG_COLL); // MFRC522_REG_COLL[7..0] bits are: ValuesAfterColl reserved CollPosNotValid CollPos[4:0]
				if (valueOfMFRC522_REG_COLL & 0x20)
                { // CollPosNotValid
					return MI_STATUS_COLLISION; // Without a valid collision position we cannot continue
				}
				uint8_t collisionPos = valueOfMFRC522_REG_COLL & 0x1F; // Values 0-31, 0 means bit 32.
				if (collisionPos == 0)
                {
					collisionPos = 32;
				}
				if (collisionPos <= currentLevelKnownBits) { // No progress - should not happen
					return MI_STATUS_INTERNAL_ERROR;
				}
				// Choose the PICC with the bit set.
				currentLevelKnownBits	= collisionPos;
				count			= currentLevelKnownBits % 8; // The bit to modify
				checkBit		= (currentLevelKnownBits - 1) % 8;
				index			= 1 + (currentLevelKnownBits / 8) + (count ? 1 : 0); // First byte is index 0.
				buffer[index]	|= (1 << checkBit);
			}
			else if (result != MI_STATUS_OK)
			{
				return result;
			}
			else { // STATUS_OK


				if (currentLevelKnownBits >= 32)    // This was a SELECT.
                {
					selectDone = true; // No more anticollision
					// We continue below outside the while.
				}
				else  // it was an ANTICOLLISION.
                { 
					// We now have all 32 bits of the UID in this Cascade Level
					currentLevelKnownBits = 32;
					// Run loop again to do the SELECT.
				}
			}
		} // End of while (!selectDone)

		// We do not check the CBB - it was constructed by us above.
		// Copy the found UID bytes from buffer[] to uid->uidByte[]
		index			= (buffer[2] == MFRC522_PICC_CMD_CT) ? 3 : 2; // source index in buffer[]
		bytesToCopy		= (buffer[2] == MFRC522_PICC_CMD_CT) ? 3 : 4;
		for (count = 0; count < bytesToCopy; count++)
        {
			uid->uidByte[uidIndex + count] = buffer[index++];
		}

		// Check response SAK (Select Acknowledge)
		if (responseLength != 3 || txLastBits != 0)    // SAK must be exactly 24 bits (1 byte + CRC_A).
        {
			return MI_STATUS_ERROR;
		}
		// Verify CRC_A - do our own calculation and store the control in buffer[2..3] - those bytes are not needed anymore.
		bk_mfrc522_calulate_crc(responseBuffer, 1, &buffer[2]);

		if ((buffer[2] != responseBuffer[1]) || (buffer[3] != responseBuffer[2])) 
        {
			return MI_STATUS_CRC_WRONG;
		}
		if (responseBuffer[0] & 0x04)    // Cascade bit set - UID not complete yes
        { 
			cascadeLevel++;
		}
		else 
        {
			uidComplete = true;
			uid->sak = responseBuffer[0];
		}
	} // End of while (!uidComplete)
	uid->size = 3 * cascadeLevel + 1;    // Set correct uid->size

	return MI_STATUS_OK;
}

void bk_mfrc522_set_low_power(void)
{
	uint8_t tx_control = bk_mfrc522_read_rawRc(MFRC522_REG_TX_CONTROL);

	tx_control &= ~(1 << 5);
	tx_control |=  (1 << 4);
	bk_mfrc522_write_rawRc(MFRC522_REG_TX_CONTROL, tx_control);

	uint8_t tx_sel = bk_mfrc522_read_rawRc(MFRC522_REG_TX_SEL);

	tx_sel &= ~0x30;  
	tx_sel |=  0x10; 
	bk_mfrc522_write_rawRc(MFRC522_REG_TX_SEL, tx_sel);

	uint8_t rf_cfg = bk_mfrc522_read_rawRc(MFRC522_REG_RF_CFG);
	rf_cfg &= 0x0F;
	rf_cfg |= (0x1 << 4);
	bk_mfrc522_write_rawRc(MFRC522_REG_RF_CFG, rf_cfg);

	uint8_t tx_mode = bk_mfrc522_read_rawRc(0x02);  // TxModeReg
	tx_mode &= ~0x07;
	bk_mfrc522_write_rawRc(0x02, tx_mode);

	uint8_t rx_mode = bk_mfrc522_read_rawRc(0x13);  // RxModeReg
	rx_mode &= ~0x07;
	bk_mfrc522_write_rawRc(0x13, rx_mode);
}

/****************************************************END OF FILE****************************************************/