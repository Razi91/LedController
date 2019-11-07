
#ifdef __cplusplus
extern "C" {
#endif

#define PIXELS 120

void rxHandle();

typedef struct ws2812b_color {
	uint8_t red, green, blue;
} ws2812b_color __attribute__((packed));

typedef struct Status {
	uint32_t lastRx;
	uint8_t enabled;
	uint8_t receiving;
	uint8_t hasUpdate;
	uint8_t maxPixels;
} Status;

extern Status status;

union Command {
	uint8_t data[PIXELS * 3 + 1];
	struct {
		uint8_t pixels;
		ws2812b_color colors[PIXELS];
	};
}__attribute__((packed));


#define WS_ZERO 0b11000000
#define WS_ONE 0b11111000

#ifdef __cplusplus
}
#endif
