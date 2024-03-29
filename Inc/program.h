
#ifdef __cplusplus
extern "C" {
#endif

#define PIXELS 150

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
	uint32_t rxPos;
} Status;

extern Status status;

typedef union Command {
	uint8_t data[PIXELS * 3 + 3 + 2 + 1];
	struct {
		char magic[3];
		uint8_t pixels_hi;
		uint8_t pixels_lo;
		uint8_t checksum;
		ws2812b_color colors[PIXELS];
	};
}__attribute__((packed)) Command;

extern Command command;


#define WS_ZERO 0b11000000
#define WS_ONE 0b11111000

void run();

void addChar(uint8_t data);

#ifdef __cplusplus
}
#endif
