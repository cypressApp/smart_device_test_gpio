
// 34 35 36 37 38 39 are INPUT olny pins

#define GPIO_OUTPUT_1  33
#define GPIO_OUTPUT_2  25
#define GPIO_OUTPUT_3  32
#define GPIO_OUTPUT_4  26


#define GPIO_OUTPUT_PIN_SEL    ( (1ULL<<GPIO_OUTPUT_1) |  (1ULL<<GPIO_OUTPUT_2) | (1ULL<<GPIO_OUTPUT_3) | (1ULL<<GPIO_OUTPUT_4) )

void init_gpio_pins(void);

