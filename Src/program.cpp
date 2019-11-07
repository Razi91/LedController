#include "main.h"
#include "program.h"
#include "stm32f1xx_ll_usart.h"
#include <cmath>

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;

ws2812b_color ledBuffer[PIXELS];

Status status;
Command command;
int rxPos;

void copyPixelsToBuffer() {
	if (command.pixels > PIXELS) {
		command.pixels = 0;
		return;
	}
	for (int i = 0; i < command.pixels; i++) {
		ledBuffer[i] = command.colors[i];
	}
	status.maxPixels =
			status.maxPixels > command.pixels ?
					status.maxPixels : command.pixels;
	command.pixels = 0;
	status.hasUpdate = 0;
}

uint8_t spiBuffer[24 * PIXELS + 10];

extern "C" void run() {
	rxPos = 0;
	command.pixels = 0;
	status.enabled = 1;
	status.hasUpdate = 0;
	status.lastRx = HAL_GetTick();
	status.receiving = 0;
	LL_USART_TransmitData8(USART1, '!');

	while (1) {
		if (HAL_DMA_GetState(&hdma_spi1_tx) != HAL_DMA_STATE_BUSY) {
			continue;
		}
		if (status.hasUpdate) {
			LL_USART_DisableIT_RXNE (USART1);
			copyPixelsToBuffer();
			LL_USART_EnableIT_RXNE(USART1);
			int bit = 0;
			for (int led = 0; led < status.maxPixels; led++) {
				for (int i = 7; i >= 0; i--) {
					spiBuffer[bit++] = ledBuffer[led].green & (1 << i) ? WS_ONE : WS_ZERO;
				}
				for (int i = 7; i >= 0; i--) {
					spiBuffer[bit++] = ledBuffer[led].red & (1 << i) ? WS_ONE : WS_ZERO;
				}
				for (int i = 7; i >= 0; i--) {
					spiBuffer[bit++] = ledBuffer[led].blue & (1 << i) ? WS_ONE : WS_ZERO;
				}
			}
			spiBuffer[bit] = 0;
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
			HAL_SPI_Transmit_DMA(&hspi1, spiBuffer, 3 * 8 * status.maxPixels + 1);
		} else {
			if (HAL_DMA_GetState(&hdma_spi1_tx) != HAL_DMA_STATE_BUSY) {
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
			}
		}
	}
}

extern "C" void rxHandle() {
	if (LL_USART_IsActiveFlag_ORE (USART1)) {
		// overrun
		LL_USART_ClearFlag_ORE(USART1);
		rxPos = 0;
		status.hasUpdate = 0;
		return;
	}
	if (LL_USART_IsActiveFlag_RXNE (USART1)) {
		// has data
		auto now = HAL_GetTick();
		if (rxPos > 0 && now - status.lastRx > 2) {
			rxPos = 0;
		}
		status.lastRx = now;
		command.data[rxPos] = LL_USART_ReceiveData8(USART1);
		rxPos++;
		if (rxPos > 1) {
			if (rxPos == 1 + command.pixels * 3) {
				status.hasUpdate = 1;
				rxPos = 0;
			}
		}
	}
}

