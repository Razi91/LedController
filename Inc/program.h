
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

#ifdef __cplusplus
}
#endif
