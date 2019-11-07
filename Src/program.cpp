#include "main.h"
#include "program.h"
#include "stm32f1xx_ll_usart.h"
#include <cmath>

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_tx;

ws2812b_color ledBuffer[PIXELS];
#define zero 0b11000000
#define one 0b11111000

Status status;

union Command {
	uint8_t data[PIXELS * 3 + 1];
	struct {
		uint8_t pixels;
		ws2812b_color colors[PIXELS];
	};
}__attribute__((packed));

Command command;
int rxPos;

void setPixels() {
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

uint8_t buffer[24 * PIXELS + 10];

extern "C" void run() {
	rxPos = 0;
	command.pixels = 0;
	status.enabled = 1;
	status.hasUpdate = 0;
	status.lastRx = HAL_GetTick();
	status.receiving = 0;
	float p = 0;
	LL_USART_TransmitData8(USART1, '!');

	while (1) {
		p += 0.1;
		if (status.hasUpdate) {
			LL_USART_DisableIT_RXNE (USART1);
			setPixels();
			LL_USART_EnableIT_RXNE(USART1);
			int b = 0;
			for (int led = 0; led < status.maxPixels; led++) {
				for (int i = 7; i >= 0; i--) {
					buffer[b++] = ledBuffer[led].green & (1 << i) ? one : zero;
				}
				for (int i = 7; i >= 0; i--) {
					buffer[b++] = ledBuffer[led].red & (1 << i) ? one : zero;
				}
				for (int i = 7; i >= 0; i--) {
					buffer[b++] = ledBuffer[led].blue & (1 << i) ? one : zero;
				}
			}
			buffer[b] = 0;
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
			HAL_SPI_Transmit_DMA(&hspi1, buffer, 3 * 8 * status.maxPixels + 1);
		} else {
			if (HAL_DMA_GetState(&hdma_spi1_tx) != HAL_DMA_STATE_BUSY) {
				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
			}
		}
//		HAL_Delay(8);
	}
}

extern "C" void rxHandle() {
	if (LL_USART_IsActiveFlag_ORE (USART1)) {
		LL_USART_ClearFlag_ORE(USART1);
		rxPos = 0;
		status.hasUpdate = 0;
		return;
	}
	if (LL_USART_IsActiveFlag_RXNE (USART1)) {
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

