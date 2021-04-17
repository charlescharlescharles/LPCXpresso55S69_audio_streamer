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
#include "fsl_usart.h"
#include <stdbool.h>
#include "fsl_sysctl.h"
#include "fsl_codec_adapter.h"
#include "fsl_power.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_common.h"
#include "fsl_iocon.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define I2C                        		(I2C4)
#define I2S_MASTER_CLOCK_FREQUENCY 		(24576000)
#define I2S_TX                     		(I2S7)
#define I2S_RX                     		(I2S6)
#define DEMO_DMA                   		(DMA0)
#define DEMO_I2S_TX_CHANNEL        		(19)
#define I2S_RX_CHANNEL             		(16)
#define I2S_TX_MODE                		(kI2S_MasterSlaveNormalMaster)
#define AUDIO_PROTOCOL             		(kCODEC_BusI2S)
#define VOLUME 							(50U)
#define SAMPLE_RATE						(kWM8904_SampleRate12kHz)
#define BIT_WIDTH						(kWM8904_BitWidth16)
#define BAUDRATE						(460800U)
#define I2C_MASTER_SLAVE_ADDR_7BIT 		(0x1AU)
#define I2C_MASTER 						((I2C_Type *)I2C4_BASE)
#define I2C_MASTER_CLOCK_FREQUENCY 		(12000000)
#define I2C_DATA_LENGTH            		(2U)
#define BYTES_PER_SAMPLE				(2U)
#define I2S_CLOCK_DIVIDER				(85U)
#if (BYTES_PER_SAMPLE == 3)
	#define BUF_SIZE					(4080U)
#else
	#define BUF_SIZE					(4096U)
#endif
#define UART_TX_SIZE					(16U)
#define USART                			(USART2)
#define REQUEST_SIGNAL 					(0xFF)
#define FAULT_SIGNAL					(0xFE)
#define FLEXCOMM2_PERIPHERAL 			((USART_Type *)FLEXCOMM2)
#define FLEXCOMM2_CLOCK_SOURCE 			(12000000UL)
#define PIO0_27_DIGIMODE_DIGITAL 		(0x01U)
#define PIO0_27_FUNC_ALT1 				(0x01U)
#define PIO1_24_DIGIMODE_DIGITAL 		(0x01U)
#define PIO1_24_FUNC_ALT1 				(0x01U)
#define PLLOUT							(24576000U)
#define SIGNAL_PLAY						(0xFDU)
#define SIGNAL_STOP						(0xFCU)

/*******************************************************************************
 * Variables
 ******************************************************************************/


typedef struct audio_characteristics {
	wm8904_bit_width_t bit_depth;
	wm8904_sample_rate_t sampling_frequency;
	uint8_t num_channels;
}audio_t;

audio_t audio;
uint8_t bit_depth_val;
uint32_t sampling_frequency_val;

static dma_handle_t s_DmaTxHandle;
static i2s_config_t s_TxConfig;
static i2s_dma_handle_t s_TxHandle;
static i2s_transfer_t s_TxTransfer;
extern codec_config_t boardCodecConfig;
codec_handle_t codecHandle;

uint8_t g_master_txBuff[I2C_DATA_LENGTH];
uint8_t g_master_rxBuff[I2C_DATA_LENGTH];

// double buffer vars
uint8_t g_data_buffer_A[BUF_SIZE]; // alternate between buffer A and B
uint8_t g_data_buffer_B[BUF_SIZE];
uint8_t * rx_buf 				= g_data_buffer_A; // pointer to the buffer we are reading data into
uint8_t * tx_buf 				= g_data_buffer_B;
uint8_t * rx_buf_ptr;
static bool rx_buf_full = false; // TODO is this necessary anymore?

static uint8_t g_UART_req_data[3] = {REQUEST_SIGNAL, (uint8_t)(((BUF_SIZE / BYTES_PER_SAMPLE) >> 8)&0xFF), (uint8_t)((BUF_SIZE / BYTES_PER_SAMPLE) & 0xFF)};
static uint8_t g_UART_baud_check[4] = {REQUEST_SIGNAL, (uint8_t)((BAUDRATE >> 16)&0xFF), (uint8_t)((BAUDRATE >> 8)&0xFF) ,(uint8_t)((BAUDRATE) & 0xFF)};

enum playback_state { PLAY, STOP };
enum playback_state state = STOP;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void StartSoundPlayback(void);

static void TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData);

static void printRegVals(void);

static void customCodecConfig(void);

static int writeToReg(uint8_t regAddr, const void *txBuff);

static void refill_rx_buf();

static void stream_tx_buf(void);

void I2S_TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData);

void StartPlayback(void);

static void swap_buffers(void);

void setup(void);

void BOARD_InitSysctrl(void);

status_t set_baudrate(uint32_t baudrate);

void get_audio_characteristics(audio_t * audio);

uint8_t get_i2s_div(audio_t * audio);

void baud_check(void);

void init_CODEC(void);

void init_UART(void);

void receive_state(void);

/*******************************************************************************
 * Code
 ******************************************************************************/


/*
 * Change the baud rate after the USART has been initialized
 * TODO test
 */
status_t set_baudrate(uint32_t baudrate)
{
	return USART_SetBaudRate(USART, baudrate, 12000000U );
}

uint8_t get_i2s_div(audio_t * audio)
{
	PRINTF("i2s div: %d", (PLLOUT / (bit_depth_val * sampling_frequency_val)) - 1);
	return (PLLOUT / (bit_depth_val * sampling_frequency_val)) - 1;
}


/*
 * get the bit depth and sampling frequency over UART
 */
void get_audio_characteristics(audio_t * audio)
{

	USART_WriteByte(USART, (uint8_t)0xFF);

	USART_ReadBlocking(USART, &bit_depth_val, 1);
	PRINTF("%d\r\n", bit_depth_val);
	USART_ReadBlocking(USART, (uint8_t *)(&(sampling_frequency_val)), 4);
	USART_ReadBlocking(USART, (uint8_t * )(&(audio->num_channels)), 1);

	switch (bit_depth_val)
	{
		case ( 16U ) :
			audio->bit_depth = kWM8904_BitWidth16;
			break;
		case ( 20U ) :
			audio->bit_depth = kWM8904_BitWidth20;
			break;
		case ( 24U ) :
			audio->bit_depth = kWM8904_BitWidth24;
			break;
		case ( 32U ) :
			audio->bit_depth = kWM8904_BitWidth32;
			break;
		default :
			PRINTF("Invalid bit depth: %d \r\n", bit_depth_val);
			assert(0);
	}

	switch (sampling_frequency_val)
	{
		case ( 8000U ) :
			audio->sampling_frequency = kWM8904_SampleRate8kHz;
			break;
		case ( 11025U ) :
			audio->sampling_frequency = kWM8904_SampleRate12kHz;
			break;
		case ( 12000U ) :
			audio->sampling_frequency = kWM8904_SampleRate12kHz;
			break;
		case ( 16000U ) :
			audio->sampling_frequency = kWM8904_SampleRate16kHz;
			break;
		case ( 24000U ) :
			audio->sampling_frequency = kWM8904_SampleRate24kHz;
			break;
		case ( 32000U ) :
			audio->sampling_frequency = kWM8904_SampleRate32kHz;
			break;
		case ( 48000U ) :
			audio->sampling_frequency = kWM8904_SampleRate48kHz;
			break;
		default :
			PRINTF("Invalid samling frequency: %d \r\n", sampling_frequency_val);
			assert(0);
	}

	// TODO reject incompatible types
	PRINTF("bit depth: %d\r\n", bit_depth_val);
	PRINTF("sampling frequency: %d\r\n", sampling_frequency_val);
	PRINTF("number of channels: %d\r\n", audio->num_channels);


}

/*
 * Send the buffer size so that python script knows how many bytes to write
 */
void send_buffer_size(void)
{
	USART_WriteBlocking(USART, (uint8_t *)g_UART_req_data, UART_TX_SIZE);
}

/*
 * Make sure baud rates are the same in C and python components of app
 */
void baud_check(void)
{
	uint8_t buf[1] = {0};
	USART_WriteBlocking(USART, (uint8_t *)g_UART_baud_check, UART_TX_SIZE);

	USART_ReadBlocking(USART, buf, 1);

	if (buf[0] == FAULT_SIGNAL){

		PRINTF("baud rates are not the same in MCU and python components\r\n");
		assert(0);
	}
}


/*
 *
 */
static void refill_rx_buf()
{

	NVIC_DisableIRQ(DMA0_IRQn);



	rx_buf_ptr = rx_buf;

	// send ready signal
	USART_WriteBlocking(USART, (uint8_t *)g_UART_req_data, UART_TX_SIZE);


	// get number of bytes incoming

	receive_state();


	if (state == STOP){
		NVIC_EnableIRQ(DMA0_IRQn);
		return;
	}

	// otherwise continue

	while(rx_buf_ptr < rx_buf + BUF_SIZE){ // TODO will this get stuck if last transfer is < 4096? fudge

		USART_ReadBlocking(USART, rx_buf_ptr, UART_TX_SIZE);
		rx_buf_ptr += UART_TX_SIZE;
	}

	rx_buf_full = true;



	NVIC_EnableIRQ(DMA0_IRQn);
}

/*
 *
 */
static void stream_tx_buf(void)
{

	s_TxTransfer.data     = tx_buf;
    s_TxTransfer.dataSize = sizeof(g_data_buffer_A);

	I2S_TxTransferCreateHandleDMA(I2S_TX, &s_TxHandle, &s_DmaTxHandle, I2S_TxCallback, (void *)&s_TxTransfer);
    I2S_TxTransferSendDMA(I2S_TX, &s_TxHandle, s_TxTransfer);
}

/*
 * Get the current state from the python end of the app (pause or play)
 */
void receive_state(void)
{
	uint8_t state_val;
	USART_ReadBlocking(USART, &state_val, 1 );

	if (state_val == SIGNAL_PLAY){
		state = PLAY;
	}else if (state_val == SIGNAL_STOP){
		state = STOP;
	}

}

/*
 *
 */
void I2S_TxCallback(I2S_Type *base, i2s_dma_handle_t *handle, status_t completionStatus, void *userData)
{

	// check state
/*
	receive_state();
	if (state == STOP){
		PRINTF("stop");
		return;
	}
*/
	if (state == PLAY){

		swap_buffers();

		rx_buf_full = false;

		stream_tx_buf();

		refill_rx_buf();
	}

}

/*
 *
 */
static void swap_buffers(void)
{
	if (rx_buf == g_data_buffer_A)
	{
		rx_buf = g_data_buffer_B;
		tx_buf = g_data_buffer_A;

	}else
	{
		rx_buf = g_data_buffer_A;
		tx_buf = g_data_buffer_B;
	}
}

/*
 *
 */
void StartPlayback(void)
{
	state = PLAY;

	// fill both buffers
	refill_rx_buf();
	swap_buffers();
	refill_rx_buf();
	// start I2S callback chaining
	stream_tx_buf();
}

/***********************************************************************************************************************
 * I2C functions
 **********************************************************************************************************************/

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


/***********************************************************************************************************************
 * Setup functions
 **********************************************************************************************************************/

void init_UART(void)
{

	// PINS
	IOCON->PIO[0][27] = ((IOCON->PIO[0][27] &
	                          /* Mask bits to zero which are setting */
	                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

	                         /* Selects pin function.
	                          * : PORT027 (pin 27) is configured as FC2_TXD_SCL_MISO_WS. */
	                         | IOCON_PIO_FUNC(PIO0_27_FUNC_ALT1)

	                         /* Select Digital mode.
	                          * : Enable Digital mode.
	                          * Digital input is enabled. */
	                         | IOCON_PIO_DIGIMODE(PIO0_27_DIGIMODE_DIGITAL));

	    const uint32_t port0_pin29_config = (/* Pin is configured as FC0_RXD_SDA_MOSI_DATA */
	                                         IOCON_PIO_FUNC1 |
	                                         /* No addition pin function */
	                                         IOCON_PIO_MODE_INACT |
	                                         /* Standard mode, output slew rate control is enabled */
	                                         IOCON_PIO_SLEW_STANDARD |
	                                         /* Input function is not inverted */
	                                         IOCON_PIO_INV_DI |
	                                         /* Enables digital function */
	                                         IOCON_PIO_DIGITAL_EN |
	                                         /* Open drain is disabled */
	                                         IOCON_PIO_OPENDRAIN_DI);
	    /* PORT0 PIN29 (coords: 92) is configured as FC0_RXD_SDA_MOSI_DATA */
	    IOCON_PinMuxSet(IOCON, 0U, 29U, port0_pin29_config);

	    const uint32_t port0_pin30_config = (/* Pin is configured as FC0_TXD_SCL_MISO_WS */
	                                         IOCON_PIO_FUNC1 |
	                                         /* No addition pin function */
	                                         IOCON_PIO_MODE_INACT |
	                                         /* Standard mode, output slew rate control is enabled */
	                                         IOCON_PIO_SLEW_STANDARD |
	                                         /* Input function is not inverted */
	                                         IOCON_PIO_INV_DI |
	                                         /* Enables digital function */
	                                         IOCON_PIO_DIGITAL_EN |
	                                         /* Open drain is disabled */
	                                         IOCON_PIO_OPENDRAIN_DI);
	    /* PORT0 PIN30 (coords: 94) is configured as FC0_TXD_SCL_MISO_WS */
	    IOCON_PinMuxSet(IOCON, 0U, 30U, port0_pin30_config);

	    IOCON->PIO[1][24] = ((IOCON->PIO[1][24] &
	                          /* Mask bits to zero which are setting */
	                          (~(IOCON_PIO_FUNC_MASK | IOCON_PIO_DIGIMODE_MASK)))

	                         /* Selects pin function.
	                          * : PORT124 (pin 3) is configured as FC2_RXD_SDA_MOSI_DATA. */
	                         | IOCON_PIO_FUNC(PIO1_24_FUNC_ALT1)

	                         /* Select Digital mode.
	                          * : Enable Digital mode.
	                          * Digital input is enabled. */
	                         | IOCON_PIO_DIGIMODE(PIO1_24_DIGIMODE_DIGITAL));


// CLOCKS

	    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);                 /*!< Switch FLEXCOMM2 to FRO12M */

// PERIPHERALs


	    const usart_config_t FLEXCOMM2_config = {
	      .baudRate_Bps = BAUDRATE,
	      .syncMode = kUSART_SyncModeDisabled,
	      .parityMode = kUSART_ParityDisabled,
	      .stopBitCount = kUSART_OneStopBit,
	      .bitCountPerChar = kUSART_8BitsPerChar,
	      .loopback = false,
	      .txWatermark = kUSART_TxFifo0,
	      .rxWatermark = kUSART_RxFifo1,
	      .enableRx = true,
	      .enableTx = true,
	      .enableHardwareFlowControl = false,
	      .enableMode32k = false,
	      .clockPolarity = kUSART_RxSampleOnFallingEdge,
	      .enableContinuousSCLK = false
	    };

	    RESET_PeripheralReset(kFC2_RST_SHIFT_RSTn);
	    USART_Init(FLEXCOMM2_PERIPHERAL, &FLEXCOMM2_config, FLEXCOMM2_CLOCK_SOURCE);


}

void setup(void)
{
// SETUP AND CLOCK STUFF

	/* set BOD VBAT level to 1.65V */
	POWER_SetBodVbatLevel(kPOWER_BodVbatLevel1650mv, kPOWER_BodHystLevel50mv, false);
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
	CLOCK_SetPLL0Freq(&pll0Setup);

	CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 0U, true);
	CLOCK_SetClkDiv(kCLOCK_DivPll0Clk, 1U, false);

	/* I2S clocks */
	CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM6);
	CLOCK_AttachClk(kPLL0_DIV_to_FLEXCOMM7);

	/* Attach PLL clock to MCLK for I2S, no divider */
	CLOCK_AttachClk(kPLL0_to_MCLK);
	SYSCON->MCLKDIV = SYSCON_MCLKDIV_DIV(0U);
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

	EnableIRQ(FLEXCOMM6_IRQn);
	EnableIRQ(FLEXCOMM7_IRQn);

	/* Initialize the rest */
	BOARD_InitPins();
	BOARD_BootClockFROHF96M();
	BOARD_InitDebugConsole();
	BOARD_InitSysctrl();



// INIT UART
	init_UART();

	baud_check();

	init_CODEC();

	send_buffer_size();

}
/*
 * Initialize the codec, as well as I2S and DMA for sending data to codec
 */
void init_CODEC(void)
{

	get_audio_characteristics(&audio);

	wm8904_config_t wm8904Config = {
	    .i2cConfig    = {.codecI2CInstance = BOARD_CODEC_I2C_INSTANCE, .codecI2CSourceClock = BOARD_CODEC_I2C_CLOCK_FREQ},
	    .recordSource = kWM8904_RecordSourceLineInput,
	    .recordChannelLeft  = kWM8904_RecordChannelLeft2,
	    .recordChannelRight = kWM8904_RecordChannelRight2,
	    .playSource         = kWM8904_PlaySourceDAC,
	    .slaveAddress       = WM8904_I2C_ADDRESS,
	    .protocol           = kWM8904_ProtocolI2S,
	    .format             = {.sampleRate = audio.sampling_frequency, .bitWidth =audio.bit_depth},
	    .mclk_HZ            = I2S_MASTER_CLOCK_FREQUENCY,
	    .master             = false,
	};

	codec_config_t boardCodecConfig = {.codecDevType = kCODEC_WM8904, .codecDevConfig = &wm8904Config};

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

	/* Initial volume kept low for hearing safety.
	 * Adjust it to your needs, 0-100, 0 for mute, 100 for maximum volume.
	 */
	if (CODEC_SetVolume(&codecHandle, kCODEC_PlayChannelHeadphoneLeft | kCODEC_PlayChannelHeadphoneRight, VOLUME) !=
		kStatus_Success)
	{
		assert(false);
	}

// CONFIGURE & INIT I2S

	PRINTF("Configure I2S\r\n");


	uint8_t i2s_clock_divider = get_i2s_div(&audio);



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
	s_TxConfig.divider     = i2s_clock_divider;
	s_TxConfig.masterSlave = I2S_TX_MODE;

	I2S_TxInit(I2S_TX, &s_TxConfig);

// INIT DMA

	DMA_Init(DEMO_DMA);
	DMA_EnableChannel(DEMO_DMA, DEMO_I2S_TX_CHANNEL);
	DMA_SetChannelPriority(DEMO_DMA, DEMO_I2S_TX_CHANNEL, kDMA_ChannelPriority3); // TODO why priority 3?
	DMA_CreateHandle(&s_DmaTxHandle, DEMO_DMA, DEMO_I2S_TX_CHANNEL);
}


void BOARD_InitSysctrl(void)
{

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


/***********************************************************************************************************************
 * Main
 **********************************************************************************************************************/

/*
 * Wait for a track to play
 */
void StartTrack(void)
{


	// wait for start signal

	// get file specifics

	// init codec

	// play until stop signal

	// deinit codec


	PRINTF("inside startTrack");


	uint8_t state_val;
/*
	// wait for start signal
	while(1)
	{
		USART_ReadBlocking(USART, &state_val, 1 );
		if (state_val == SIGNAL_PLAY){
			break;
		}
	}
*/


	StartPlayback();

	while (state == PLAY)
	{

	}
}



/*!
 * @brief Main function
 */
int main(void) {

	setup();

	//StartPlayback();

	//while(1){}


	while(1)
	{
		StartTrack();
	}

	//StartPlayback();
	/*
    while (1)
    {
    	if (state == STOP){
    		//USART_WriteByte(USART, (uint8_t)0xFF);
    		receive_state();
    		if (state == PLAY){
    			StartPlayback();
    		}

    	}
    }
    */
}

