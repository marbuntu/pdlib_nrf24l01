/* NRF24L01 module contains following pins to connect with the controller	
.
 * 
 * CE	->	Chip enable (Activates RX and TX) 
 * CSN	->	SPI chip select signal
 * SCK	->	SPI clock
 * MOSI	->	SPI data input (Master Out Slave Input)
 * MISO	->	SPI data output
 * IRQ	->	Maskable interrupt pin. Active Low
 * 
 * Required SPI operation: (Section 8 nRF24L01 Product Specification)
 * 
 * New command start with HIGH -> LOW on CSN
 * 
 * Command word: 	MSBit to LSBit
 * 
 * Data bytes:		LSByte to MSByte (MSBit first in every byte)
 * 
 * 	
 * =====================================================================
 * LM4F120H5QR (Stellaris)
 * =====================================================================
 * 
 * Stellaris SSI can be configured as Freescale SPI.
 * 
 * The CSN, SCK, MOSI and MISO pins are analogous to Freescale SPI.
 * 
 * Need to have a separate CE pin.
 * 
 * 
 */
 

#include <stdio.h>
#include <stdlib.h>
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "nRF24L01.h"
#include "pdlib_nrf24l01.h"
#include "pdlib_spi.h"


static void _NRF24L01_CEHigh();
static void _NRF24L01_CELow();
	
	
	
	
static void _NRF24L01_CSNHigh();
static void _NRF24L01_CSNLow();
static void _NRF24L01_SendCommand(unsigned char ucCommand, unsigned char *pucData, unsigned int uiLength);


#define TYPE_RX		0x01
#define TYPE_TX		0x02

static unsigned long g_ulCEPin;
static unsigned long g_ulCEBase;
static unsigned long g_ulCSNPin;
static unsigned long g_ulCSNBase;

static unsigned char g_ucStatus;
static unsigned char g_ucPdlibStatus;


/* PS:
 * 
 * Function		: 	NRF24L01_Init
 * 
 * Arguments	: 	ulCEBase	:	This is the base of the CE pin
 * 					ulCEPin		:	This is the pin to be used for CE
 * 					ulCEPeriph	:	This is the peripheral bit mask
 * 					ulCSNBase	:	This is the base of the CSN pin
 * 					ulCSNPin	:	This is the pin to be used for CSN
 * 					ulCSNPeriph	:	This is the peripheral bit mask
 * 					ucSSIIndex	:	The index of the SSI module
 * 
 * Return		: 	None
 * 
 * Description	: 	The function will initialize the SSI communication 
 * 					for the module. Also the function initializes and
 * 					configures the CE pin. It will reset the registers
 * 					to their default values.
 *
 * 					We cannot use the CSN pin (ie. FSS) in the SSI module
 * 					because we need to keep it LOW throughout the data
 * 					transmission. (ie: Write and Read passes) Therefore
 * 					a different pin should be confiured for this purpose
 * 
 */
 	
  
#ifdef PART_LM4F120H5QR

void
NRF24L01_Init(	unsigned long ulCEBase,
				unsigned long ulCEPin,
				unsigned long ulCEPeriph,
				unsigned long ulCSNBase,
				unsigned long ulCSNPin,
				unsigned long ulCSNPeriph,
				unsigned char ucSSIIndex)
{
	g_ucPdlibStatus = 0x00;
	/* PS: Initialize communication */
#ifdef PDLIB_SPI
	pdlibSPI_ConfigureSPIInterface(ucSSIIndex);
#endif

	/* PS: Set the CE pin */
	g_ulCEBase = ulCEBase;
	g_ulCEPin = ulCEPin;
	
	/* PS: Set the CSN pin */
	g_ulCSNBase = ulCSNBase;
	g_ulCSNPin = ulCSNPin;

	/* PS: Configure the CE pin to be GPIO output */
	ROM_SysCtlPeripheralEnable(ulCEPeriph);
	ROM_GPIOPinTypeGPIOOutput(g_ulCEBase, g_ulCEPin);
	
	_NRF24L01_CELow();

	/* PS: Configure the CSN pin to be GPIO output */
	ROM_SysCtlPeripheralEnable(ulCSNPeriph);
	ROM_GPIOPinTypeGPIOOutput(ulCSNBase, ulCSNPin);

	_NRF24L01_CSNHigh();

	NRF24L01_RegisterInit();

	g_ucPdlibStatus = 0x01;
}

#endif

/* PS:
 *
 * Function		: 	NRF24L01_RegisterInit
 *
 * Arguments	:	None
 *
 * Return		: 	None
 *
 * Description	:	This function will reset the CE pin and reset all the registers.
 *
 */

void
NRF24L01_RegisterInit()
{
	unsigned char ucRxAddr1[5] = {0xE7,0xE7,0xE7,0xE7,0xE7};
	unsigned char ucRxAddr2[5] = {0xC2,0xC2,0xC2,0xC2,0xC2};
	unsigned char ucReg;
	
	NRF24L01_FlushTX();
	NRF24L01_FlushRX();

	_NRF24L01_CELow();
	NRF24L01_RegisterWrite_8(RF24_CONFIG,0x09);

	ucReg = NRF24L01_RegisterRead_8(RF24_CONFIG);

	NRF24L01_RegisterWrite_8(RF24_EN_AA,0x3F);

	ucReg = NRF24L01_RegisterRead_8(RF24_EN_AA);

	NRF24L01_RegisterWrite_8(RF24_EN_RXADDR,0x03);

	ucReg = NRF24L01_RegisterRead_8(RF24_EN_RXADDR);

	NRF24L01_RegisterWrite_8(RF24_SETUP_AW,0x03);
	NRF24L01_RegisterWrite_8(RF24_SETUP_RETR,0x03);
	NRF24L01_RegisterWrite_8(RF24_RF_CH,0x02);
	NRF24L01_RegisterWrite_8(RF24_RF_SETUP,0x0F);
	NRF24L01_RegisterWrite_8(RF24_STATUS,0x70);
	NRF24L01_RegisterWrite_8(RF24_CD, 0x00);
	NRF24L01_RegisterWrite_Multi(RF24_RX_ADDR_P0,ucRxAddr1,5);
	NRF24L01_RegisterWrite_Multi(RF24_RX_ADDR_P1,ucRxAddr2,5);
	NRF24L01_RegisterWrite_8(RF24_RX_ADDR_P2,0xC3);
	NRF24L01_RegisterWrite_8(RF24_RX_ADDR_P3,0xC4);
	NRF24L01_RegisterWrite_8(RF24_RX_ADDR_P4,0xC5);
	NRF24L01_RegisterWrite_8(RF24_RX_ADDR_P5,0xC6);
	NRF24L01_RegisterWrite_Multi(RF24_TX_ADDR,ucRxAddr1,5);
	NRF24L01_RegisterWrite_8(RF24_RX_PW_P0,0x00);
	NRF24L01_RegisterWrite_8(RF24_RX_PW_P1,0x00);
	NRF24L01_RegisterWrite_8(RF24_RX_PW_P2,0x00);
	NRF24L01_RegisterWrite_8(RF24_RX_PW_P3,0x00);
	NRF24L01_RegisterWrite_8(RF24_RX_PW_P4,0x00);
	NRF24L01_RegisterWrite_8(RF24_RX_PW_P5,0x00);
	NRF24L01_RegisterWrite_8(RF24_DYNPD,0x00);
	NRF24L01_RegisterWrite_8(RF24_FEATURE,0x00);


/*	_NRF24L01_CELow();
	NRF24L01_RegisterWrite_8(RF24_CONFIG,0x08);
	NRF24L01_RegisterWrite_8(RF24_EN_AA,0x3F);
	NRF24L01_RegisterWrite_8(RF24_EN_RXADDR,0x03);
	NRF24L01_RegisterWrite_8(RF24_SETUP_AW,0x03);
	NRF24L01_RegisterWrite_8(RF24_SETUP_RETR,0x9F);
	NRF24L01_RegisterWrite_8(RF24_RF_CH,0x02);
	NRF24L01_RegisterWrite_8(RF24_RF_SETUP,0x0F);
	NRF24L01_RegisterWrite_8(RF24_STATUS,0x70);
	NRF24L01_RegisterWrite_8(RF24_DYNPD,0x03);
	NRF24L01_RegisterWrite_8(RF24_CD, 0x00);
	NRF24L01_RegisterWrite_8(RF24_FEATURE,RF24_EN_DPL);*/

	//NRF24L01_SetRFChannel(0);
	//NRF24L01_SetAirDataRate(0);
	//NRF24L01_SetPAGain(0);

}

/* PS:
 * 
 * Function		: 	NRF24L01_GetStatus
 * 
 * Arguments	: 	None.
 * 
 * Return		: 	Status register value.
 * 
 * Description	: 	Gets the status register of the module
 * 
 */

unsigned char 
NRF24L01_GetStatus()
{
	NRF24L01_RegisterRead_8(RF24_NOP);
	return g_ucStatus;
}

/* PS:
 * 
 * Function		: 	NRF24L01_SetAirDataRate
 * 
 * Arguments	: 	ucDataRate	:	1 for 1 Mbps
 * 									2 for 2 Mbps
 * 
 * Return		: 	None
 * 
 * Description	: 	Sets the air data rate (default 2Mbps)
 * 
 */
 
void
NRF24L01_SetAirDataRate(unsigned char ucDataRate)
{
	unsigned char ucCurrentVal = NRF24L01_RegisterRead_8(RF24_RF_SETUP);

	ucDataRate--;	// 0 for 1 Mbps, 1 for 2 Mbps

	ucCurrentVal |= (ucDataRate << 3);
	
	NRF24L01_RegisterWrite_8(RF24_RF_SETUP, ucCurrentVal);
}
 
 
 
/* PS:
 * 
 * Function		: 	NRF24L01_SetRFChannel
 * 
 * Arguments	: 	ucRFChannel	:	RF channel value (only 0:6 bits valid)
 * 
 * Return		: 	None
 * 
 * Description	: 	Sets the RF channel frequency based on the below
 * 					formula,
 * 						2400 + ucRFChannel (MHz)
 * 
 */
 
void
NRF24L01_SetRFChannel(unsigned char ucRFChannel)
{
	NRF24L01_RegisterWrite_8(RF24_RF_CH, ucRFChannel);
}
 
 
 
/* PS:
 * 
 * Function		: 	NRF24L01_SetPAGain
 * 
 * Arguments	: 	ucPAGain	: Only bits 0:1 are valid.
 * 
 * Return		: 	None
 * 
 * Description	: 	Sets the power amplifier gain based on the ucPAGain.
 * 					0 = 0 dBm (default)
 * 					-6 = -6 dBm
 * 					-12 = -12 dBm
 * 					-18 = -18 dBm
 * 
 */
 
void
NRF24L01_SetPAGain(int iPAGain)
{
	unsigned char ucCurrentVal = NRF24L01_RegisterRead_8(RF24_RF_SETUP);

	if(iPAGain < -18)
	{
		iPAGain = -18;
	}else if(iPAGain > 0)
	{
		iPAGain = 0;
	}

	 /* PS:
	  * 				11 = 0 dBm (default)
	  *					10 = -6 dBm
	  *					01 = -12 dBm
	  *					00 = -18 dBm
	  */

	iPAGain = 3 - (-1*(iPAGain) / 6);

	ucCurrentVal |= ((iPAGain && 0x03) << 1);
	
	NRF24L01_RegisterWrite_8(RF24_RF_SETUP, ucCurrentVal);
} 
 
 
/* PS:
 * 
 * Function		: 	NRF24L01_SetLNAGain
 * 
 * Arguments	: 	ucLNAGain	:	0 - Disable LNA gain
 * 									1 - Enable LNA gain
 * 
 * Return		: 	None
 * 
 * Description	: 	Function will setup the LNA gain of the module.
 * 
 */
 
void
NRF24L01_SetLNAGain(unsigned char ucLNAGain)
{
	unsigned char ucCurrentVal = NRF24L01_RegisterRead_8(RF24_RF_SETUP);
	ucCurrentVal |= (ucLNAGain << 0);
	
	NRF24L01_RegisterWrite_8(RF24_RF_SETUP, ucCurrentVal);
}


/* PS:
 * 
 * Function		: 	NRF24L01_PowerDown
 * 
 * Arguments	: 	None
 * 
 * Return		: 	None
 * 
 * Description	: 	Go to power down mode
 * 
 */
 
void
NRF24L01_PowerDown()
{
	unsigned char ucCurrentVal = NRF24L01_RegisterRead_8(RF24_CONFIG);
	ucCurrentVal &= (~RF24_PWR_UP);
	
	NRF24L01_RegisterWrite_8(RF24_CONFIG, ucCurrentVal);
	
	_NRF24L01_CELow();
}

/* PS:
 * 
 * Function		: 	NRF24L01_PowerUp
 * 
 * Arguments	: 	None
 * 
 * Return		: 	None
 * 
 * Description	: 	Go to Standby mode. Takes 1.5 ms to go to Standby.
 * 
 */
 
void
NRF24L01_PowerUp()
{
	unsigned char ucCurrentVal = NRF24L01_RegisterRead_8(RF24_CONFIG);
	ucCurrentVal |= (RF24_PWR_UP);
	NRF24L01_RegisterWrite_8(RF24_CONFIG, ucCurrentVal);
}


/* PS:
 * 
 * Function		: 	NRF24L01_FlushTX
 * 
 * Arguments	: 	None
 * 
 * Return		: 	None
 * 
 * Description	: 	Flush tx.
 * 
 */
 
void
NRF24L01_FlushTX()
{
	_NRF24L01_SendCommand(RF24_FLUSH_TX, NULL, 0);
}


/* PS:
 * 
 * Function		: 	NRF24L01_FlushRX
 * 
 * Arguments	: 	None
 * 
 * Return		: 	None
 * 
 * Description	: 	Flush rx.
 * 
 */
 
void
NRF24L01_FlushRX()
{
	_NRF24L01_SendCommand(RF24_FLUSH_RX, NULL, 0);
}


/* PS:
 * 
 * Function		: 	NRF24L01_EnableRxMode
 * 
 * Arguments	: 	None
 * 
 * Return		: 	None
 * 
 * Description	: 	Put the module into RX mode
 * 
 */

void
NRF24L01_EnableRxMode()
{
	unsigned char ucCurrentVal = NRF24L01_RegisterRead_8(RF24_CONFIG);
	ucCurrentVal |= (RF24_PRIM_RX);
	
	NRF24L01_RegisterWrite_8(RF24_CONFIG, ucCurrentVal);
	
	_NRF24L01_CEHigh();
}


/* PS:
 * 
 * Function		: 	NRF24L01_EnableTxMode
 * 
 * Arguments	: 	None
 * 
 * Return		: 	None
 * 
 * Description	: 	Put the module into TX mode
 * 
 */

void
NRF24L01_EnableTxMode()
{
	unsigned char ucCurrentVal = NRF24L01_GetStatus();

	// PS: Clear TX_DS and MAX_RT interrupts
	ucCurrentVal &= (~(RF24_TX_DS | RF24_MAX_RT));
	NRF24L01_RegisterWrite_8(RF24_STATUS, ucCurrentVal);

	// PS: Set to TX mode
	ucCurrentVal = NRF24L01_RegisterRead_8(RF24_CONFIG);
	ucCurrentVal &= (~RF24_PRIM_RX);
	
	NRF24L01_RegisterWrite_8(RF24_CONFIG, ucCurrentVal);
	
	_NRF24L01_CEHigh();
}

/* PS:
 * 
 * Function		: 	NRF24L01_IsDataReadyRx
 * 
 * Arguments	: 	None
 * 
 * Return		: 	0	:	Data is not in RX FIFO
 * 					1	:	Data is in RX FIFO
 * 
 * Description	: 	Get the state of RX FIFO
 * 
 */
 
int
NRF24L01_IsDataReadyRx()
{
	unsigned char ucRxFifo;
	ucRxFifo = NRF24L01_RegisterRead_8(RF24_FIFO_STATUS);
	
	return ((ucRxFifo & RF24_RX_EMPTY) ? 0 : 1);
}



/* PS:
 *
 * Function		: 	NRF24L01_IsTxFifoFull
 *
 * Arguments	: 	None
 *
 * Return		: 	0	:	TX FIFO is NOT full
 * 					1	:	TX FIFO is full
 *
 * Description	: 	Get the state of TX FIFO
 *
 */

int
NRF24L01_IsTxFifoFull()
{
	unsigned char ucTxFifo;
	ucTxFifo = NRF24L01_RegisterRead_8(RF24_FIFO_STATUS);

	return ((ucTxFifo & RF24_TX_FULL) ? 0 : 1);
}



/* PS:
 * 
 * Function		: 	NRF24L01_GetRxDataAmount
 * 
 * Arguments	: 	ucDataPipe	:	Index of the pipe
 * 
 * Return		: 	Available number of bytes.
 * 
 * Description	: 	Reading the number of data bytes available
 * 					in the specified pipe.
 * 
 */

unsigned char
NRF24L01_GetRxDataAmount(unsigned char ucDataPipe)
{
	unsigned char ucTemp;
	if(ucDataPipe < 6)
	{
		ucTemp = NRF24L01_RegisterRead_8(RF24_RX_PW_P0 + ucDataPipe);
		return (ucTemp & 0x3F);
	}else
	{
		return 0;
	}
}
	
	
/* PS:
 * 
 * Function		: 	NRF24L01_SetTXAddress
 * 
 * Arguments	: 	pucAddress	:	Buffer which contains the five bytes to put to address.
 * 
 * Return		: 	None
 * 
 * Description	: 	Set the address of the module for TX mode. The data
 * 					packet which is transmitted from this RF module will
 * 					contain this address as destination address.
 * 
 */
 
void 
NRF24L01_SetTXAddress(unsigned char* pucAddress)
{
	NRF24L01_RegisterWrite_Multi(RF24_TX_ADDR, pucAddress, 5);
}



/* PS:
 *
 * Function		: 	NRF24L01_WaitForTxComplete
 *
 * Arguments	: 	ucWaitMode	:	0 : Do not wait, return the current state
 * 									1 : Wait until TX done or MAX_RT asserted
 *
 * Return		: 	0	:	TX not complete and no error reported
 * 					1	:	TX complete with error
 * 					2	:	TX completed successfully
 *
 * Description	: 	The function can wait until the
 * 						-TX payload is successfully delivered (If ACK is available)
 * 						or
 * 						-TX payload has successfully transmitted.
 *
 * 					The function will be blocking if ucWaitMode is '1'.
 *
 * 					The function will return -1 if the maximum no of retransmissions elapsed.
 *
 */

int
NRF24L01_WaitForTxComplete(unsigned char ucWaitMode)
{
	int ret = 0;

	NRF24L01_GetStatus();

	if(!ucWaitMode)
	{
		ret = ((g_ucStatus & (RF24_MAX_RT | RF24_TX_DS)) << 4);
	}else
	{
		while(!(g_ucStatus & (RF24_MAX_RT | RF24_TX_DS)))
		{
			NRF24L01_GetStatus();
		}

		ret = ((g_ucStatus & (RF24_MAX_RT | RF24_TX_DS)) << 4);
	}

	return ret;
}


/* PS:
 * 
 * Function		: 	NRF24L01_SetRXAddress
 * 
 * Arguments	: 	ucDataPipe	:	Data pipe number
 * 					pucAddress	:	Buffer which contains the one/five bytes to put to address.
 * 
 * Return		: 	None
 * 
 * Description	: 	Set the RX address. P0 and P1 pipes have 5 byte address
 * 					other pipes have 1 byte address(LSB). Other bytes are taken from
 * 					the P1 pipe address.
 * 
 */
 
void 
NRF24L01_SetRxAddress(	unsigned char ucDataPipe,
						unsigned char *pucAddress)
{
	if(pucAddress)
	{
		switch(ucDataPipe)
		{
			case 0:
			case 1:
				NRF24L01_RegisterWrite_Multi((RF24_RX_ADDR_P0 + ucDataPipe), pucAddress, 5);
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				NRF24L01_RegisterWrite_8((RF24_RX_ADDR_P0 + ucDataPipe),pucAddress[0]);
				break;
			default:
				break;
		}
	}
}


/* PS:
 * 
 * Function		: 	NRF24L01_SetTxPayload
 * 
 * Arguments	: 	pucData		:	Buffer which contains the data to be written to TX fifo
 * 					uiLength	:	Length of the data buffer
 * 
 * Return		: 	-1 	: Tx FIFO full
 * 					0	: Success
 * 
 * Description	: 	Set the TX payload. 
 * 
 */
 
int
NRF24L01_SetTxPayload(	unsigned char* pucData,
						unsigned int uiLength)
{
	int ret = 0;

	// PS: Check whether TX fifo is full
	if(NRF24L01_IsTxFifoFull())
	{
		ret = -1;
	}else
	{

		// PS: Check the Auto Ack feature

		// PS: Check TX reuse feature

		_NRF24L01_SendCommand(RF24_W_TX_PAYLOAD, pucData, uiLength);
	}

	return ret;
}
 
 
/* PS:
 * 
 * Function		: 	NRF24L01_
 * 
 * Arguments	: 	
 * 
 * Return		: 	None
 * 
 * Description	: 	
 * 
 */


/* PS: Not used
 * 
 * Function		: 	NRF24L01_ExecuteCommand
 * 
 * Arguments	: 	ucType		: 	0x01 - For RX
 * 									0x02 - For TX
 * 					ucCommand	:	Command to be executed.
 * 					pucData		:	Tx: Payload of the command.
 * 									Rx: Buffer to receive data.
 * 					uiLength	:	Length of the payload/buffer.
 * 									(If there is no data buffer this value
 * 									must be set to zero)
 * 
 * 					pucStatus	: 	If not NULL status register value will
 * 									be saved here.
 * 
 * Return		: 	Number of bytes transferred or received if success.
 * 					Negetive error code if failed.
 * 
 * Description	: 	ExecuteCommand function can be used to send commands
 * 					to the RF module 
 * 
 * 					For TX:
 * 
 * 					The payload	of the command can also be submitted to 
 * 					the function for easier operation. The ucType parameter 
 * 					should be set to 0x02. The function will return after
 * 					all the data is submitted to the module.
 * 
 * 					For RX:
 * 
 * 					A buffer should be preallocated and provided to the 
 * 					function. The length of the buffer is also expected.
 * 					The ucType parameter should be set to 0x01. Function 
 * 					will return as soon as it received the number of bytes 
 * 					specified by the uiLength or when the module has no
 * 					more data in the Rx FIFO.
 * 
 */

int
NRF24L01_ExecuteCommand(unsigned char ucType, unsigned char ucCommand, unsigned char *pucData, unsigned int uiLength, unsigned char *pucStatus)
{
	int iReturn = -1;
	unsigned char *pucDataBuffer = NULL;
	
	/* PS: Validating arguments */
	if(uiLength > 0)
	{
		if(NULL == pucData)
		{
			return iReturn;
		}
	}else
	{
		if((TYPE_TX == ucType) && (RF24_NOP != ucCommand))
		{
			/* PS: Buffer length should be at least one */
			return iReturn;
		}
	}
	
	if(TYPE_RX == ucType)
	{
		/* PS: Rx operation */
	}else if(TYPE_TX == ucType)
	{
		/* PS: Tx operation */
		
		/* TODO:
		 * Sometimes malloc and memcpy functions take longer time.
		 * Need to check whether we can improve performance by 
		 * executing the pdlibSPI_SendData function twice. That is 
		 * once for command and next for payload. Then we can 
		 * avoid using memcpy and malloc here.
		 */

#ifdef PDLIB_SPI
		pucDataBuffer = (unsigned char*) malloc(sizeof(char) * (uiLength+1));
		if(pucDataBuffer)
		{
			pucDataBuffer[0] = ucCommand;
			memcpy(&pucDataBuffer[1],pucData,uiLength);
			
			iReturn = pdlibSPI_SendData(pucDataBuffer, (uiLength+1));
			free(pucDataBuffer);
			/* PS: Update the current status register value */
			pdlibSPI_ReceiveDataBlocking(&g_ucStatus);
			(*pucStatus) = g_ucStatus;
		}else
		{
			iReturn = -1;
		}
		
#endif
	}else
	{
	}
	
	
	return iReturn;
}


// ----------------  Hardware Pin Control ------------------ //


/* PS:
 * 
 * Function		: 	_NRF24L01_CELow
 * 
 * Arguments	: 	None
 * 
 * Return		: 	None
 * 
 * Description	: 	This function drives the CE pin low
 * 
 */
 
static void
_NRF24L01_CELow()
{
	ROM_GPIOPinWrite(g_ulCEBase, g_ulCEPin, 0x00);
}
 
  
/* PS:
 * 
 * Function		: 	_NRF24L01_CEHigh
 * 
 * Arguments	: 	None
 * 
 * Return		: 	None
 * 
 * Description	: 	This function drives the CE pin high
 * 
 */

static void
_NRF24L01_CEHigh()
{
	ROM_GPIOPinWrite(g_ulCEBase, g_ulCEPin, 0xFF);
}


/* PS:
 *
 * Function		: 	_NRF24L01_CSNLow
 *
 * Arguments	: 	None
 *
 * Return		: 	None
 *
 * Description	: 	This function drives the CSN pin low
 *
 */

static void
_NRF24L01_CSNLow()
{
	ROM_GPIOPinWrite(g_ulCSNBase, g_ulCSNPin, 0x00);
}


/* PS:
 *
 * Function		: 	_NRF24L01_CSNHigh
 *
 * Arguments	: 	None
 *
 * Return		: 	None
 *
 * Description	: 	This function drives the CSN pin high
 *
 */

static void
_NRF24L01_CSNHigh()
{
	ROM_GPIOPinWrite(g_ulCSNBase, g_ulCSNPin, 0xFF);
}


// ----------------------- Register Read/Write functions ---------------------- //


/* PS:
 * 
 * Function		: 	NRF24L01_RegisterWrite_8
 * 
 * Arguments	: 	ucRegister	:	Address of the register
 * 					ucValue		:	Value to write to the register
 * 
 * Return		: 	None
 * 
 * Description	: 	This function will write 1 byte of data to an 8 bit
 * 					register. The function will update the Status
 * 					variable too.
 * 
 */
 
static void
NRF24L01_RegisterWrite_8(unsigned char ucRegister, unsigned char ucValue)
{
	unsigned char ucData[2];
	
	ucData[0] = (RF24_W_REGISTER | ucRegister);
	ucData[1] = ucValue;
	
	_NRF24L01_CSNLow();
	
	/* PS: Send address */
	g_ucStatus = pdlibSPI_TransferByte(ucData[0]);

	/* PS: Send data */
	pdlibSPI_TransferByte(ucData[1]);

	_NRF24L01_CSNHigh();
}


/* PS:
 * 
 * Function		: 	NRF24L01_RegisterWrite_Multi
 * 
 * Arguments	: 	ucRegister	:	Address of the register
 * 					pucData		:	Value to write to the register
 * 					uiLength	:	Length of the data field
 * 
 * Return		: 	None
 * 
 * Description	: 	This function will write more than one byte of data 
 * 					to an the specified register. 
 * 					Function will update the Status	variable too.
 * 
 */
 
static void
NRF24L01_RegisterWrite_Multi(	unsigned char ucRegister,
								unsigned char *pucData,
								unsigned int uiLength)
{
	if(NULL != pucData)
	{
		unsigned char *pucBuffer = (unsigned char*) malloc(sizeof(unsigned char) * (uiLength));
		
		if(NULL != pucBuffer)
		{
			memcpy(pucBuffer, pucData, uiLength);

			_NRF24L01_CSNLow();

			g_ucStatus = pdlibSPI_TransferByte(RF24_W_REGISTER | ucRegister);
			pdlibSPI_SendData(pucBuffer, uiLength);
			
			_NRF24L01_CSNHigh();

			free(pucBuffer);
		}
	}
}


/* PS:
 * 
 * Function		: 	_NRF24L01_SendCommand
 * 
 * Arguments	: 	ucCommand	:	Command to send.
 * 					pucData		:	Data associated with the command
 * 					uiLength	:	Length of the data field
 * 
 * Return		: 	None
 * 
 * Description	: 	This function will send any command with the data buffer
 * 					to the RF module. This will also update the status register.
 * 					If there is no payload for the command set the pucData NULL and
 * 					make the uiLength as 0
 * 
 */
 
static void
_NRF24L01_SendCommand(	unsigned char ucCommand,
						unsigned char *pucData,
						unsigned int uiLength)
{
	unsigned char *pucBuffer = (unsigned char*) malloc(sizeof(unsigned char) * (uiLength + 1));
	
	if(NULL != pucBuffer)
	{
		pucBuffer[0] = ucCommand;
		
		if(NULL != pucData)
		{
			memcpy(&pucBuffer[1], pucData, uiLength);
		}
		
		_NRF24L01_CSNLow();

		pdlibSPI_SendData(pucBuffer, uiLength+1);
		
		//g_ucStatus = pdlibSPI_ReceiveDataBlocking();
		
		_NRF24L01_CSNHigh();

		free(pucBuffer);
	}
}


/* PS:
 * 
 * Function		: 	NRF24L01_RegisterRead_8
 * 
 * Arguments	: 	ucRegister	:	Address of the register
 * 
 * Return		: 	The value in the 8 bit register
 * 
 * Description	: 	This function will write 1 byte of data to an 8 bit
 * 					register. The function will update the Status
 * 					variable too.
 * 
 */
 
unsigned char
NRF24L01_RegisterRead_8(unsigned char ucRegister)
{
	unsigned char ucData;

	_NRF24L01_CSNLow();

	g_ucStatus = pdlibSPI_TransferByte(RF24_R_REGISTER | ucRegister);
	
	ucData = pdlibSPI_TransferByte(RF24_NOP);

	_NRF24L01_CSNHigh();

	return ucData;
}


/* PS:
 *
 * Function		: 	NRF24L01_RegisterRead_Multi
 *
 * Arguments	: 	ucRegister	:	Address of the register
 * 					pucBuffer	: 	Buffer to store the reply
 * 					uiLength	:	Length of the buffer
 *
 * Return		: 	Status register value will be returned.
 *
 * Description	: 	This function will read more than 1 byte of data from a
 * 					register. The function will update the Status
 * 					variable too.
 *
 */

unsigned char
NRF24L01_RegisterRead_Multi(	unsigned char ucRegister,
								unsigned char *pucBuffer,
								unsigned int uiLength)
{
	int i;

	_NRF24L01_CSNLow();

	g_ucStatus = pdlibSPI_TransferByte(RF24_R_REGISTER | ucRegister);

	for(i = 0; i<uiLength; i++)
	{
		pucBuffer[i] = pdlibSPI_TransferByte(RF24_NOP);
	}

	_NRF24L01_CSNHigh();

	return g_ucStatus;
}
