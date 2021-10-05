#include "main.h"
#include "program.h"
#include "stm32f1xx_ll_usart.h"
#include "usbd_cdc_if.h"
#include "usbd_cdc.h"
#include <cmath>

extern USBD_HandleTypeDef hUsbDeviceFS;
extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;

ws2812b_color ledBuffer[PIXELS];
uint8_t spiBuffer[24 * PIXELS + 10];

Status status;
Command command;

void copyPixelsToBuffer() {
	const auto pixels = command.pixels_lo + (command.pixels_hi << 8);
	if (pixels > PIXELS) {
		command.pixels_lo = 0;
		return;
	}
	for (int i = 0; i < pixels; i++) {
		ledBuffer[i] = command.colors[i];
	}
	status.maxPixels = status.maxPixels > pixels ? status.maxPixels : pixels;
	command.pixels_lo = 0;
	status.hasUpdate = 0;
}

extern "C" void run() {
	status.rxPos = 0;
	command.pixels_lo = 0;
	status.enabled = 1;
	status.hasUpdate = 0;
	status.lastRx = HAL_GetTick();
	status.receiving = 0;
	LL_USART_TransmitData8(USART1, '!');

	USBD_CDC_ReceivePacket(&hUsbDeviceFS);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

	while (1) {
		//CDC_Transmit_FS((uint8_t*)"?", 1);
		if (HAL_DMA_GetState(&hdma_spi1_tx) == HAL_DMA_STATE_BUSY) {
//			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
			continue;
		}

		auto now = HAL_GetTick();
//		const auto isOutdated = now - status.lastRx > 2;
		const auto update = status.hasUpdate;

		if (update) {
			LL_USART_TransmitData8(USART1, '2');
			LL_USART_DisableIT_RXNE(USART1);
			copyPixelsToBuffer();
			LL_USART_EnableIT_RXNE(USART1);
			int bit = 0;
			for (int led = 0; led < status.maxPixels; led++) {
//				for (int i = 7; i >= 0; i--) {
//					spiBuffer[bit++] = WS_ZERO;
//				}
//				for (int i = 7; i >= 0; i--) {
//					spiBuffer[bit++] = WS_ONE;
//				}
//				for (int i = 7; i >= 0; i--) {
//					spiBuffer[bit++] = WS_ZERO;
//				}

				for (int i = 7; i >= 0; i--) {
					spiBuffer[bit++] =
							ledBuffer[led].green & (1 << i) ? WS_ONE : WS_ZERO;
				}
				for (int i = 7; i >= 0; i--) {
					spiBuffer[bit++] =
							ledBuffer[led].red & (1 << i) ? WS_ONE : WS_ZERO;
				}
				for (int i = 7; i >= 0; i--) {
					spiBuffer[bit++] =
							ledBuffer[led].blue & (1 << i) ? WS_ONE : WS_ZERO;
				}
			}
			spiBuffer[bit] = 0;
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
//			CDC_Transmit_FS((uint8_t*) "SET\r\n", 5);
			HAL_SPI_Transmit_DMA(&hspi1, spiBuffer,
					3 * 8 * status.maxPixels + 1);
			{
				char buf[16];
				sprintf(buf, "\nD:%d\n", status.maxPixels);
				for (int i = 0; i < strlen(buf); i++) {
					while (!LL_USART_IsActiveFlag_TXE(USART1))
						;
					LL_USART_TransmitData8(USART1, buf[i]);
				}
			}
			status.hasUpdate = false;
			status.rxPos = 0;
			status.lastRx = now;
		} else {
			if (HAL_DMA_GetState(&hdma_spi1_tx) != HAL_DMA_STATE_BUSY) {
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
			}
		}
	}
}

const uint8_t magic[] = { 'A', 'd', 'a' };
#define MAGICSIZE sizeof(magic)

extern "C" void addChar(uint8_t data) {
	if (status.hasUpdate)
		return;

	auto now = HAL_GetTick();
	if (status.rxPos > 0 && now - status.lastRx > 2) {
		status.rxPos = 0;
	}
	status.lastRx = now;
	command.data[status.rxPos] = data;

	// header
	if (status.rxPos < MAGICSIZE + 3) {
		if (status.rxPos < MAGICSIZE) {
			if (data != magic[status.rxPos]) {
				LL_USART_TransmitData8(USART1, 'A' + status.rxPos);
				status.rxPos = 0;
				status.hasUpdate = 0;
				return;
			}
			LL_USART_TransmitData8(USART1, '+' + status.rxPos);
		} else if (status.rxPos == MAGICSIZE + 2) {
			status.maxPixels = command.pixels_lo + command.pixels_hi * 0xff;
			const auto check = command.pixels_hi ^ command.pixels_lo ^ 0x55;
			if (data != check) {
				status.rxPos = 0;
				status.hasUpdate = 0;
				LL_USART_TransmitData8(USART1, 'R');
			} else {
				LL_USART_TransmitData8(USART1, 'D');
			}
		}

	}
	status.rxPos++;

	if (status.rxPos == command.pixels_lo * 3 + MAGICSIZE + 3) {
		LL_USART_TransmitData8(USART1, 'E');
		status.hasUpdate = 1;
		status.rxPos = 0;
	}
}

extern "C" void rxHandle() {
	if (LL_USART_IsActiveFlag_ORE(USART1)) {
		// overrun
		LL_USART_ClearFlag_ORE(USART1);
//		status.rxPos = 0;
//		status.hasUpdate = 0;
		return;
	}
	if (LL_USART_IsActiveFlag_RXNE(USART1)) {
		uint8_t data = LL_USART_ReceiveData8(USART1);
		addChar(data);
	}
}

