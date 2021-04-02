/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "board.h"
#include "fsl_dma.h"
#include "fsl_i2c.h"
#include "fsl_i2s.h"
#include "fsl_i2s_dma.h"
#include "fsl_wm8904.h"
#include "fsl_codec_common.h"
#include "music.h"
#include "example_tones.h"
//#include "stereo_tones.h"

#include <stdbool.h>
#include "fsl_sysctl.h"
#include "fsl_codec_adapter.h"
#include "fsl_power.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define I2C                        (I2C4)
#define I2S_MASTER_CLOCK_FREQUENCY (24576000)
#define I2S_TX                     (I2S7)
#define I2S_RX                     (I2S6)
#define DEMO_DMA                        (DMA0)
#define DEMO_I2S_TX_CHANNEL             (19)
#define I2S_RX_CHANNEL             (16)
#define I2S_CLOCK_DIVIDER          (CLOCK_GetPll0OutFreq() / 48000U / 16U / 2U)
#define I2S_TX_MODE                (kI2S_MasterSlaveNormalMaster)
#define AUDIO_BIT_WIDTH            (16)
#define AUDIO_SAMPLE_RATE          (48000)
#define AUDIO_PROTOCOL             kCODEC_BusI2S
#define VOLUME 							20U

// Codec registers
#define R_AUDIO_INTERFACE_0				0X18

// I2C stuff
#define I2C_MASTER_SLAVE_ADDR_7BIT 		0x1AU // codec addr
#define I2C_MASTER 						((I2C_Type *)I2C4_BASE)
#define I2C_MASTER_CLOCK_FREQUENCY 		(12000000)
#define I2C_DATA_LENGTH            		2U


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void StartSoundPlayback(void);

static void TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData);

static void printRegVals(void);

static void customCodecConfig(void);

static int writeToReg(uint8_t regAddr, const void *txBuff);


/*******************************************************************************
 * Variables
 ******************************************************************************/
wm8904_config_t wm8904Config = {
    .i2cConfig    = {.codecI2CInstance = BOARD_CODEC_I2C_INSTANCE, .codecI2CSourceClock = BOARD_CODEC_I2C_CLOCK_FREQ},
    .recordSource = kWM8904_RecordSourceLineInput,
    .recordChannelLeft  = kWM8904_RecordChannelLeft2,
    .recordChannelRight = kWM8904_RecordChannelRight2,
    .playSource         = kWM8904_PlaySourceDAC,
    .slaveAddress       = WM8904_I2C_ADDRESS,
    .protocol           = kWM8904_ProtocolI2S,
    .format             = {.sampleRate = kWM8904_SampleRate48kHz, .bitWidth = kWM8904_BitWidth16},
    .mclk_HZ            = I2S_MASTER_CLOCK_FREQUENCY,
    .master             = false,
};


codec_config_t boardCodecConfig = {.codecDevType = kCODEC_WM8904, .codecDevConfig = &wm8904Config};

static dma_handle_t s_DmaTxHandle;
static i2s_config_t s_TxConfig;
static i2s_dma_handle_t s_TxHandle;
static i2s_transfer_t s_TxTransfer;
extern codec_config_t boardCodecConfig;
codec_handle_t codecHandle;

uint8_t g_master_txBuff[I2C_DATA_LENGTH];
uint8_t g_master_rxBuff[I2C_DATA_LENGTH];


/*******************************************************************************
 * Code
 ******************************************************************************/


void BOARD_InitSysctrl(void) {
    SYSCTL_Init(SYSCTL);
    /* select signal source for share set */
    SYSCTL_SetShareSignalSrc(SYSCTL, kSYSCTL_ShareSet0, kSYSCTL_SharedCtrlSignalSCK, kSYSCTL_Flexcomm7);
    SYSCTL_SetShareSignalSrc(SYSCTL, kSYSCTL_ShareSet0, kSYSCTL_SharedCtrlSignalWS, kSYSCTL_Flexcomm7);
    /* select share set for special flexcomm signal */
    SYSCTL_SetShareSet(SYSCTL, kSYSCTL_Flexcomm7, kSYSCTL_FlexcommSignalSCK, kSYSCTL_ShareSet0);
    SYSCTL_SetShareSet(SYSCTL, kSYSCTL_Flexcomm7, kSYSCTL_FlexcommSignalWS, kSYSCTL_ShareSet0);
    SYSCTL_SetShareSet(SYSCTL, kSYSCTL_Flexcomm6, kSYSCTL_FlexcommSignalSCK, kSYSCTL_ShareSet0);
    SYSCTL_SetShareSet(SYSCTL, kSYSCTL_Flexcomm6, kSYSCTL_FlexcommSignalWS, kSYSCTL_ShareSet0);
}

static void StartSoundPlayback(void) {

    PRINTF("Setup looping playback of sine wave\r\n");
/*
    s_TxTransfer.data     = &audioArray[0];
    s_TxTransfer.dataSize = sizeof(*audioArray);
   */
    /*
    s_TxTransfer.data     = &g_Music[0];
    s_TxTransfer.dataSize = sizeof(g_Music);
    */
    s_TxTransfer.data     = (uint8_t *)&data[0];
    s_TxTransfer.dataSize = sizeof(data);

    PRINTF("size of transfer: %d\r\n", sizeof(data));


    I2S_TxTransferCreateHandleDMA(I2S_TX, &s_TxHandle, &s_DmaTxHandle, TxCallback, (void *)&s_TxTransfer);
    /* need to queue two transmit buffers so when the first one
     * finishes transfer, the other immediatelly starts */
    // TODO why does it start looping only after the second call?
    I2S_TxTransferSendDMA(I2S_TX, &s_TxHandle, s_TxTransfer); // I hear a blip
    //I2S_TxTransferSendDMA(DEMO_I2S_TX, &s_TxHandle, s_TxTransfer); // I hear a constant tone???
}

static void TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData) {
    /* Enqueue the same original buffer all over again */

	/*
    i2s_transfer_t *transfer = (i2s_transfer_t *)userData;
    I2S_TxTransferSendDMA(base, handle, *transfer); */
}

// use I2C to print codec register values
static int printRegValues(void){

	i2c_master_config_t masterConfig;
	status_t reVal        = kStatus_Fail; // default is fail
	uint8_t regAddr = 0x00U; // iterate thru registers

	I2C_MasterGetDefaultConfig(&masterConfig);

	I2C_MasterInit(I2C_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);

	while(regAddr < 249) {

		if (kStatus_Success
				== I2C_MasterStart(I2C_MASTER, I2C_MASTER_SLAVE_ADDR_7BIT,
						kI2C_Write)) {

			reVal = I2C_MasterWriteBlocking(I2C_MASTER, &regAddr, 1,
					kI2C_TransferNoStopFlag);
			if (reVal != kStatus_Success) {
				return -1;
			}

			reVal = I2C_MasterRepeatedStart(I2C_MASTER,
					I2C_MASTER_SLAVE_ADDR_7BIT, kI2C_Read);
			if (reVal != kStatus_Success) {
				return -1;
			}

			reVal = I2C_MasterReadBlocking(I2C_MASTER, g_master_rxBuff,
					I2C_DATA_LENGTH, kI2C_TransferNoStopFlag);
			if (reVal != kStatus_Success) {
				return -1;
			}

			// send stop signal
			reVal = I2C_MasterStop(I2C_MASTER);
			if (reVal != kStatus_Success) {
				return -1;
			}
		}

		PRINTF("reg addr: 0x%02x \r\n", regAddr);
		PRINTF("reg value: 0x");

		for (uint32_t k = 0U; k < I2C_DATA_LENGTH; k++) {
			PRINTF("%02x", g_master_rxBuff[k]);
		}
		PRINTF("\r\n\r\n");

		regAddr++;
	}
}


static int writeToReg(uint8_t regAddr, const void *txBuff){

	i2c_master_config_t masterConfig;
	status_t reVal = kStatus_Fail; // default is fail

	I2C_MasterGetDefaultConfig(&masterConfig);

	I2C_MasterInit(I2C_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);

	if (kStatus_Success
			== I2C_MasterStart(I2C_MASTER, I2C_MASTER_SLAVE_ADDR_7BIT,
					kI2C_Write)) {

		// send register addr
		reVal = I2C_MasterWriteBlocking(I2C_MASTER, &regAddr, 1U,
				kI2C_TransferNoStopFlag);
		if (reVal != kStatus_Success) {
			return -1;
		}
		// send data
		reVal = I2C_MasterWriteBlocking(I2C_MASTER, txBuff,
				I2C_DATA_LENGTH, kI2C_TransferNoStopFlag);
		if (reVal != kStatus_Success) {
			return -1;
		}
		reVal = I2C_MasterStop(I2C_MASTER);
		if (reVal != kStatus_Success) {
			return -1;
		}
	}

	// check that the write was successful



}

// further configure the codec to get it to actually play stuff
static void customCodecConfig(void){

	//DACDAT is not used when loopback is enabled, so we actually don't want to do this
	/*
	g_master_txBuff[0] = 0x02;
	g_master_txBuff[1] = 0x50;
	// set the loopback register
	writeToReg(R_AUDIO_INTERFACE_0, g_master_txBuff);
	*/
}

/*!
 * @brief Main function
 */
int main(void) {

// SETUP AND CLOCK STUFF

    /* set BOD VBAT level to 1.65V */
    POWER_SetBodVbatLevel(kPOWER_BodVbatLevel1650mv, kPOWER_BodHystLevel50mv, false);
    //TODO what are all of these clocks for?
    CLOCK_EnableClock(kCLOCK_InputMux);
    CLOCK_EnableClock(kCLOCK_Iocon);
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_Gpio1);

    /* USART0 clock */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* I2C clock */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);

    PMC->PDRUNCFGCLR0 |= PMC_PDRUNCFG0_PDEN_XTAL32M_MASK;   /*!< Ensure XTAL16M is on  */
    PMC->PDRUNCFGCLR0 |= PMC_PDRUNCFG0_PDEN_LDOXO32M_MASK;  /*!< Ensure XTAL16M is on  */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK; /*!< Ensure CLK_IN is on  */
    ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK;

    /*!< Switch PLL0 clock source selector to XTAL16M */
    CLOCK_AttachClk(kEXT_CLK_to_PLL0);

    const pll_setup_t pll0Setup = {
        .pllctrl = SYSCON_PLL0CTRL_CLKEN_MASK | SYSCON_PLL0CTRL_SELI(2U) | SYSCON_PLL0CTRL_SELP(31U),
        .pllndec = SYSCON_PLL0NDEC_NDIV(125U),
        .pllpdec = SYSCON_PLL0PDEC_PDIV(8U),
        .pllsscg = {0x0U, (SYSCON_PLL0SSCG1_MDIV_EXT(3072U) | SYSCON_PLL0SSCG1_SEL_EXT_MASK)},
        .pllRate = 24576000U,
        .flags   = PLL_SETUPFLAG_WAITLOCK};

    /*!< Configure PLL to the desired values */
    // TODO what is PLL?
    CLOCK_SetPLL0Freq(&pll0Setup);

    CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 0U, true);
    CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 1U, false);

    /* I2S clocks */
    CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM6);
    CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM7);

    /* Attach PLL clock to MCLK for I2S, no divider */
    CLOCK_AttachClk(kPLL0_to_MCLK);
    SYSCON->MCLKDIV = SYSCON_MCLKDIV_DIV(0U); // TODO what is SYSCON?
    SYSCON->MCLKIO  = 1U;

// PERIPHERAL CONFIG

    // TODO why do we have to reset the peripherals?
    RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn); /* reset FLEXCOMM for I2C */
    RESET_PeripheralReset(kDMA0_RST_SHIFT_RSTn); /* reset FLEXCOMM for DMA0 */
    RESET_PeripheralReset(kFC6_RST_SHIFT_RSTn); /* reset FLEXCOMM for I2S */
    RESET_PeripheralReset(kFC7_RST_SHIFT_RSTn);

    /* reset NVIC for FLEXCOMM6 and FLEXCOMM7 */
    // TODO what is NVIC?
    NVIC_ClearPendingIRQ(FLEXCOMM6_IRQn);
    NVIC_ClearPendingIRQ(FLEXCOMM7_IRQn);

    /* Enable interrupts for I2S */
    // TODO why do we need interrupts for I2S?
    EnableIRQ(FLEXCOMM6_IRQn);
    EnableIRQ(FLEXCOMM7_IRQn);

    /* Initialize the rest */
    BOARD_InitPins();
    BOARD_BootClockFROHF96M();
    BOARD_InitDebugConsole();
    BOARD_InitSysctrl();

// CONFIGURE THE CODEC

    PRINTF("Configure WM8904 codec\r\n");

    // TODO why is the bitwidth 16? Isn't the codec a 24-bit device?
    /* protocol: i2s
     * sampleRate: 48K
     * bitwidth:16
     */
    if (CODEC_Init(&codecHandle, &boardCodecConfig) != kStatus_Success)
    {
        PRINTF("WM8904_Init failed!\r\n");
        assert(false);
    }

    // add my own config stuff
    //customCodecConfig();

    /* Initial volume kept low for hearing safety.
     * Adjust it to your needs, 0-100, 0 for mute, 100 for maximum volume.
     */
    if (CODEC_SetVolume(&codecHandle, kCODEC_PlayChannelHeadphoneLeft | kCODEC_PlayChannelHeadphoneRight, VOLUME) !=
        kStatus_Success)
    {
        assert(false);
    }

    // TODO check that codec registers have the right values in them
   // printRegValues();

// CONFIGURE & INIT I2S

    PRINTF("Configure I2S\r\n");

    /*
     * masterSlave = kI2S_MasterSlaveNormalMaster;
     * mode = kI2S_ModeI2sClassic;
     * rightLow = false;
     * leftJust = false;
     * pdmData = false;
     * sckPol = false;
     * wsPol = false;
     * divider = 1;
     * oneChannel = false;
     * dataLength = 16;
     * frameLength = 32;
     * position = 0;
     * watermark = 4;
     * txEmptyZero = true;
     * pack48 = false;
     */
    I2S_TxGetDefaultConfig(&s_TxConfig);
    s_TxConfig.divider     = I2S_CLOCK_DIVIDER;
    s_TxConfig.masterSlave = I2S_TX_MODE;

    I2S_TxInit(I2S_TX, &s_TxConfig);

// INIT DMA

    DMA_Init(DEMO_DMA);
    DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL);
    DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL, kDMA_ChannelPriority3); // TODO why priority 3?
    DMA_CreateHandle(&s_DmaTxHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL);

// SEND DATA TO CODEC AND PLAY IT


   //printRegValues();

    StartSoundPlayback();

    while (1)
    {
    }
}

