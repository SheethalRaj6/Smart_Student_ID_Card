/*
 * ============================================================================
 *  SMART STUDENT ID  -  complete firmware in ONE file
 *  Board : STM32F401RCT6 / STM32F401CCU6 "Black Pill" (25 MHz crystal)
 *  Tools : Keil uVision 5, Arm Compiler 6
 * ============================================================================
  */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROVIDE_SYSTEMINIT      0   /* 0 = RTE Device:Startup supplies SystemInit */

#ifndef BOARD_H
#define BOARD_H
/* =====================================================================
 * COMPONENT TEST MODE
 *   0  = normal application
 *   1  = I2C scan (expect 27 or 3F, 56, 57, 68)
 *   2  = buzzer
 *   3  = buttons (Mode + SOS)
 *   4  = RC522 version + card UID
 *   5  = DS3231 date/time
 *   6  = MAX30102 IR value + BPM
 *   7  = KY-037 sound peak-to-peak + raw ADC
 *   8  = HC-SR04 distance
 *   9  = ESP8266 AT + AT+GMR
 *   10 = LM35 temperature
 *   11 = pin toggle: every project pin 0 V / 3.3 V each second (unplug modules)
 *   12 = RC522 deep debug: SPI patterns, self test, registers, live card probe
 * ===================================================================== */
#define TEST_COMPONENT          0

/* =====================================================================
 * NETWORK  (phone hotspot must be 2.4 GHz; ESP8266 has no 5 GHz radio)
 * SERVER_IP = the laptop's IPv4 address on the hotspot (server prints it)
 * ===================================================================== */
#define WIFI_SSID               "YOUR_HOTSPOT_NAME"
#define WIFI_PASS               "YOUR_HOTSPOT_PASSWORD"
#define SERVER_IP               "192.168.x.x"
#define SERVER_PORT             5000
#define ESP_BAUD                115200u

/* =====================================================================
 * APPLICATION SETTINGS
 * ===================================================================== */
#define MEAL_COST               50          /* Rs per meal                    */

#define BOOT_MSG_MS             1500u       /* each boot message              */
#define ESP_BOOT_WAIT_MS        1500u       /* let NodeMCU finish booting     */
#define RESULT_MSG_MS           2500u       /* card result on LCD             */
#define ALERT_MSG_MS            3000u       /* SOS / noise / entry on LCD     */

#define CARD_REPEAT_BLOCK_MS    2500u       /* ignore same card re-tap        */
#define RFID_POLL_MS            80u

#define NOISE_THRESHOLD_P2P     400         /* ADC counts peak-to-peak (tune with test 7) */
#define NOISE_WINDOW_MS         30u
#define NOISE_POLL_MS           100u
#define NOISE_COOLDOWN_MS       5000u

#define ENTRY_DISTANCE_CM       30          /* closer than this = entry (tune with test 8) */
#define ENTRY_POLL_MS           150u
#define ENTRY_COOLDOWN_MS       4000u

#define FINGER_IR_THRESHOLD     50000UL     /* MAX30102 IR above this = finger present */
#define HEALTH_POST_PERIOD_MS   10000u
#define TEMP_READ_PERIOD_MS     1000u
#define TEMP_POST_PERIOD_MS     60000u
#define WIFI_RETRY_PERIOD_MS    60000u

/* DS3231: when the RTC has lost power (OSF flag), load the build time.
 * RTC_FORCE_SET 1 = always load build time on reset (use once, then set 0). */
#define RTC_SET_FROM_BUILD_TIME 1
#define RTC_FORCE_SET           0

/* Button modules (G-V-S): the pressed level is detected automatically at
 * start-up (do not hold the button while the board boots).
 * BUTTON_ACTIVE_LOW only selects the internal pull resistor. */
#define BUTTON_ACTIVE_LOW       1

/* ONE BUTTON for both functions (button on PB9, nothing on PB8):
 *   1 press            -> toggle ATTENDANCE / MESS PAYMENT
 *   SOS_PRESS_COUNT    -> SOS alert (quick presses, each within CLICK_GAP_MS)
 * Set SINGLE_BUTTON 0 to use two buttons (Mode on PB8, SOS on PB9). */
#define SINGLE_BUTTON           1
#define SOS_PRESS_COUNT         3
#define CLICK_GAP_MS            600u    /* pause that ends a press sequence */
#define BUTTON_DEBOUNCE_MS      30u

/* Buzzer: 1 = drive with 2.7 kHz PWM (works for passive AND active piezo)
 *         0 = plain GPIO high/low (active buzzer only)                   */
#define BUZZER_USE_PWM          1
#define BUZZER_FREQ_HZ          2700u


#define RC522_NSS_PORT          GPIOA
#define RC522_NSS_PIN           4u
#define RC522_RST_PORT          GPIOB
#define RC522_RST_PIN           0u

#define BTN_MODE_PORT           GPIOB
#define BTN_MODE_PIN            8u
#define BTN_SOS_PORT            GPIOB
#define BTN_SOS_PIN             9u        /* EXTI line 9: keep on pin 9 */

#define US_TRIG_PORT            GPIOB
#define US_TRIG_PIN             12u
#define US_ECHO_PORT            GPIOB
#define US_ECHO_PIN             13u

#define BUZZER_PORT             GPIOB
#define BUZZER_PIN              4u        /* TIM3_CH1 = AF2 */

#define LED_PORT                GPIOC
#define LED_PIN                 13u

#define ADC_CH_LM35             1u        /* PA1 */
#define ADC_CH_SOUND            2u        /* PA2 */

/* I2C 7-bit addresses */
#define I2C_ADDR_LCD_A          0x27u
#define I2C_ADDR_LCD_B          0x3Fu
#define I2C_ADDR_EEPROM         0x56u     /* ZS-042 AT24C32 after A0 bridged */
#define I2C_ADDR_MAX30102       0x57u
#define I2C_ADDR_DS3231         0x68u

#endif /* BOARD_H */

/* ======================================================================== */
/* stm32f401_regs.h */
/* ======================================================================== */
/*
 * stm32f401_regs.h - minimal register definitions for STM32F401xC
 *
 * Self-contained so the project builds with any version of the Keil
 * STM32F4xx device pack (no RTE components, no CubeMX needed).
 * Offsets follow RM0368 (STM32F401 reference manual).
 */
#ifndef STM32F401_REGS_H
#define STM32F401_REGS_H
#define __IO volatile

/* ---------------- RCC ---------------- */
typedef struct {
    __IO uint32_t CR;          /* 0x00 */
    __IO uint32_t PLLCFGR;     /* 0x04 */
    __IO uint32_t CFGR;        /* 0x08 */
    __IO uint32_t CIR;         /* 0x0C */
    __IO uint32_t AHB1RSTR;    /* 0x10 */
    __IO uint32_t AHB2RSTR;    /* 0x14 */
    __IO uint32_t AHB3RSTR;    /* 0x18 */
    uint32_t      RESERVED0;   /* 0x1C */
    __IO uint32_t APB1RSTR;    /* 0x20 */
    __IO uint32_t APB2RSTR;    /* 0x24 */
    uint32_t      RESERVED1[2];/* 0x28 */
    __IO uint32_t AHB1ENR;     /* 0x30 */
    __IO uint32_t AHB2ENR;     /* 0x34 */
    __IO uint32_t AHB3ENR;     /* 0x38 */
    uint32_t      RESERVED2;   /* 0x3C */
    __IO uint32_t APB1ENR;     /* 0x40 */
    __IO uint32_t APB2ENR;     /* 0x44 */
} RCC_TypeDef;

#define RCC_CR_HSION        (1u << 0)
#define RCC_CR_HSIRDY       (1u << 1)
#define RCC_CR_HSEON        (1u << 16)
#define RCC_CR_HSERDY       (1u << 17)
#define RCC_CR_PLLON        (1u << 24)
#define RCC_CR_PLLRDY       (1u << 25)

#define RCC_AHB1ENR_GPIOAEN (1u << 0)
#define RCC_AHB1ENR_GPIOBEN (1u << 1)
#define RCC_AHB1ENR_GPIOCEN (1u << 2)

#define RCC_APB1ENR_TIM2EN  (1u << 0)
#define RCC_APB1ENR_TIM3EN  (1u << 1)
#define RCC_APB1ENR_I2C1EN  (1u << 21)
#define RCC_APB1RSTR_I2C1RST (1u << 21)

#define RCC_APB2ENR_USART1EN (1u << 4)
#define RCC_APB2ENR_ADC1EN  (1u << 8)
#define RCC_APB2ENR_SPI1EN  (1u << 12)
#define RCC_APB2ENR_SYSCFGEN (1u << 14)

/* ---------------- FLASH ---------------- */
typedef struct {
    __IO uint32_t ACR;         /* 0x00 */
    __IO uint32_t KEYR;        /* 0x04 */
    __IO uint32_t OPTKEYR;     /* 0x08 */
    __IO uint32_t SR;          /* 0x0C */
    __IO uint32_t CR;          /* 0x10 */
    __IO uint32_t OPTCR;       /* 0x14 */
} FLASH_TypeDef;

/* ---------------- GPIO ---------------- */
typedef struct {
    __IO uint32_t MODER;       /* 0x00 */
    __IO uint32_t OTYPER;      /* 0x04 */
    __IO uint32_t OSPEEDR;     /* 0x08 */
    __IO uint32_t PUPDR;       /* 0x0C */
    __IO uint32_t IDR;         /* 0x10 */
    __IO uint32_t ODR;         /* 0x14 */
    __IO uint32_t BSRR;        /* 0x18 */
    __IO uint32_t LCKR;        /* 0x1C */
    __IO uint32_t AFR[2];      /* 0x20, 0x24 */
} GPIO_TypeDef;

/* ---------------- USART ---------------- */
typedef struct {
    __IO uint32_t SR;          /* 0x00 */
    __IO uint32_t DR;          /* 0x04 */
    __IO uint32_t BRR;         /* 0x08 */
    __IO uint32_t CR1;         /* 0x0C */
    __IO uint32_t CR2;         /* 0x10 */
    __IO uint32_t CR3;         /* 0x14 */
    __IO uint32_t GTPR;        /* 0x18 */
} USART_TypeDef;

#define USART_SR_ORE        (1u << 3)
#define USART_SR_RXNE       (1u << 5)
#define USART_SR_TC         (1u << 6)
#define USART_SR_TXE        (1u << 7)
#define USART_CR1_RE        (1u << 2)
#define USART_CR1_TE        (1u << 3)
#define USART_CR1_RXNEIE    (1u << 5)
#define USART_CR1_UE        (1u << 13)

/* ---------------- I2C ---------------- */
typedef struct {
    __IO uint32_t CR1;         /* 0x00 */
    __IO uint32_t CR2;         /* 0x04 */
    __IO uint32_t OAR1;        /* 0x08 */
    __IO uint32_t OAR2;        /* 0x0C */
    __IO uint32_t DR;          /* 0x10 */
    __IO uint32_t SR1;         /* 0x14 */
    __IO uint32_t SR2;         /* 0x18 */
    __IO uint32_t CCR;         /* 0x1C */
    __IO uint32_t TRISE;       /* 0x20 */
    __IO uint32_t FLTR;        /* 0x24 */
} I2C_TypeDef;

/* ---------------- SPI ---------------- */
typedef struct {
    __IO uint32_t CR1;         /* 0x00 */
    __IO uint32_t CR2;         /* 0x04 */
    __IO uint32_t SR;          /* 0x08 */
    __IO uint32_t DR;          /* 0x0C */
    __IO uint32_t CRCPR;       /* 0x10 */
    __IO uint32_t RXCRCR;      /* 0x14 */
    __IO uint32_t TXCRCR;      /* 0x18 */
    __IO uint32_t I2SCFGR;     /* 0x1C */
    __IO uint32_t I2SPR;       /* 0x20 */
} SPI_TypeDef;

/* ---------------- ADC ---------------- */
typedef struct {
    __IO uint32_t SR;          /* 0x00 */
    __IO uint32_t CR1;         /* 0x04 */
    __IO uint32_t CR2;         /* 0x08 */
    __IO uint32_t SMPR1;       /* 0x0C */
    __IO uint32_t SMPR2;       /* 0x10 */
    __IO uint32_t JOFR1;       /* 0x14 */
    __IO uint32_t JOFR2;       /* 0x18 */
    __IO uint32_t JOFR3;       /* 0x1C */
    __IO uint32_t JOFR4;       /* 0x20 */
    __IO uint32_t HTR;         /* 0x24 */
    __IO uint32_t LTR;         /* 0x28 */
    __IO uint32_t SQR1;        /* 0x2C */
    __IO uint32_t SQR2;        /* 0x30 */
    __IO uint32_t SQR3;        /* 0x34 */
    __IO uint32_t JSQR;        /* 0x38 */
    __IO uint32_t JDR1;        /* 0x3C */
    __IO uint32_t JDR2;        /* 0x40 */
    __IO uint32_t JDR3;        /* 0x44 */
    __IO uint32_t JDR4;        /* 0x48 */
    __IO uint32_t DR;          /* 0x4C */
} ADC_TypeDef;

typedef struct {
    __IO uint32_t CSR;         /* 0x00 */
    __IO uint32_t CCR;         /* 0x04 */
    __IO uint32_t CDR;         /* 0x08 */
} ADC_Common_TypeDef;

#define ADC_SR_EOC          (1u << 1)
#define ADC_CR2_ADON        (1u << 0)
#define ADC_CR2_SWSTART     (1u << 30)

/* ---------------- TIM (general purpose) ---------------- */
typedef struct {
    __IO uint32_t CR1;         /* 0x00 */
    __IO uint32_t CR2;         /* 0x04 */
    __IO uint32_t SMCR;        /* 0x08 */
    __IO uint32_t DIER;        /* 0x0C */
    __IO uint32_t SR;          /* 0x10 */
    __IO uint32_t EGR;         /* 0x14 */
    __IO uint32_t CCMR1;       /* 0x18 */
    __IO uint32_t CCMR2;       /* 0x1C */
    __IO uint32_t CCER;        /* 0x20 */
    __IO uint32_t CNT;         /* 0x24 */
    __IO uint32_t PSC;         /* 0x28 */
    __IO uint32_t ARR;         /* 0x2C */
    __IO uint32_t RCR;         /* 0x30 */
    __IO uint32_t CCR1;        /* 0x34 */
    __IO uint32_t CCR2;        /* 0x38 */
    __IO uint32_t CCR3;        /* 0x3C */
    __IO uint32_t CCR4;        /* 0x40 */
    __IO uint32_t BDTR;        /* 0x44 */
    __IO uint32_t DCR;         /* 0x48 */
    __IO uint32_t DMAR;        /* 0x4C */
    __IO uint32_t OR;          /* 0x50 */
} TIM_TypeDef;

/* ---------------- EXTI / SYSCFG ---------------- */
typedef struct {
    __IO uint32_t IMR;         /* 0x00 */
    __IO uint32_t EMR;         /* 0x04 */
    __IO uint32_t RTSR;        /* 0x08 */
    __IO uint32_t FTSR;        /* 0x0C */
    __IO uint32_t SWIER;       /* 0x10 */
    __IO uint32_t PR;          /* 0x14 */
} EXTI_TypeDef;

typedef struct {
    __IO uint32_t MEMRMP;      /* 0x00 */
    __IO uint32_t PMC;         /* 0x04 */
    __IO uint32_t EXTICR[4];   /* 0x08..0x14 */
    uint32_t      RESERVED[2]; /* 0x18, 0x1C */
    __IO uint32_t CMPCR;       /* 0x20 */
} SYSCFG_TypeDef;

/* ---------------- Peripheral base addresses ---------------- */
#define TIM2        ((TIM_TypeDef *)        0x40000000UL)
#define TIM3        ((TIM_TypeDef *)        0x40000400UL)
#define I2C1        ((I2C_TypeDef *)        0x40005400UL)
#define USART1      ((USART_TypeDef *)      0x40011000UL)
#define ADC1        ((ADC_TypeDef *)        0x40012000UL)
#define ADC_COMMON  ((ADC_Common_TypeDef *) 0x40012300UL)
#define SPI1        ((SPI_TypeDef *)        0x40013000UL)
#define SYSCFG      ((SYSCFG_TypeDef *)     0x40013800UL)
#define EXTI        ((EXTI_TypeDef *)       0x40013C00UL)
#define GPIOA       ((GPIO_TypeDef *)       0x40020000UL)
#define GPIOB       ((GPIO_TypeDef *)       0x40020400UL)
#define GPIOC       ((GPIO_TypeDef *)       0x40020800UL)
#define RCC         ((RCC_TypeDef *)        0x40023800UL)
#define FLASH       ((FLASH_TypeDef *)      0x40023C00UL)

/* ---------------- Cortex-M4 core ---------------- */
#define REG32(a)            (*(volatile uint32_t *)(a))
#define SYST_CSR            REG32(0xE000E010UL)
#define SYST_RVR            REG32(0xE000E014UL)
#define SYST_CVR            REG32(0xE000E018UL)
#define NVIC_ISER(n)        REG32(0xE000E100UL + 4u * (n))
#define NVIC_IPR_BYTE(n)    (*(volatile uint8_t *)(0xE000E400UL + (n)))
#define SCB_SHPR3           REG32(0xE000ED20UL)
#define SCB_CPACR           REG32(0xE000ED88UL)

/* IRQ numbers used */
#define IRQN_EXTI9_5        23u
#define IRQN_USART1         37u

static inline void nvic_enable_irq(uint32_t irqn)
{
    NVIC_ISER(irqn >> 5) = 1u << (irqn & 31u);
}

/* STM32F4 implements 4 priority bits (upper nibble). prio 0..15, 0 = highest */
static inline void nvic_set_priority(uint32_t irqn, uint32_t prio)
{
    NVIC_IPR_BYTE(irqn) = (uint8_t)((prio & 0x0Fu) << 4);
}

static inline uint32_t irq_save(void)
{
    uint32_t primask;
    __asm volatile ("mrs %0, primask" : "=r" (primask));
    __asm volatile ("cpsid i" ::: "memory");
    return primask;
}

static inline void irq_restore(uint32_t primask)
{
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}

#endif /* STM32F401_REGS_H */

/* ======================================================================== */
/* gpio.h */
/* ======================================================================== */
/*
 * gpio.h - tiny inline GPIO helpers (register level)
 */
#ifndef GPIO_H
#define GPIO_H
#define GPIO_MODE_INPUT     0u
#define GPIO_MODE_OUTPUT    1u
#define GPIO_MODE_AF        2u
#define GPIO_MODE_ANALOG    3u

#define GPIO_PULL_NONE      0u
#define GPIO_PULL_UP        1u
#define GPIO_PULL_DOWN      2u

#define GPIO_SPEED_LOW      0u
#define GPIO_SPEED_MED      1u
#define GPIO_SPEED_FAST     2u
#define GPIO_SPEED_HIGH     3u

static inline void gpio_mode(GPIO_TypeDef *p, uint32_t pin, uint32_t mode)
{
    p->MODER = (p->MODER & ~(3u << (pin * 2u))) | (mode << (pin * 2u));
}

static inline void gpio_pull(GPIO_TypeDef *p, uint32_t pin, uint32_t pull)
{
    p->PUPDR = (p->PUPDR & ~(3u << (pin * 2u))) | (pull << (pin * 2u));
}

static inline void gpio_speed(GPIO_TypeDef *p, uint32_t pin, uint32_t speed)
{
    p->OSPEEDR = (p->OSPEEDR & ~(3u << (pin * 2u))) | (speed << (pin * 2u));
}

static inline void gpio_open_drain(GPIO_TypeDef *p, uint32_t pin, int od)
{
    if (od) p->OTYPER |= (1u << pin);
    else    p->OTYPER &= ~(1u << pin);
}

static inline void gpio_af(GPIO_TypeDef *p, uint32_t pin, uint32_t af)
{
    uint32_t idx = pin >> 3;
    uint32_t sh  = (pin & 7u) * 4u;
    p->AFR[idx] = (p->AFR[idx] & ~(0xFu << sh)) | (af << sh);
}

static inline void gpio_set(GPIO_TypeDef *p, uint32_t pin)   { p->BSRR = (1u << pin); }
static inline void gpio_clr(GPIO_TypeDef *p, uint32_t pin)   { p->BSRR = (1u << (pin + 16u)); }
static inline int  gpio_read(GPIO_TypeDef *p, uint32_t pin)  { return (int)((p->IDR >> pin) & 1u); }
static inline void gpio_toggle(GPIO_TypeDef *p, uint32_t pin){ p->ODR ^= (1u << pin); }

#endif /* GPIO_H */

/* ======================================================================== */
/* system.h */
/* ======================================================================== */
/*
 * system.h - clock tree (84 MHz), 1 ms SysTick, 1 us TIM2 timebase
 */
#ifndef SYSTEM_H
#define SYSTEM_H
extern volatile uint32_t g_ms_ticks;
extern uint32_t g_sysclk_hz;

void     system_init(void);     /* PLL 84 MHz, SysTick 1 kHz, TIM2 1 MHz, LED */
int      system_hse_ok(void);   /* 1 = running from 25 MHz crystal */
uint32_t millis(void);
uint32_t micros(void);
void     delay_ms(uint32_t ms);
void     delay_us(uint32_t us);
void     led_toggle(void);

#endif /* SYSTEM_H */

/* ======================================================================== */
/* buzzer.h */
/* ======================================================================== */
/*
 * buzzer.h - non-blocking beep patterns, driven from the 1 ms SysTick
 */
#ifndef BUZZER_H
#define BUZZER_H
void buzzer_init(void);
void buzzer_beep(uint32_t count, uint32_t on_ms, uint32_t off_ms);
int  buzzer_busy(void);
void buzzer_wait(void);        /* block until the pattern finishes */
void buzzer_tick_1ms(void);    /* called by SysTick_Handler */

#endif /* BUZZER_H */

/* ======================================================================== */
/* uart.h */
/* ======================================================================== */
/*
 * uart.h - USART1 (PA9 TX, PA10 RX) to the ESP8266, interrupt-driven RX ring
 */
#ifndef UART_H
#define UART_H
void uart_init(uint32_t baud);
void uart_write(const char *buf, uint32_t len);
void uart_puts(const char *s);
int  uart_getc(char *c);          /* 1 = got a char, 0 = buffer empty */
void uart_flush_rx(void);
uint32_t uart_overflows(void);

#endif /* UART_H */

/* ======================================================================== */
/* i2c.h */
/* ======================================================================== */
/*
 * i2c.h - I2C1 master, 100 kHz, PB6 = SCL, PB7 = SDA (polling with timeouts)
 */
#ifndef I2C_H
#define I2C_H
#define I2C_OK           0
#define I2C_ERR_NACK    (-1)
#define I2C_ERR_TIMEOUT (-2)

void i2c_init(void);
int  i2c_probe(uint8_t addr7);
int  i2c_write(uint8_t addr7, const uint8_t *data, uint32_t len);
int  i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t value);
int  i2c_read(uint8_t addr7, uint8_t *buf, uint32_t len);
int  i2c_read_regs(uint8_t addr7, uint8_t reg, uint8_t *buf, uint32_t len);

#endif /* I2C_H */

/* ======================================================================== */
/* spi.h */
/* ======================================================================== */
/*
 * spi.h - SPI1 master, mode 0, 1.3 MHz (PA5 SCK, PA6 MISO, PA7 MOSI)
 */
#ifndef SPI_H
#define SPI_H
void    spi_init(void);
uint8_t spi_xfer(uint8_t b);

#endif /* SPI_H */

/* ======================================================================== */
/* lcd.h */
/* ======================================================================== */
/*
 * lcd.h - 16x2 HD44780 LCD through a PCF8574 I2C backpack (0x27 or 0x3F)
 */
#ifndef LCD_H
#define LCD_H
int     lcd_init(void);                    /* 0 = found, -1 = not on bus */
int     lcd_present(void);
uint8_t lcd_address(void);
void    lcd_clear(void);
void    lcd_line(uint8_t row, const char *text);         /* pads to 16 chars */
void    lcd_show(const char *line1, const char *line2);  /* only redraws changes */

#endif /* LCD_H */

/* ======================================================================== */
/* rc522.h */
/* ======================================================================== */
/*
 * rc522.h - MFRC522 RFID reader over SPI1
 */
#ifndef RC522_H
#define RC522_H
/* Diagnostics for the Keil Watch window */
extern volatile uint8_t  g_rc522_version;   /* VersionReg: 0x91/0x92 genuine, 0x88/0xB2/0x12 clones */
extern volatile uint8_t  g_rc522_readback;  /* write 0x26 to ModWidthReg, read back: must be 0x26 */
extern volatile uint8_t  g_rc522_txctrl;    /* TxControlReg: low 2 bits = 3 means antenna on */
extern volatile uint8_t  g_rc522_irq;       /* ComIrqReg after the last card poll */
extern volatile uint8_t  g_rc522_err;       /* ErrorReg after the last card poll */
extern volatile int32_t  g_rc522_status;    /* last poll: 0 ok, -1 error, -2 no card, -3 collision */
extern volatile uint32_t g_rc522_polls;     /* counts every card poll */
extern volatile uint32_t g_rc522_atqa;      /* counts polls where a card answered REQA */

int     rc522_init(void);             /* 0 = chip answered, -1 = not found */
uint8_t rc522_version(void);          /* 0x91/0x92 genuine, 0x88/0xB2 clones */
int     rc522_read_uid(uint8_t uid[4]);  /* 1 = new card read, 0 = none */
void    rc522_uid_to_hex(const uint8_t uid[4], char out[9]);

/* ---- low-level debug helpers (used by TEST_COMPONENT 12) ---- */
uint8_t rc522_reg_read(uint8_t reg);
void    rc522_reg_write(uint8_t reg, uint8_t value);
/* write 16 patterns to two read/write registers; returns error count,
 * *bad_bits = OR of all bits that read back wrong */
int     rc522_spi_pattern_test(uint8_t *bad_bits);
/* chip's built-in digital self test (datasheet 16.1.1): 64 bytes.
 * returns 2 = matches v2.0, 1 = matches v1.0, 0 = unknown (clone), -1 = no result */
int     rc522_self_test(uint8_t out[64]);

typedef struct {
    int8_t  result;     /* 0 answered, 1 nothing (timer), 2 garbled reply, -4 sw timeout */
    uint8_t irq;        /* ComIrqReg */
    uint8_t err;        /* ErrorReg */
    uint8_t fifo;       /* bytes received */
    uint8_t lastbits;   /* ControlReg RxLastBits */
    uint8_t atqa[2];
} rc522_probe_t;
/* send REQA (wupa = 0) or WUPA (wupa = 1) once and report everything seen */
void    rc522_probe(int wupa, rc522_probe_t *p);

#endif /* RC522_H */

/* ======================================================================== */
/* ds3231.h */
/* ======================================================================== */
/*
 * ds3231.h - DS3231 real-time clock (ZS-042 module) at I2C 0x68
 */
#ifndef DS3231_H
#define DS3231_H
typedef struct {
    uint16_t year;     /* 2000..2099 */
    uint8_t  month;    /* 1..12 */
    uint8_t  day;      /* 1..31 */
    uint8_t  hour;     /* 0..23 */
    uint8_t  minute;
    uint8_t  second;
} rtc_time_t;

int  ds3231_present(void);
int  ds3231_read(rtc_time_t *t);                 /* 0 = ok */
int  ds3231_set(const rtc_time_t *t);
int  ds3231_lost_power(void);                    /* 1 = OSF set (time invalid) */
int  ds3231_set_from_build_time(void);           /* uses __DATE__ / __TIME__ */
int  ds3231_init(void);                          /* start osc, apply RTC_* options */
void ds3231_format(const rtc_time_t *t, char out[20]);   /* YYYY-MM-DD HH:MM:SS */
void ds3231_timestamp(char out[20]);             /* read + format (zeros on error) */

#endif /* DS3231_H */

/* ======================================================================== */
/* max30102.h */
/* ======================================================================== */
/*
 * max30102.h - MAX30102 pulse sensor at I2C 0x57
 * Configured for SpO2 mode, 200 sps with 4-sample averaging = 50 samples/s
 */
#ifndef MAX30102_H
#define MAX30102_H
#define MAX30102_SAMPLE_MS  20u      /* one FIFO sample every 20 ms */

int     max30102_init(void);         /* 0 = found (part ID 0x15) */
uint8_t max30102_part_id(void);
/* drain up to max samples; *overflow = 1 if samples were lost */
int     max30102_read_fifo(uint32_t *red, uint32_t *ir, int max, int *overflow);

#endif /* MAX30102_H */

/* ======================================================================== */
/* hr_algo.h */
/* ======================================================================== */
/*
 * hr_algo.h - heart-rate (BPM) estimation from the MAX30102 IR signal
 * Pure C, no hardware access: one call per 20 ms sample.
 */
#ifndef HR_ALGO_H
#define HR_ALGO_H
void hr_reset(void);
void hr_mark_gap(void);              /* samples were lost: restart beat timing */
int  hr_add_sample(uint32_t ir);     /* returns 1 when a beat is detected */
int  hr_finger_present(void);
int  hr_bpm(void);                   /* 0 = not valid yet */

#endif /* HR_ALGO_H */

/* ======================================================================== */
/* sensors.h */
/* ======================================================================== */
/*
 * sensors.h - ADC1 (LM35 on PA1, KY-037 on PA2) and HC-SR04 ultrasonic
 */
#ifndef SENSORS_H
#define SENSORS_H
#define US_NO_SENSOR    (-1)     /* echo never went high: check wiring */
#define US_OUT_OF_RANGE 999      /* nothing within ~4 m */

void     sensors_init(void);
uint16_t adc_read(uint32_t channel);
int      sound_p2p(uint32_t window_ms, uint16_t *last_raw);   /* peak-to-peak counts */
int      lm35_read_c10(void);                                  /* tenths of a degree C */
int      ultrasonic_cm(void);                                  /* cm, or codes above */

#endif /* SENSORS_H */

/* ======================================================================== */
/* buttons.h */
/* ======================================================================== */
/*
 * buttons.h - Mode / SOS buttons
 *
 * SINGLE_BUTTON = 1 : one button on PB9. Presses are counted in the EXTI9
 *                     interrupt; when the button has been idle for
 *                     CLICK_GAP_MS the count is decided in SysTick:
 *                       1 press  -> mode event
 *                       SOS_PRESS_COUNT or more -> SOS
 * SINGLE_BUTTON = 0 : Mode on PB8 (polled), SOS on PB9 (EXTI9, 1 press).
 */
#ifndef BUTTONS_H
#define BUTTONS_H
extern volatile uint8_t  g_sos_flag;        /* set when SOS is requested */
extern volatile uint32_t g_sos_irq_count;   /* raw debounced presses on PB9 */
extern volatile uint32_t g_btn_last_clicks; /* size of the last press sequence */
extern volatile uint8_t  g_btn_idle_level;  /* PB9 level when not pressed (auto-detected) */

void buttons_init(void);
void buttons_tick_1ms(void);   /* called from SysTick_Handler */
int  button_mode_event(void);  /* 1 once per mode request */
int  button_mode_down(void);   /* raw: 1 = pressed (PB8, or PB9 in single mode) */
int  button_sos_down(void);    /* raw: 1 = pressed (PB9) */

#endif /* BUTTONS_H */

/* ======================================================================== */
/* esp8266.h */
/* ======================================================================== */
/*
 * esp8266.h - ESP8266 (NodeMCU, AT firmware) HTTP client over USART1
 */
#ifndef ESP8266_H
#define ESP8266_H
#define ESP_OK            0
#define ESP_ERR_NO_REPLY (-1)
#define ESP_ERR_CONNECT  (-2)
#define ESP_ERR_SEND     (-3)
#define ESP_ERR_RESPONSE (-4)

int esp_init(void);                                   /* AT handshake, station mode */
int esp_join(const char *ssid, const char *pass, char *ip, int ipmax);
int esp_get_ip(char *ip, int ipmax);
int esp_link_up(void);                                /* 1 = joined to the AP */
int esp_version(char *out, int outmax);               /* first line of AT+GMR */
int esp_cmd(const char *cmd, uint32_t timeout_ms);    /* 0 = OK */

/* POST JSON to SERVER_IP:SERVER_PORT. Returns ESP_OK and fills the HTTP
 * status + body, or a negative ESP_ERR_* code. */
int esp_http_post(const char *path, const char *json,
                  int *status, char *body, int bodymax);

/* helpers for small flat JSON bodies */
int json_get_int(const char *json, const char *key, int32_t *out);
int json_get_str(const char *json, const char *key, char *out, int outmax);

#endif /* ESP8266_H */

/* ======================================================================== */
/* app.h */
/* ======================================================================== */
/*
 * app.h - Smart Student ID main application + Keil Watch variables
 */
#ifndef APP_H
#define APP_H
/* Add these to the Keil Watch window while debugging */
extern volatile char    g_last_uid[9];     /* last card, 8 hex chars */
extern volatile int32_t g_last_bpm;        /* 0 = no valid reading */
extern volatile int32_t g_sound_level;     /* KY-037 peak-to-peak (ADC counts) */
extern volatile int32_t g_distance_cm;     /* HC-SR04 (-1 = no sensor, 999 = out of range) */
extern volatile float   g_temperature_c;   /* LM35 */

void app_run(void);                        /* never returns */

#endif /* APP_H */

/* ======================================================================== */
/* component_tests.h */
/* ======================================================================== */
/*
 * component_tests.h - one-component-at-a-time bring-up tests
 * Select with TEST_COMPONENT in board.h (1..10). Watch these in Keil:
 */
#ifndef COMPONENT_TESTS_H
#define COMPONENT_TESTS_H
extern volatile uint32_t g_test_alive;     /* counts up while the test runs */
extern volatile int32_t  g_test_ok;        /* 1 = component responded correctly */
extern volatile int32_t  g_test_value;     /* main measured value */
extern volatile char     g_test_text[32];  /* short text result */

void component_test_run(int test);         /* never returns */

#endif /* COMPONENT_TESTS_H */

/* ======================================================================== */
/* system.c */
/* ======================================================================== */
/*
 * system.c - clocks, SysTick (1 ms), TIM2 (1 us free-running), SystemInit
 *
 * Clock tree:
 *   HSE 25 MHz -> /M 25 -> x N 336 -> /P 4 = 84 MHz SYSCLK, /Q 7 = 48 MHz
 *   (falls back to HSI 16 MHz with M = 16 if the crystal does not start)
 *   AHB = 84 MHz, APB1 = 42 MHz (timers 84 MHz), APB2 = 84 MHz
 */
volatile uint32_t g_ms_ticks = 0;
uint32_t g_sysclk_hz = 16000000u;
static int s_hse_ok = 0;

/* Called from Reset_Handler before C runtime init: no globals here. */
#if PROVIDE_SYSTEMINIT
void SystemInit(void)
{
    SCB_CPACR |= (0xFu << 20);          /* CP10 + CP11 full access: enable FPU */
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");
}
#endif


static void clock_init(void)
{
    uint32_t i;
    uint32_t pllcfgr;

    /* make sure we run from HSI while (re)configuring */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }
    RCC->CFGR &= ~3u;
    while ((RCC->CFGR & 0x0Cu) != 0u) { }
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) { }

    /* try the 25 MHz crystal */
    RCC->CR |= RCC_CR_HSEON;
    for (i = 0; i < 2000000u; i++) {
        if (RCC->CR & RCC_CR_HSERDY) { s_hse_ok = 1; break; }
    }

    if (s_hse_ok) {
        pllcfgr = 25u | (336u << 6) | (1u << 16) | (1u << 22) | (7u << 24);
    } else {
        RCC->CR &= ~RCC_CR_HSEON;
        pllcfgr = 16u | (336u << 6) | (1u << 16) | (0u << 22) | (7u << 24);
    }
    RCC->PLLCFGR = pllcfgr;

    /* 2 wait states for 84 MHz @ 3.3 V, prefetch + caches on */
    FLASH->ACR = 2u | (1u << 8) | (1u << 9) | (1u << 10);
    while ((FLASH->ACR & 0xFu) != 2u) { }

    /* AHB /1, APB1 /2, APB2 /1 */
    RCC->CFGR = (RCC->CFGR & ~((0xFu << 4) | (7u << 10) | (7u << 13))) | (4u << 10);

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    RCC->CFGR = (RCC->CFGR & ~3u) | 2u;            /* SYSCLK = PLL */
    while ((RCC->CFGR & 0x0Cu) != 0x08u) { }

    g_sysclk_hz = 84000000u;
}

void system_init(void)
{
    clock_init();

    /* SysTick 1 kHz, priority 8 */
    SYST_RVR = (g_sysclk_hz / 1000u) - 1u;
    SYST_CVR = 0u;
    SCB_SHPR3 = (SCB_SHPR3 & 0x00FFFFFFu) | (0x80u << 24);
    SYST_CSR = 7u;                                 /* CPU clock, IRQ, enable */

    /* TIM2: 32-bit, 1 MHz free-running (APB1 timer clock = 84 MHz) */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->CR1 = 0u;
    TIM2->PSC = 84u - 1u;
    TIM2->ARR = 0xFFFFFFFFu;
    TIM2->EGR = 1u;
    TIM2->CR1 = 1u;

    /* GPIO clocks + on-board LED (PC13, active low) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;
    gpio_set(LED_PORT, LED_PIN);
    gpio_mode(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT);
}

int system_hse_ok(void) { return s_hse_ok; }

uint32_t millis(void) { return g_ms_ticks; }

uint32_t micros(void) { return TIM2->CNT; }

void delay_ms(uint32_t ms)
{
    uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms) { }
}

void delay_us(uint32_t us)
{
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us) { }
}

void led_toggle(void) { gpio_toggle(LED_PORT, LED_PIN); }

void SysTick_Handler(void)
{
    g_ms_ticks++;
    buzzer_tick_1ms();
    buttons_tick_1ms();
}

/* Trap faults where the debugger can see them */
void HardFault_Handler(void)
{
    for (;;) { }
}

/* ======================================================================== */
/* buzzer.c */
/* ======================================================================== */
/*
 * buzzer.c - piezo buzzer on PB4
 *   BUZZER_USE_PWM = 1 : TIM3_CH1 PWM at BUZZER_FREQ_HZ (passive or active piezo)
 *   BUZZER_USE_PWM = 0 : plain push-pull GPIO (active buzzer)
 * The beep pattern runs inside SysTick, so it keeps beeping while the main
 * loop is blocked in a Wi-Fi request.
 */
static volatile uint32_t s_remaining = 0;   /* beeps left (including current) */
static volatile uint32_t s_on_ms = 0, s_off_ms = 0;
static volatile uint32_t s_phase_left = 0;
static volatile uint8_t  s_is_on = 0;

static void buzzer_hw(int on)
{
#if BUZZER_USE_PWM
    TIM3->CCR1 = on ? (TIM3->ARR + 1u) / 2u : 0u;
#else
    if (on) gpio_set(BUZZER_PORT, BUZZER_PIN);
    else    gpio_clr(BUZZER_PORT, BUZZER_PIN);
#endif
}

void buzzer_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
#if BUZZER_USE_PWM
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;
    gpio_af(BUZZER_PORT, BUZZER_PIN, 2u);                 /* TIM3_CH1 */
    gpio_mode(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_AF);
    TIM3->CR1   = 0u;
    TIM3->PSC   = 84u - 1u;                               /* 1 MHz */
    TIM3->ARR   = (1000000u / BUZZER_FREQ_HZ) - 1u;
    TIM3->CCR1  = 0u;
    TIM3->CCMR1 = (6u << 4) | (1u << 3);                  /* PWM mode 1, preload */
    TIM3->CCER  = 1u;                                     /* CC1E */
    TIM3->EGR   = 1u;
    TIM3->CR1   = (1u << 7) | 1u;                         /* ARPE, CEN */
#else
    gpio_clr(BUZZER_PORT, BUZZER_PIN);
    gpio_mode(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUTPUT);
#endif
    buzzer_hw(0);
}

void buzzer_beep(uint32_t count, uint32_t on_ms, uint32_t off_ms)
{
    uint32_t pm = irq_save();
    if (count == 0u) {
        s_remaining = 0u;
        s_is_on = 0u;
        buzzer_hw(0);
    } else {
        s_on_ms = on_ms;
        s_off_ms = off_ms;
        s_remaining = count;
        s_phase_left = on_ms;
        s_is_on = 1u;
        buzzer_hw(1);
    }
    irq_restore(pm);
}

int buzzer_busy(void) { return s_remaining != 0u; }

void buzzer_wait(void)
{
    while (buzzer_busy()) { }
}

void buzzer_tick_1ms(void)
{
    if (s_remaining == 0u) return;
    if (s_phase_left > 0u) s_phase_left--;
    if (s_phase_left != 0u) return;

    if (s_is_on) {
        buzzer_hw(0);
        s_is_on = 0u;
        s_remaining--;
        s_phase_left = (s_remaining != 0u) ? s_off_ms : 0u;
    } else {
        buzzer_hw(1);
        s_is_on = 1u;
        s_phase_left = s_on_ms;
    }
}

/* ======================================================================== */
/* uart.c */
/* ======================================================================== */
/*
 * uart.c - USART1 driver for the ESP8266 link
 *   PA9  = USART1_TX (AF7) -> NodeMCU RX
 *   PA10 = USART1_RX (AF7) <- NodeMCU TX
 */
#define RX_BUF_SIZE 1024u                  /* power of two */

static volatile uint8_t  s_rx[RX_BUF_SIZE];
static volatile uint32_t s_head = 0, s_tail = 0;
static volatile uint32_t s_ovf = 0;

void uart_init(uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;

    gpio_af(GPIOA, 9u, 7u);
    gpio_af(GPIOA, 10u, 7u);
    gpio_pull(GPIOA, 10u, GPIO_PULL_UP);
    gpio_speed(GPIOA, 9u, GPIO_SPEED_FAST);
    gpio_mode(GPIOA, 9u, GPIO_MODE_AF);
    gpio_mode(GPIOA, 10u, GPIO_MODE_AF);

    USART1->CR1 = 0u;
    USART1->CR2 = 0u;
    USART1->CR3 = 0u;
    USART1->BRR = (84000000u + baud / 2u) / baud;    /* APB2 = 84 MHz, OVER8 = 0 */
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    s_head = s_tail = 0u;
    nvic_set_priority(IRQN_USART1, 4u);
    nvic_enable_irq(IRQN_USART1);
}

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        uint8_t c = (uint8_t)USART1->DR;              /* also clears ORE */
        uint32_t next = (s_head + 1u) & (RX_BUF_SIZE - 1u);
        if (next != s_tail) {
            s_rx[s_head] = c;
            s_head = next;
        } else {
            s_ovf++;
        }
    }
}

void uart_write(const char *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        while (!(USART1->SR & USART_SR_TXE)) { }
        USART1->DR = (uint8_t)buf[i];
    }
    while (!(USART1->SR & USART_SR_TC)) { }
}

void uart_puts(const char *s) { uart_write(s, (uint32_t)strlen(s)); }

int uart_getc(char *c)
{
    if (s_tail == s_head) return 0;
    *c = (char)s_rx[s_tail];
    s_tail = (s_tail + 1u) & (RX_BUF_SIZE - 1u);
    return 1;
}

void uart_flush_rx(void)
{
    uint32_t pm = irq_save();
    s_tail = s_head;
    irq_restore(pm);
}

uint32_t uart_overflows(void) { return s_ovf; }

/* ======================================================================== */
/* i2c.c */
/* ======================================================================== */
/*
 * i2c.c - I2C1 master driver (STM32F4 "v1" I2C peripheral), 100 kHz
 *
 * Follows the RM0368 receive sequences for N = 1, N = 2 and N > 2 bytes.
 * Every wait has a timeout; a stuck bus is recovered with 9 SCL clocks.
 */
#define I2C_TIMEOUT_MS  10u

#define CR1_PE      (1u << 0)
#define CR1_START   (1u << 8)
#define CR1_STOP    (1u << 9)
#define CR1_ACK     (1u << 10)
#define CR1_POS     (1u << 11)
#define CR1_SWRST   (1u << 15)

#define SR1_SB      (1u << 0)
#define SR1_ADDR    (1u << 1)
#define SR1_BTF     (1u << 2)
#define SR1_RXNE    (1u << 6)
#define SR1_TXE     (1u << 7)
#define SR1_BERR    (1u << 8)
#define SR1_ARLO    (1u << 9)
#define SR1_AF      (1u << 10)

#define SR2_BUSY    (1u << 1)

#define SCL_PIN     6u
#define SDA_PIN     7u

static void bus_recover(void)
{
    uint32_t i;

    /* drive the pins as open-drain GPIO */
    gpio_open_drain(GPIOB, SCL_PIN, 1);
    gpio_open_drain(GPIOB, SDA_PIN, 1);
    gpio_pull(GPIOB, SCL_PIN, GPIO_PULL_UP);
    gpio_pull(GPIOB, SDA_PIN, GPIO_PULL_UP);
    gpio_set(GPIOB, SCL_PIN);
    gpio_set(GPIOB, SDA_PIN);
    gpio_mode(GPIOB, SCL_PIN, GPIO_MODE_OUTPUT);
    gpio_mode(GPIOB, SDA_PIN, GPIO_MODE_OUTPUT);
    delay_us(10);

    /* a slave holding SDA low is released by clocking it out */
    for (i = 0; i < 9u && !gpio_read(GPIOB, SDA_PIN); i++) {
        gpio_clr(GPIOB, SCL_PIN); delay_us(5);
        gpio_set(GPIOB, SCL_PIN); delay_us(5);
    }

    /* STOP condition */
    gpio_clr(GPIOB, SDA_PIN); delay_us(5);
    gpio_set(GPIOB, SCL_PIN); delay_us(5);
    gpio_set(GPIOB, SDA_PIN); delay_us(5);
}

static void periph_config(void)
{
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    gpio_af(GPIOB, SCL_PIN, 4u);
    gpio_af(GPIOB, SDA_PIN, 4u);
    gpio_speed(GPIOB, SCL_PIN, GPIO_SPEED_FAST);
    gpio_speed(GPIOB, SDA_PIN, GPIO_SPEED_FAST);
    gpio_mode(GPIOB, SCL_PIN, GPIO_MODE_AF);
    gpio_mode(GPIOB, SDA_PIN, GPIO_MODE_AF);

    I2C1->CR1 = CR1_SWRST;
    I2C1->CR1 = 0u;
    I2C1->CR2 = 42u;              /* APB1 = 42 MHz */
    I2C1->CCR = 210u;             /* 42 MHz / (2 * 100 kHz) */
    I2C1->TRISE = 43u;            /* 1000 ns * 42 MHz + 1 */
    I2C1->CR1 = CR1_PE;
}

void i2c_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;
    bus_recover();
    periph_config();
}

static void reinit(void)
{
    I2C1->CR1 = 0u;
    bus_recover();
    periph_config();
}

static int fail(int code)
{
    I2C1->CR1 |= CR1_STOP;
    I2C1->SR1 &= ~(SR1_AF | SR1_ARLO | SR1_BERR);
    I2C1->CR1 &= ~CR1_POS;
    if (code == I2C_ERR_TIMEOUT) reinit();
    return code;
}

/* wait for SR1 flag; returns NACK if the slave did not acknowledge */
static int wait_sr1(uint32_t flag)
{
    uint32_t t0 = millis();
    while (!(I2C1->SR1 & flag)) {
        if (I2C1->SR1 & SR1_AF) return I2C_ERR_NACK;
        if ((millis() - t0) > I2C_TIMEOUT_MS) return I2C_ERR_TIMEOUT;
    }
    return I2C_OK;
}

static int wait_idle(void)
{
    uint32_t t0 = millis();
    while ((I2C1->SR2 & SR2_BUSY) || (I2C1->CR1 & CR1_STOP)) {
        if ((millis() - t0) > I2C_TIMEOUT_MS) {
            reinit();
            return ((I2C1->SR2 & SR2_BUSY) ? I2C_ERR_TIMEOUT : I2C_OK);
        }
    }
    return I2C_OK;
}

static inline void clear_addr(void)
{
    volatile uint32_t tmp;
    tmp = I2C1->SR1;
    tmp = I2C1->SR2;
    (void)tmp;
}

/* generate (repeated) START and send the address; ADDR is left set */
static int start(uint8_t addr7, uint32_t rd)
{
    int r;
    I2C1->CR1 |= CR1_START;
    r = wait_sr1(SR1_SB);
    if (r) return I2C_ERR_TIMEOUT;
    I2C1->DR = (uint32_t)((addr7 << 1) | (rd ? 1u : 0u));
    return wait_sr1(SR1_ADDR);
}

static int rx_after_addr(uint8_t *buf, uint32_t len)
{
    uint32_t pm;

    if (len == 1u) {
        I2C1->CR1 &= ~CR1_ACK;
        pm = irq_save();
        clear_addr();
        I2C1->CR1 |= CR1_STOP;
        irq_restore(pm);
        if (wait_sr1(SR1_RXNE)) return fail(I2C_ERR_TIMEOUT);
        buf[0] = (uint8_t)I2C1->DR;
    } else if (len == 2u) {
        I2C1->CR1 &= ~CR1_ACK;
        I2C1->CR1 |= CR1_POS;
        clear_addr();
        if (wait_sr1(SR1_BTF)) return fail(I2C_ERR_TIMEOUT);
        pm = irq_save();
        I2C1->CR1 |= CR1_STOP;
        buf[0] = (uint8_t)I2C1->DR;
        irq_restore(pm);
        buf[1] = (uint8_t)I2C1->DR;
        I2C1->CR1 &= ~CR1_POS;
    } else {
        I2C1->CR1 |= CR1_ACK;
        clear_addr();
        while (len > 3u) {
            if (wait_sr1(SR1_RXNE)) return fail(I2C_ERR_TIMEOUT);
            *buf++ = (uint8_t)I2C1->DR;
            len--;
        }
        /* last three bytes */
        if (wait_sr1(SR1_BTF)) return fail(I2C_ERR_TIMEOUT);
        I2C1->CR1 &= ~CR1_ACK;
        *buf++ = (uint8_t)I2C1->DR;                 /* N-2 */
        if (wait_sr1(SR1_BTF)) return fail(I2C_ERR_TIMEOUT);
        pm = irq_save();
        I2C1->CR1 |= CR1_STOP;
        *buf++ = (uint8_t)I2C1->DR;                 /* N-1 */
        irq_restore(pm);
        *buf = (uint8_t)I2C1->DR;                   /* N   */
    }
    return I2C_OK;
}

int i2c_write(uint8_t addr7, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    int r;

    if (wait_idle()) return I2C_ERR_TIMEOUT;
    r = start(addr7, 0u);
    if (r) return fail(r);
    clear_addr();
    for (i = 0; i < len; i++) {
        r = wait_sr1(SR1_TXE);
        if (r) return fail(r);
        I2C1->DR = data[i];
    }
    if (len) {
        r = wait_sr1(SR1_BTF);
        if (r) return fail(r);
    }
    I2C1->CR1 |= CR1_STOP;
    return I2C_OK;
}

int i2c_probe(uint8_t addr7)
{
    return (i2c_write(addr7, 0, 0u) == I2C_OK);
}

int i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t value)
{
    uint8_t b[2];
    b[0] = reg;
    b[1] = value;
    return i2c_write(addr7, b, 2u);
}

int i2c_read(uint8_t addr7, uint8_t *buf, uint32_t len)
{
    int r;
    if (len == 0u) return I2C_OK;
    if (wait_idle()) return I2C_ERR_TIMEOUT;
    r = start(addr7, 1u);
    if (r) return fail(r);
    return rx_after_addr(buf, len);
}

int i2c_read_regs(uint8_t addr7, uint8_t reg, uint8_t *buf, uint32_t len)
{
    int r;
    if (len == 0u) return I2C_OK;
    if (wait_idle()) return I2C_ERR_TIMEOUT;

    r = start(addr7, 0u);
    if (r) return fail(r);
    clear_addr();
    r = wait_sr1(SR1_TXE);
    if (r) return fail(r);
    I2C1->DR = reg;
    r = wait_sr1(SR1_BTF);
    if (r) return fail(r);

    r = start(addr7, 1u);                         /* repeated START */
    if (r) return fail(r);
    return rx_after_addr(buf, len);
}

/* ======================================================================== */
/* spi.c */
/* ======================================================================== */
/*
 * spi.c - SPI1 master for the RC522 (NSS is a plain GPIO handled by rc522.c)
 */
#define SPI_SR_RXNE (1u << 0)
#define SPI_SR_TXE  (1u << 1)

void spi_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    gpio_af(GPIOA, 5u, 5u);
    gpio_af(GPIOA, 6u, 5u);
    gpio_af(GPIOA, 7u, 5u);
    gpio_speed(GPIOA, 5u, GPIO_SPEED_FAST);
    gpio_speed(GPIOA, 7u, GPIO_SPEED_FAST);
    gpio_mode(GPIOA, 5u, GPIO_MODE_AF);
    gpio_mode(GPIOA, 6u, GPIO_MODE_AF);
    gpio_mode(GPIOA, 7u, GPIO_MODE_AF);

    /* master, CPOL 0, CPHA 0, fPCLK/64 = 1.3 MHz (safe on breadboard wires), software NSS */
    SPI1->CR1 = 0u;
    SPI1->CR2 = 0u;
    SPI1->CR1 = (1u << 2) | (5u << 3) | (1u << 8) | (1u << 9);
    SPI1->CR1 |= (1u << 6);                                   /* SPE */
}

uint8_t spi_xfer(uint8_t b)
{
    uint32_t guard = 100000u;
    while (!(SPI1->SR & SPI_SR_TXE) && --guard) { }
    *(volatile uint8_t *)&SPI1->DR = b;
    guard = 100000u;
    while (!(SPI1->SR & SPI_SR_RXNE) && --guard) { }
    return *(volatile uint8_t *)&SPI1->DR;
}

/* ======================================================================== */
/* lcd.c */
/* ======================================================================== */
/*
 * lcd.c - HD44780 16x2 in 4-bit mode via PCF8574
 *   PCF8574 bits: P0 = RS, P1 = RW, P2 = EN, P3 = backlight, P4..P7 = D4..D7
 * Each nibble is sent as one I2C write of {data|EN, data} so the EN pulse
 * lasts a full I2C byte time (~90 us).
 */
#define LCD_RS  0x01u
#define LCD_EN  0x04u
#define LCD_BL  0x08u

static uint8_t s_addr = 0;
static int     s_ok = 0;
static char    s_cache[2][17];

static void send_byte(uint8_t value, uint8_t rs)
{
    uint8_t hi = (uint8_t)((value & 0xF0u) | rs | LCD_BL);
    uint8_t lo = (uint8_t)(((value << 4) & 0xF0u) | rs | LCD_BL);
    uint8_t b[4];
    b[0] = (uint8_t)(hi | LCD_EN); b[1] = hi;
    b[2] = (uint8_t)(lo | LCD_EN); b[3] = lo;
    (void)i2c_write(s_addr, b, 4u);
}

static void send_nibble(uint8_t high_nibble)
{
    uint8_t v = (uint8_t)((high_nibble & 0xF0u) | LCD_BL);
    uint8_t b[2];
    b[0] = (uint8_t)(v | LCD_EN);
    b[1] = v;
    (void)i2c_write(s_addr, b, 2u);
}

static void cmd(uint8_t c)
{
    send_byte(c, 0u);
    if (c == 0x01u || c == 0x02u) delay_ms(3);
}

int lcd_init(void)
{
    static const uint8_t candidates[] = {
        I2C_ADDR_LCD_A, I2C_ADDR_LCD_B,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E
    };
    uint32_t i;
    uint8_t bl = LCD_BL;

    s_ok = 0;
    for (i = 0; i < sizeof(candidates); i++) {
        if (i2c_probe(candidates[i])) { s_addr = candidates[i]; s_ok = 1; break; }
    }
    if (!s_ok) return -1;

    delay_ms(50);
    (void)i2c_write(s_addr, &bl, 1u);
    send_nibble(0x30); delay_ms(5);
    send_nibble(0x30); delay_ms(1);
    send_nibble(0x30); delay_ms(1);
    send_nibble(0x20); delay_ms(1);          /* 4-bit mode */
    cmd(0x28);                               /* 2 lines, 5x8 */
    cmd(0x08);                               /* display off */
    cmd(0x01);                               /* clear */
    cmd(0x06);                               /* entry: increment */
    cmd(0x0C);                               /* display on, no cursor */

    memset(s_cache, 0, sizeof(s_cache));
    return 0;
}

int     lcd_present(void) { return s_ok; }
uint8_t lcd_address(void) { return s_addr; }

void lcd_clear(void)
{
    if (!s_ok) return;
    cmd(0x01);
    memset(s_cache, 0, sizeof(s_cache));
}

void lcd_line(uint8_t row, const char *text)
{
    char buf[17];
    uint32_t i, n;

    if (row > 1u) return;
    n = (uint32_t)strlen(text);
    if (n > 16u) n = 16u;
    for (i = 0; i < 16u; i++) buf[i] = (i < n) ? text[i] : ' ';
    buf[16] = '\0';

    if (strcmp(buf, s_cache[row]) == 0) return;      /* unchanged */
    memcpy(s_cache[row], buf, 17u);
    if (!s_ok) return;

    cmd((uint8_t)(0x80u | (row ? 0x40u : 0x00u)));
    for (i = 0; i < 16u; i++) send_byte((uint8_t)buf[i], LCD_RS);
}

void lcd_show(const char *line1, const char *line2)
{
    lcd_line(0u, line1);
    lcd_line(1u, line2);
}

/* ======================================================================== */
/* rc522.c */
/* ======================================================================== */
/*
 * rc522.c - MFRC522 driver: REQA -> anticollision (cascade level 1) -> HALT
 *
 * A halted card stays silent until it leaves the field, so holding the card
 * on the reader produces exactly one read.
 */
/* registers */
#define CommandReg      0x01u
#define ComIrqReg       0x04u
#define DivIrqReg       0x05u
#define ErrorReg        0x06u
#define FIFODataReg     0x09u
#define FIFOLevelReg    0x0Au
#define ControlReg      0x0Cu
#define BitFramingReg   0x0Du
#define CollReg         0x0Eu
#define ModeReg         0x11u
#define TxModeReg       0x12u
#define RxModeReg       0x13u
#define TxControlReg    0x14u
#define TxASKReg        0x15u
#define CRCResultRegH   0x21u
#define CRCResultRegL   0x22u
#define ModWidthReg     0x24u
#define RFCfgReg        0x26u
#define TModeReg        0x2Au
#define TPrescalerReg   0x2Bu
#define TReloadRegH     0x2Cu
#define TReloadRegL     0x2Du
#define VersionReg      0x37u
#define Status1Reg      0x07u
#define Status2Reg      0x08u
#define AutoTestReg     0x36u

/* commands */
#define PCD_Idle        0x00u
#define PCD_Mem         0x01u
#define PCD_CalcCRC     0x03u
#define PCD_Transceive  0x0Cu
#define PCD_SoftReset   0x0Fu

#define PICC_REQA       0x26u
#define PICC_WUPA       0x52u
#define PICC_SEL_CL1    0x93u
#define PICC_HLTA       0x50u

#define ST_OK        0
#define ST_ERR      (-1)
#define ST_TIMEOUT  (-2)
#define ST_COLL     (-3)

static uint8_t s_version = 0;

volatile uint8_t  g_rc522_version  = 0;
volatile uint8_t  g_rc522_readback = 0;
volatile uint8_t  g_rc522_txctrl   = 0;
volatile uint8_t  g_rc522_irq      = 0;
volatile uint8_t  g_rc522_err      = 0;
volatile int32_t  g_rc522_status   = 0;
volatile uint32_t g_rc522_polls    = 0;
volatile uint32_t g_rc522_atqa     = 0;

static void wr(uint8_t reg, uint8_t v)
{
    gpio_clr(RC522_NSS_PORT, RC522_NSS_PIN);
    (void)spi_xfer((uint8_t)((reg << 1) & 0x7Eu));
    (void)spi_xfer(v);
    gpio_set(RC522_NSS_PORT, RC522_NSS_PIN);
}

static uint8_t rd(uint8_t reg)
{
    uint8_t v;
    gpio_clr(RC522_NSS_PORT, RC522_NSS_PIN);
    (void)spi_xfer((uint8_t)(((reg << 1) & 0x7Eu) | 0x80u));
    v = spi_xfer(0x00u);
    gpio_set(RC522_NSS_PORT, RC522_NSS_PIN);
    return v;
}

static void set_bits(uint8_t reg, uint8_t mask) { wr(reg, (uint8_t)(rd(reg) | mask)); }
static void clr_bits(uint8_t reg, uint8_t mask) { wr(reg, (uint8_t)(rd(reg) & (uint8_t)~mask)); }

int rc522_init(void)
{
    uint32_t t0;

    gpio_set(RC522_NSS_PORT, RC522_NSS_PIN);
    gpio_mode(RC522_NSS_PORT, RC522_NSS_PIN, GPIO_MODE_OUTPUT);
    gpio_mode(RC522_RST_PORT, RC522_RST_PIN, GPIO_MODE_OUTPUT);
    spi_init();

    gpio_clr(RC522_RST_PORT, RC522_RST_PIN);   /* hard reset */
    delay_ms(2);
    gpio_set(RC522_RST_PORT, RC522_RST_PIN);
    delay_ms(50);

    wr(CommandReg, PCD_SoftReset);
    delay_ms(50);
    t0 = millis();
    while ((rd(CommandReg) & 0x10u) && (millis() - t0) < 100u) { }   /* PowerDown */

    wr(TxModeReg, 0x00u);
    wr(RxModeReg, 0x00u);
    wr(ModWidthReg, 0x26u);
    wr(TModeReg, 0x80u);          /* timer starts after transmission */
    wr(TPrescalerReg, 0xA9u);     /* 40 kHz timer */
    wr(TReloadRegH, 0x03u);       /* 1000 ticks = 25 ms timeout */
    wr(TReloadRegL, 0xE8u);
    wr(TxASKReg, 0x40u);          /* 100 % ASK */
    wr(ModeReg, 0x3Du);           /* CRC preset 0x6363 */
    wr(RFCfgReg, 0x70u);          /* receiver gain 48 dB (better range) */

    if ((rd(TxControlReg) & 0x03u) != 0x03u) set_bits(TxControlReg, 0x03u);   /* antenna on */

    s_version = rd(VersionReg);
    g_rc522_version = s_version;
    g_rc522_readback = rd(ModWidthReg);          /* written 0x26 above */
    g_rc522_txctrl = rd(TxControlReg);
    if (g_rc522_readback != 0x26u) return -1;    /* SPI reads are not reliable */
    return (s_version == 0x00u || s_version == 0xFFu) ? -1 : 0;
}

uint8_t rc522_version(void) { return s_version; }

static int transceive(const uint8_t *send, uint8_t send_len,
                      uint8_t *back, uint8_t *back_len, uint8_t *valid_bits)
{
    uint8_t i, n, err;
    uint8_t tx_last_bits = valid_bits ? *valid_bits : 0u;
    uint32_t t0;

    wr(CommandReg, PCD_Idle);
    wr(ComIrqReg, 0x7Fu);             /* clear IRQ flags */
    wr(FIFOLevelReg, 0x80u);          /* flush FIFO */
    for (i = 0; i < send_len; i++) wr(FIFODataReg, send[i]);
    wr(BitFramingReg, tx_last_bits);
    wr(CommandReg, PCD_Transceive);
    set_bits(BitFramingReg, 0x80u);   /* StartSend */

    t0 = millis();
    for (;;) {
        n = rd(ComIrqReg);
        g_rc522_irq = n;
        if (n & 0x30u) break;                      /* RxIRq | IdleIRq */
        if (n & 0x01u) return ST_TIMEOUT;          /* TimerIRq: no card */
        if ((millis() - t0) > 40u) return ST_TIMEOUT;
    }
    clr_bits(BitFramingReg, 0x80u);

    err = rd(ErrorReg);
    g_rc522_err = err;
    if (err & 0x13u) return ST_ERR;               /* BufferOvfl, Parity, Protocol */

    if (back && back_len) {
        n = rd(FIFOLevelReg);
        if (n > *back_len) return ST_ERR;
        *back_len = n;
        for (i = 0; i < n; i++) back[i] = rd(FIFODataReg);
        if (valid_bits) *valid_bits = (uint8_t)(rd(ControlReg) & 0x07u);
    }
    if (err & 0x08u) return ST_COLL;
    return ST_OK;
}

static int calc_crc(const uint8_t *data, uint8_t len, uint8_t out[2])
{
    uint8_t i;
    uint32_t t0;

    wr(CommandReg, PCD_Idle);
    wr(DivIrqReg, 0x04u);
    wr(FIFOLevelReg, 0x80u);
    for (i = 0; i < len; i++) wr(FIFODataReg, data[i]);
    wr(CommandReg, PCD_CalcCRC);
    t0 = millis();
    while (!(rd(DivIrqReg) & 0x04u)) {
        if ((millis() - t0) > 10u) return ST_TIMEOUT;
    }
    wr(CommandReg, PCD_Idle);
    out[0] = rd(CRCResultRegL);
    out[1] = rd(CRCResultRegH);
    return ST_OK;
}

static void picc_halt(void)
{
    uint8_t buf[4];
    buf[0] = PICC_HLTA;
    buf[1] = 0x00u;
    if (calc_crc(buf, 2u, &buf[2]) != ST_OK) return;
    (void)transceive(buf, 4u, 0, 0, 0);           /* a timeout here = success */
}

int rc522_read_uid(uint8_t uid[4])
{
    uint8_t atqa[2], atqa_len = 2u, bits = 7u;
    uint8_t cmd[2], back[5], back_len = 5u;
    uint8_t req = PICC_REQA;
    int r;

    g_rc522_polls++;
    clr_bits(CollReg, 0x80u);
    r = transceive(&req, 1u, atqa, &atqa_len, &bits);
    g_rc522_status = r;
    if (r != ST_OK || atqa_len != 2u || bits != 0u) return 0;
    g_rc522_atqa++;

    cmd[0] = PICC_SEL_CL1;
    cmd[1] = 0x20u;
    bits = 0u;
    r = transceive(cmd, 2u, back, &back_len, &bits);
    g_rc522_status = r;
    if (r != ST_OK || back_len != 5u) return 0;
    if ((uint8_t)(back[0] ^ back[1] ^ back[2] ^ back[3]) != back[4]) return 0;   /* BCC */

    uid[0] = back[0]; uid[1] = back[1]; uid[2] = back[2]; uid[3] = back[3];
    picc_halt();
    return 1;
}

void rc522_uid_to_hex(const uint8_t uid[4], char out[9])
{
    static const char hx[] = "0123456789ABCDEF";
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        out[2u * i]      = hx[uid[i] >> 4];
        out[2u * i + 1u] = hx[uid[i] & 0x0Fu];
    }
    out[8] = '\0';
}

/* ------------------------------------------------------------------ */
/* debug helpers                                                        */
/* ------------------------------------------------------------------ */
uint8_t rc522_reg_read(uint8_t reg)              { return rd(reg); }
void    rc522_reg_write(uint8_t reg, uint8_t v)  { wr(reg, v); }

int rc522_spi_pattern_test(uint8_t *bad_bits)
{
    static const uint8_t pat[] = {
        0x00, 0xFF, 0x55, 0xAA, 0x01, 0x02, 0x04, 0x08,
        0x10, 0x20, 0x40, 0x80, 0x0F, 0xF0, 0x3C, 0xC3
    };
    static const uint8_t regs[] = { TReloadRegL, TPrescalerReg };
    uint8_t keep[2], i, j, v;
    int errors = 0;

    *bad_bits = 0u;
    keep[0] = rd(TReloadRegL);
    keep[1] = rd(TPrescalerReg);
    for (j = 0; j < 2u; j++) {
        for (i = 0; i < sizeof(pat); i++) {
            wr(regs[j], pat[i]);
            v = rd(regs[j]);
            if (v != pat[i]) { errors++; *bad_bits |= (uint8_t)(v ^ pat[i]); }
        }
    }
    wr(TReloadRegL, keep[0]);
    wr(TPrescalerReg, keep[1]);
    return errors;
}

int rc522_self_test(uint8_t out[64])
{
    static const uint8_t v1[8] = { 0x00, 0xC6, 0x37, 0xD5, 0x32, 0xB7, 0x57, 0x5C };
    static const uint8_t v2[8] = { 0x00, 0xEB, 0x66, 0xBA, 0x57, 0xBF, 0x23, 0x95 };
    uint32_t t0;
    uint8_t i, n = 0;
    int m1 = 1, m2 = 1;

    for (i = 0; i < 64u; i++) out[i] = 0u;

    wr(CommandReg, PCD_SoftReset);
    delay_ms(50);
    wr(FIFOLevelReg, 0x80u);
    for (i = 0; i < 25u; i++) wr(FIFODataReg, 0x00u);
    wr(CommandReg, PCD_Mem);                    /* clear internal buffer */
    delay_ms(5);
    wr(AutoTestReg, 0x09u);                     /* enable self test */
    wr(FIFODataReg, 0x00u);
    wr(CommandReg, PCD_CalcCRC);

    t0 = millis();
    while ((millis() - t0) < 100u) {
        n = rd(FIFOLevelReg) & 0x7Fu;
        if (n >= 64u) break;
    }
    wr(CommandReg, PCD_Idle);
    if (n > 64u) n = 64u;
    for (i = 0; i < n; i++) out[i] = rd(FIFODataReg);
    wr(AutoTestReg, 0x00u);

    (void)rc522_init();                          /* restore normal settings */
    if (n < 64u) return -1;
    for (i = 0; i < 8u; i++) {
        if (out[i] != v1[i]) m1 = 0;
        if (out[i] != v2[i]) m2 = 0;
    }
    return m2 ? 2 : (m1 ? 1 : 0);
}

void rc522_probe(int wupa, rc522_probe_t *p)
{
    uint32_t t0;
    uint8_t n, i;

    p->result = 1;
    p->irq = p->err = p->fifo = p->lastbits = 0u;
    p->atqa[0] = p->atqa[1] = 0u;

    clr_bits(CollReg, 0x80u);
    wr(CommandReg, PCD_Idle);
    wr(ComIrqReg, 0x7Fu);
    wr(FIFOLevelReg, 0x80u);
    wr(FIFODataReg, wupa ? PICC_WUPA : PICC_REQA);
    wr(BitFramingReg, 0x07u);                   /* 7-bit short frame */
    wr(CommandReg, PCD_Transceive);
    set_bits(BitFramingReg, 0x80u);

    t0 = millis();
    for (;;) {
        n = rd(ComIrqReg);
        if (n & 0x30u) break;
        if (n & 0x01u) break;                   /* timer: nobody answered */
        if ((millis() - t0) > 40u) { p->result = -4; break; }
    }
    clr_bits(BitFramingReg, 0x80u);
    wr(CommandReg, PCD_Idle);

    p->irq = rd(ComIrqReg);
    p->err = rd(ErrorReg);
    p->fifo = (uint8_t)(rd(FIFOLevelReg) & 0x7Fu);
    p->lastbits = (uint8_t)(rd(ControlReg) & 0x07u);
    for (i = 0; i < p->fifo && i < 2u; i++) p->atqa[i] = rd(FIFODataReg);
    g_rc522_irq = p->irq;
    g_rc522_err = p->err;

    if (p->result == -4) return;
    if ((p->irq & 0x20u) && p->fifo == 2u && (p->err & 0x1Bu) == 0u) p->result = 0;
    else if ((p->irq & 0x20u) || p->fifo > 0u || (p->err & 0x1Bu)) p->result = 2;
    else p->result = 1;
}

/* ======================================================================== */
/* ds3231.c */
/* ======================================================================== */
/*
 * ds3231.c - DS3231 RTC driver (24-hour mode, BCD registers 0x00..0x06)
 */
#define REG_SECONDS 0x00u
#define REG_CONTROL 0x0Eu
#define REG_STATUS  0x0Fu

static uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10u + (v & 0x0Fu)); }
static uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10u) << 4) | (v % 10u)); }

int ds3231_present(void) { return i2c_probe(I2C_ADDR_DS3231); }

int ds3231_read(rtc_time_t *t)
{
    uint8_t r[7];
    uint8_t h;

    if (i2c_read_regs(I2C_ADDR_DS3231, REG_SECONDS, r, 7u) != I2C_OK) return -1;

    t->second = bcd2bin(r[0] & 0x7Fu);
    t->minute = bcd2bin(r[1] & 0x7Fu);
    h = r[2];
    if (h & 0x40u) {                           /* 12-hour mode */
        uint8_t hr = bcd2bin(h & 0x1Fu);
        if (hr == 12u) hr = 0u;
        if (h & 0x20u) hr = (uint8_t)(hr + 12u);
        t->hour = hr;
    } else {
        t->hour = bcd2bin(h & 0x3Fu);
    }
    t->day   = bcd2bin(r[4] & 0x3Fu);
    t->month = bcd2bin(r[5] & 0x1Fu);
    t->year  = (uint16_t)(2000u + bcd2bin(r[6]));
    return 0;
}

static uint8_t day_of_week(uint16_t y, uint8_t m, uint8_t d)   /* 1 = Monday .. 7 */
{
    static const uint8_t k[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    uint16_t yy = (m < 3u) ? (uint16_t)(y - 1u) : y;
    uint8_t w = (uint8_t)((yy + yy / 4u - yy / 100u + yy / 400u + k[m - 1u] + d) % 7u); /* 0 = Sunday */
    return (uint8_t)(w == 0u ? 7u : w);
}

int ds3231_set(const rtc_time_t *t)
{
    uint8_t b[8];
    uint8_t st;

    b[0] = REG_SECONDS;
    b[1] = bin2bcd(t->second);
    b[2] = bin2bcd(t->minute);
    b[3] = bin2bcd(t->hour);                  /* bit6 = 0 -> 24-hour */
    b[4] = day_of_week(t->year, t->month, t->day);
    b[5] = bin2bcd(t->day);
    b[6] = bin2bcd(t->month);
    b[7] = bin2bcd((uint8_t)(t->year - 2000u));
    if (i2c_write(I2C_ADDR_DS3231, b, 8u) != I2C_OK) return -1;

    /* clear the oscillator-stop flag */
    if (i2c_read_regs(I2C_ADDR_DS3231, REG_STATUS, &st, 1u) != I2C_OK) return -1;
    return i2c_write_reg(I2C_ADDR_DS3231, REG_STATUS, (uint8_t)(st & 0x7Fu));
}

int ds3231_lost_power(void)
{
    uint8_t st;
    if (i2c_read_regs(I2C_ADDR_DS3231, REG_STATUS, &st, 1u) != I2C_OK) return 0;
    return (st & 0x80u) ? 1 : 0;
}

static uint8_t two_digits(const char *s)
{
    uint8_t hi = (s[0] >= '0' && s[0] <= '9') ? (uint8_t)(s[0] - '0') : 0u;
    return (uint8_t)(hi * 10u + (uint8_t)(s[1] - '0'));
}

int ds3231_set_from_build_time(void)
{
    static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *d = __DATE__;                  /* "Oct 21 2026" */
    const char *tm = __TIME__;                 /* "10:27:00"    */
    rtc_time_t t;
    uint8_t m;

    t.month = 1u;
    for (m = 0; m < 12u; m++) {
        if (strncmp(d, &months[m * 3u], 3u) == 0) { t.month = (uint8_t)(m + 1u); break; }
    }
    t.day    = two_digits(&d[4]);
    t.year   = (uint16_t)(2000u + two_digits(&d[9]));
    t.hour   = two_digits(&tm[0]);
    t.minute = two_digits(&tm[3]);
    t.second = two_digits(&tm[6]);
    return ds3231_set(&t);
}

int ds3231_init(void)
{
    uint8_t ctrl;

    if (!ds3231_present()) return -1;
    /* EOSC = 0 keeps the oscillator running on battery; INTCN = 1 (no square wave) */
    if (i2c_read_regs(I2C_ADDR_DS3231, REG_CONTROL, &ctrl, 1u) == I2C_OK) {
        (void)i2c_write_reg(I2C_ADDR_DS3231, REG_CONTROL, (uint8_t)((ctrl & 0x7Fu) | 0x04u));
    }
#if RTC_FORCE_SET
    (void)ds3231_set_from_build_time();
#elif RTC_SET_FROM_BUILD_TIME
    if (ds3231_lost_power()) (void)ds3231_set_from_build_time();
#endif
    return 0;
}

void ds3231_format(const rtc_time_t *t, char out[20])
{
    snprintf(out, 20, "%04u-%02u-%02u %02u:%02u:%02u",
             (unsigned)t->year, (unsigned)t->month, (unsigned)t->day,
             (unsigned)t->hour, (unsigned)t->minute, (unsigned)t->second);
}

void ds3231_timestamp(char out[20])
{
    rtc_time_t t;
    if (ds3231_read(&t) == 0) {
        ds3231_format(&t, out);
    } else {
        strcpy(out, "2000-01-01 00:00:00");
    }
}

/* ======================================================================== */
/* max30102.c */
/* ======================================================================== */
/*
 * max30102.c - MAX30102 driver (polling the FIFO, no INT pin needed)
 */
#define REG_INTR_EN1    0x02u
#define REG_INTR_EN2    0x03u
#define REG_FIFO_WR     0x04u
#define REG_OVF         0x05u
#define REG_FIFO_RD     0x06u
#define REG_FIFO_DATA   0x07u
#define REG_FIFO_CFG    0x08u
#define REG_MODE_CFG    0x09u
#define REG_SPO2_CFG    0x0Au
#define REG_LED1_PA     0x0Cu   /* red */
#define REG_LED2_PA     0x0Du   /* IR  */
#define REG_PART_ID     0xFFu

static uint8_t s_part = 0;

int max30102_init(void)
{
    uint8_t v = 0;
    uint32_t t0;

    if (i2c_read_regs(I2C_ADDR_MAX30102, REG_PART_ID, &s_part, 1u) != I2C_OK) { s_part = 0; return -1; }
    if (s_part != 0x15u) return -1;

    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_MODE_CFG, 0x40u);        /* reset */
    t0 = millis();
    do {
        delay_ms(2);
        if (i2c_read_regs(I2C_ADDR_MAX30102, REG_MODE_CFG, &v, 1u) != I2C_OK) return -1;
    } while ((v & 0x40u) && (millis() - t0) < 100u);

    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_INTR_EN1, 0x00u);
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_INTR_EN2, 0x00u);
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_FIFO_WR, 0x00u);
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_OVF, 0x00u);
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_FIFO_RD, 0x00u);
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_FIFO_CFG, 0x5Fu);   /* avg 4, rollover on */
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_SPO2_CFG, 0x2Bu);   /* 4096 nA, 200 sps, 411 us */
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_LED1_PA, 0x24u);    /* ~7 mA */
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_LED2_PA, 0x24u);
    (void)i2c_write_reg(I2C_ADDR_MAX30102, REG_MODE_CFG, 0x03u);   /* SpO2 mode (red + IR) */
    return 0;
}

uint8_t max30102_part_id(void) { return s_part; }

int max30102_read_fifo(uint32_t *red, uint32_t *ir, int max, int *overflow)
{
    uint8_t ptr[3];
    uint8_t buf[6 * 5];
    int n, done = 0;

    *overflow = 0;
    if (i2c_read_regs(I2C_ADDR_MAX30102, REG_FIFO_WR, ptr, 3u) != I2C_OK) return 0;

    n = (int)(((uint32_t)ptr[0] - (uint32_t)ptr[2]) & 0x1Fu);
    if (ptr[1] != 0u) { n = 32; *overflow = 1; }
    if (n > max) n = max;

    while (done < n) {
        int k = n - done;
        int i;
        if (k > 5) k = 5;
        if (i2c_read_regs(I2C_ADDR_MAX30102, REG_FIFO_DATA, buf, (uint32_t)(k * 6)) != I2C_OK) break;
        for (i = 0; i < k; i++) {
            const uint8_t *p = &buf[i * 6];
            red[done + i] = (((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2]) & 0x3FFFFu;
            ir[done + i]  = (((uint32_t)p[3] << 16) | ((uint32_t)p[4] << 8) | p[5]) & 0x3FFFFu;
        }
        done += k;
    }
    return done;
}

/* ======================================================================== */
/* hr_algo.c */
/* ======================================================================== */
/*
 * hr_algo.c - beat detection on the IR photoplethysmogram (50 samples/s)
 *
 *  1. finger detect   : IR > FINGER_IR_THRESHOLD
 *  2. DC removal      : IIR baseline, signal = baseline - IR
 *                       (blood volume rises -> IR falls -> signal peaks)
 *  3. smoothing       : 5-sample moving average (100 ms)
 *  4. peak detection  : max/min envelopes give the pulse range; the signal
 *                       must rise above 60 % of the range and fall back
 *                       below 40 % (hysteresis). The highest point in that
 *                       excursion is the beat. A refractory period
 *                       (>= 300 ms, or 60 % of the beat interval) and the
 *                       60 % level ignore the dicrotic notch.
 *  5. BPM             : mean of the last 5 accepted intervals; intervals
 *                       more than 35 % away from the mean are rejected
 *
 * Basic monitoring only - not a medical measurement.
 */
#define FS_MS          MAX30102_SAMPLE_MS
#define MA_LEN         5
#define SETTLE_SAMPLES 50          /* ignore the first second after finger placed */
#define IV_MAX         5
#define MIN_AMPLITUDE  40.0f       /* counts peak-to-peak; below this is noise */
#define BEAT_TIMEOUT_MS 3000u

static struct {
    int      finger;
    uint32_t t;                    /* sample clock in ms */
    float    dc;
    float    ma[MA_LEN];
    float    ma_sum;
    int      ma_i;
    float    env_max, env_min;
    int      above;
    float    peak;
    uint32_t peak_t;
    int      settle;
    int      have_last;
    uint32_t last_beat;
    uint32_t iv[IV_MAX];
    int      iv_n, iv_i;
    int      rejects;
} s;

static void clear_intervals(void)
{
    s.iv_n = 0;
    s.iv_i = 0;
    s.rejects = 0;
}

void hr_reset(void)
{
    int i;
    s.finger = 0;
    s.dc = 0.0f;
    for (i = 0; i < MA_LEN; i++) s.ma[i] = 0.0f;
    s.ma_sum = 0.0f;
    s.ma_i = 0;
    s.env_max = s.env_min = 0.0f;
    s.above = 0;
    s.peak = 0.0f;
    s.peak_t = 0u;
    s.settle = SETTLE_SAMPLES;
    s.have_last = 0;
    clear_intervals();
}

void hr_mark_gap(void)
{
    s.have_last = 0;
}

static uint32_t mean_interval(void)
{
    uint32_t sum = 0;
    int i;
    for (i = 0; i < s.iv_n; i++) sum += s.iv[i];
    return s.iv_n ? sum / (uint32_t)s.iv_n : 0u;
}

static void add_interval(uint32_t iv)
{
    if (s.iv_n >= 2) {
        uint32_t m = mean_interval();
        uint32_t diff = (iv > m) ? iv - m : m - iv;
        if (diff * 100u > m * 35u) {               /* outlier */
            if (++s.rejects >= 3) clear_intervals(); /* rhythm changed: start over */
            return;
        }
    }
    s.rejects = 0;
    s.iv[s.iv_i] = iv;
    s.iv_i = (s.iv_i + 1) % IV_MAX;
    if (s.iv_n < IV_MAX) s.iv_n++;
}

static void accept_beat(uint32_t tb)
{
    uint32_t refractory = 300u;
    uint32_t m = mean_interval();
    if (s.iv_n >= 2 && (m * 6u) / 10u > refractory) refractory = (m * 6u) / 10u;

    if (!s.have_last) {
        s.have_last = 1;
        s.last_beat = tb;
    } else if ((tb - s.last_beat) >= refractory) {
        uint32_t iv = tb - s.last_beat;
        s.last_beat = tb;
        if (iv <= 2000u) add_interval(iv);       /* 30..200 BPM */
        else clear_intervals();
    }
}

int hr_add_sample(uint32_t ir)
{
    float x, y, range, hi, lo;
    int beat = 0;
    int i;

    s.t += FS_MS;

    if (ir < FINGER_IR_THRESHOLD) {
        if (s.finger) hr_reset();
        s.finger = 0;
        return 0;
    }
    if (!s.finger) {
        hr_reset();
        s.finger = 1;
        s.dc = (float)ir;
    }

    s.dc += 0.04f * ((float)ir - s.dc);
    x = s.dc - (float)ir;

    s.ma_sum += x - s.ma[s.ma_i];
    s.ma[s.ma_i] = x;
    s.ma_i = (s.ma_i + 1) % MA_LEN;
    y = s.ma_sum / (float)MA_LEN;

    if (s.settle > 0) {
        s.settle--;
        if (s.settle == 0) {                     /* rebuild the average cleanly */
            s.ma_sum = 0.0f;
            for (i = 0; i < MA_LEN; i++) s.ma_sum += s.ma[i];
        }
        s.env_max = s.env_min = y;
        return 0;
    }

    /* envelopes: jump to new extremes, otherwise relax towards each other */
    range = s.env_max - s.env_min;
    if (y > s.env_max) s.env_max = y; else s.env_max -= 0.005f * range;
    if (y < s.env_min) s.env_min = y; else s.env_min += 0.005f * range;
    range = s.env_max - s.env_min;
    hi = s.env_min + 0.6f * range;
    lo = s.env_min + 0.4f * range;

    if (range < MIN_AMPLITUDE) {                 /* too flat: no pulse */
        s.above = 0;
    } else if (!s.above) {
        if (y > hi) { s.above = 1; s.peak = y; s.peak_t = s.t; }
    } else {
        if (y > s.peak) { s.peak = y; s.peak_t = s.t; }
        if (y < lo) {
            s.above = 0;
            accept_beat(s.peak_t);
            beat = 1;
        }
    }

    if (s.have_last && (s.t - s.last_beat) > BEAT_TIMEOUT_MS) {
        s.have_last = 0;
        clear_intervals();
    }
    return beat;
}

int hr_finger_present(void) { return s.finger; }

int hr_bpm(void)
{
    uint32_t m;
    int bpm;
    if (!s.finger || s.iv_n < 3) return 0;
    m = mean_interval();
    if (m == 0u) return 0;
    bpm = (int)((60000u + m / 2u) / m);
    if (bpm < 35 || bpm > 200) return 0;
    return bpm;
}

/* ======================================================================== */
/* sensors.c */
/* ======================================================================== */
/*
 * sensors.c - analog sensors and ultrasonic ranging
 *   ADC1: 12-bit, clock 84/4 = 21 MHz, single software conversion
 *   LM35 : 10 mV/C  -> with Vref = 3.3 V, millivolts == tenths of a degree
 *   KY-037 AO: peak-to-peak amplitude over a short window
 *   HC-SR04  : 10 us trigger, echo width measured with TIM2 (1 us)
 */
void sensors_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;

    gpio_mode(GPIOA, ADC_CH_LM35, GPIO_MODE_ANALOG);    /* PA1 */
    gpio_mode(GPIOA, ADC_CH_SOUND, GPIO_MODE_ANALOG);   /* PA2 */

    ADC_COMMON->CCR = (ADC_COMMON->CCR & ~(3u << 16)) | (1u << 16);   /* PCLK2 / 4 */
    ADC1->CR1 = 0u;                                     /* 12-bit, no scan */
    ADC1->CR2 = 0u;
    ADC1->SMPR2 = (7u << (3u * ADC_CH_LM35))            /* 480 cycles: high-impedance LM35 */
                | (4u << (3u * ADC_CH_SOUND));          /*  84 cycles: fast sound sampling  */
    ADC1->SQR1 = 0u;                                    /* 1 conversion */
    ADC1->CR2 = ADC_CR2_ADON;
    delay_us(10);

    /* HC-SR04 */
    gpio_clr(US_TRIG_PORT, US_TRIG_PIN);
    gpio_mode(US_TRIG_PORT, US_TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_pull(US_ECHO_PORT, US_ECHO_PIN, GPIO_PULL_DOWN);
    gpio_mode(US_ECHO_PORT, US_ECHO_PIN, GPIO_MODE_INPUT);
}

uint16_t adc_read(uint32_t channel)
{
    uint32_t guard = 100000u;
    ADC1->SQR3 = channel & 0x1Fu;
    ADC1->SR = 0u;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC) && --guard) { }
    return (uint16_t)(ADC1->DR & 0x0FFFu);
}

int sound_p2p(uint32_t window_ms, uint16_t *last_raw)
{
    uint16_t lo = 4095u, hi = 0u, v = 0u;
    uint32_t t0 = micros();
    uint32_t span = window_ms * 1000u;

    while ((micros() - t0) < span) {
        v = adc_read(ADC_CH_SOUND);
        if (v < lo) lo = v;
        if (v > hi) hi = v;
        delay_us(200);
    }
    if (last_raw) *last_raw = v;
    return (hi >= lo) ? (int)(hi - lo) : 0;
}

int lm35_read_c10(void)
{
    uint32_t sum = 0;
    uint32_t i;
    for (i = 0; i < 32u; i++) sum += adc_read(ADC_CH_LM35);
    /* mV = avg * 3300 / 4095 ; tenths of C == mV */
    return (int)((sum * 3300u + (32u * 4095u) / 2u) / (32u * 4095u));
}

int ultrasonic_cm(void)
{
    uint32_t t0, rise, width;

    gpio_set(US_TRIG_PORT, US_TRIG_PIN);
    delay_us(10);
    gpio_clr(US_TRIG_PORT, US_TRIG_PIN);

    t0 = micros();
    while (!gpio_read(US_ECHO_PORT, US_ECHO_PIN)) {
        if ((micros() - t0) > 30000u) return US_NO_SENSOR;
    }
    rise = micros();
    while (gpio_read(US_ECHO_PORT, US_ECHO_PIN)) {
        if ((micros() - rise) > 30000u) return US_OUT_OF_RANGE;
    }
    width = micros() - rise;
    if (width > 23500u) return US_OUT_OF_RANGE;          /* > ~4 m */
    return (int)((width + 29u) / 58u);                   /* us / 58 = cm */
}

/* ======================================================================== */
/* buttons.c */
/* ======================================================================== */
/*
 * buttons.c - button handling
 *
 * PB9 uses EXTI line 9 on BOTH edges. The interrupt reads the pin level and
 * accepts a change only if the opposite edge was at least BUTTON_DEBOUNCE_MS
 * ago, so contact bounce is ignored. Every accepted press increments a
 * counter. SysTick (1 ms) closes a press sequence after CLICK_GAP_MS of
 * idle and turns it into a Mode or SOS request. Because both run in
 * interrupts, presses are caught even while the main loop waits on Wi-Fi.
 */
volatile uint8_t  g_sos_flag = 0;
volatile uint32_t g_sos_irq_count = 0;
volatile uint32_t g_btn_last_clicks = 0;
volatile uint8_t  g_btn_idle_level = 1;   /* detected idle level of PB9 */

/* PB9 state, shared by EXTI and SysTick */
static volatile uint8_t  s_pressed = 0;
static volatile uint32_t s_last_press_ms = 0;
static volatile uint32_t s_last_release_ms = 0;
static volatile uint32_t s_clicks = 0;
#if SINGLE_BUTTON
static volatile uint8_t  s_mode_flag = 0;
#else
/* PB8 (two-button mode only) */
static int      s_mode_stable = 0;
static int      s_mode_last_raw = 0;
static uint32_t s_mode_changed_ms = 0;
#endif

/* Button polarity is detected at start-up: the level each pin has while
 * nobody presses it is the "released" level, the other level is "pressed".
 * Works for both G-V-S module types (LOW-when-pressed and HIGH-when-pressed).
 * Do not hold the button while the board starts. */
static uint8_t s_idle_mode = 1u;   /* idle level of PB8 */
static uint8_t s_idle_sos  = 1u;   /* idle level of PB9 */

static uint8_t sample_idle(GPIO_TypeDef *port, uint32_t pin)
{
    uint32_t i, ones = 0;
    for (i = 0; i < 16u; i++) {
        ones += (uint32_t)gpio_read(port, pin);
        delay_us(500);
    }
    return (ones >= 8u) ? 1u : 0u;
}

#if !SINGLE_BUTTON
static int mode_pin_pressed(void) { return gpio_read(BTN_MODE_PORT, BTN_MODE_PIN) != (int)s_idle_mode; }
#endif

int button_sos_down(void) { return gpio_read(BTN_SOS_PORT, BTN_SOS_PIN) != (int)s_idle_sos; }

int button_mode_down(void)
{
#if SINGLE_BUTTON
    return button_sos_down();
#else
    return mode_pin_pressed();
#endif
}

void buttons_init(void)
{
    uint32_t pull = BUTTON_ACTIVE_LOW ? GPIO_PULL_UP : GPIO_PULL_DOWN;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    gpio_pull(BTN_MODE_PORT, BTN_MODE_PIN, pull);
    gpio_mode(BTN_MODE_PORT, BTN_MODE_PIN, GPIO_MODE_INPUT);
    gpio_pull(BTN_SOS_PORT, BTN_SOS_PIN, pull);
    gpio_mode(BTN_SOS_PORT, BTN_SOS_PIN, GPIO_MODE_INPUT);

    delay_ms(5);                                    /* let the inputs settle */
    s_idle_mode = sample_idle(BTN_MODE_PORT, BTN_MODE_PIN);
    s_idle_sos  = sample_idle(BTN_SOS_PORT, BTN_SOS_PIN);
    g_btn_idle_level = s_idle_sos;

    /* EXTI9 <- port B (EXTICR3 bits 7:4 = 0001), both edges */
    SYSCFG->EXTICR[2] = (SYSCFG->EXTICR[2] & ~(0xFu << 4)) | (1u << 4);
    EXTI->FTSR |= (1u << 9);
    EXTI->RTSR |= (1u << 9);
    EXTI->PR = (1u << 9);
    EXTI->IMR |= (1u << 9);

    s_pressed = (uint8_t)button_sos_down();
    s_clicks = 0u;

    nvic_set_priority(IRQN_EXTI9_5, 6u);
    nvic_enable_irq(IRQN_EXTI9_5);

#if !SINGLE_BUTTON
    s_mode_last_raw = mode_pin_pressed();
    s_mode_stable = s_mode_last_raw;
#endif
}

void EXTI9_5_IRQHandler(void)
{
    uint32_t now;
    int down;

    if (!(EXTI->PR & (1u << 9))) return;
    EXTI->PR = (1u << 9);                           /* write 1 to clear */

    now = g_ms_ticks;
    down = button_sos_down();

    if (down && !s_pressed) {
        if ((now - s_last_release_ms) >= BUTTON_DEBOUNCE_MS) {
            s_pressed = 1u;
            s_last_press_ms = now;
            g_sos_irq_count++;
#if SINGLE_BUTTON
            s_clicks++;
            buzzer_beep(1u, 25u, 0u);               /* click feedback */
#else
            g_sos_flag = 1u;                        /* two-button mode: 1 press = SOS */
#endif
        }
    } else if (!down && s_pressed) {
        if ((now - s_last_press_ms) >= BUTTON_DEBOUNCE_MS) {
            s_pressed = 0u;
            s_last_release_ms = now;
        }
    }
}

/* 1 ms housekeeping, runs inside SysTick_Handler */
void buttons_tick_1ms(void)
{
    uint32_t now = g_ms_ticks;

    /* a very short tap whose release edge fell inside the debounce window */
    if (s_pressed && !button_sos_down() && (now - s_last_press_ms) >= BUTTON_DEBOUNCE_MS) {
        s_pressed = 0u;
        s_last_release_ms = now;
    }

#if SINGLE_BUTTON
    if (s_clicks != 0u && !s_pressed && (now - s_last_release_ms) >= CLICK_GAP_MS) {
        uint32_t pm = irq_save();                   /* EXTI must not add a click here */
        uint32_t n = s_clicks;
        s_clicks = 0u;
        irq_restore(pm);
        g_btn_last_clicks = n;
        if (n >= SOS_PRESS_COUNT) g_sos_flag = 1u;
        else if (n == 1u)         s_mode_flag = 1u;
        /* 2 presses (when SOS needs 3): ignored on purpose */
    }
#endif
}

int button_mode_event(void)
{
#if SINGLE_BUTTON
    if (s_mode_flag) {
        s_mode_flag = 0u;
        return 1;
    }
    return 0;
#else
    int raw = mode_pin_pressed();
    uint32_t now = millis();

    if (raw != s_mode_last_raw) {
        s_mode_last_raw = raw;
        s_mode_changed_ms = now;
        return 0;
    }
    if (raw != s_mode_stable && (now - s_mode_changed_ms) >= 30u) {
        s_mode_stable = raw;
        return raw;
    }
    return 0;
#endif
}

/* ======================================================================== */
/* esp8266.c */
/* ======================================================================== */
/*
 * esp8266.c - AT-command HTTP/1.1 client for the NodeMCU (AT firmware 1.x)
 *
 * One request per TCP connection:
 *   AT+CIPSTART="TCP","<ip>",<port>  -> CONNECT / OK
 *   AT+CIPSEND=<n>                   -> '>'
 *   <request bytes>                  -> SEND OK
 *   +IPD,<len>:<response bytes> ...  -> CLOSED
 * The +IPD payloads are reassembled, then the status line and body parsed.
 */
#define WIN_LEN      48
#define RESP_MAX     768

/* ------------------------------------------------------------------ */
/* low level                                                            */
/* ------------------------------------------------------------------ */

/* Wait until the received stream ends with one of the tokens.
 * Returns the token index, or -1 on timeout. Optionally captures text. */
static int wait_tokens(const char *const *tok, int ntok, uint32_t timeout_ms,
                       char *cap, int capmax)
{
    char win[WIN_LEN];
    int wl = 0, cl = 0, i;
    uint32_t t0 = millis();
    char c;

    win[0] = '\0';
    if (cap && capmax > 0) cap[0] = '\0';

    while ((millis() - t0) < timeout_ms) {
        if (!uart_getc(&c)) continue;
        if (cap && cl < capmax - 1) { cap[cl++] = c; cap[cl] = '\0'; }
        if (wl == WIN_LEN - 1) { memmove(win, win + 1, (size_t)(WIN_LEN - 2)); wl--; }
        win[wl++] = c;
        win[wl] = '\0';
        for (i = 0; i < ntok; i++) {
            int tl = (int)strlen(tok[i]);
            if (wl >= tl && memcmp(&win[wl - tl], tok[i], (size_t)tl) == 0) return i;
        }
    }
    return -1;
}

static void send_line(const char *cmd)
{
    uart_flush_rx();
    uart_puts(cmd);
    uart_puts("\r\n");
}

int esp_cmd(const char *cmd, uint32_t timeout_ms)
{
    static const char *const tok[] = { "OK\r\n", "ERROR", "FAIL" };
    send_line(cmd);
    return (wait_tokens(tok, 3, timeout_ms, 0, 0) == 0) ? ESP_OK : ESP_ERR_NO_REPLY;
}

static int cmd_capture(const char *cmd, char *out, int outmax, uint32_t timeout_ms)
{
    static const char *const tok[] = { "OK\r\n", "ERROR" };
    send_line(cmd);
    return (wait_tokens(tok, 2, timeout_ms, out, outmax) == 0) ? ESP_OK : ESP_ERR_NO_REPLY;
}

/* ------------------------------------------------------------------ */
/* setup                                                                */
/* ------------------------------------------------------------------ */

int esp_init(void)
{
    int i, ok = 0;

    uart_init(ESP_BAUD);
    delay_ms(100);
    for (i = 0; i < 5 && !ok; i++) {
        if (esp_cmd("AT", 500u) == ESP_OK) ok = 1;
        else delay_ms(200);
    }
    if (!ok) return ESP_ERR_NO_REPLY;

    (void)esp_cmd("ATE0", 1000u);           /* no echo */
    (void)esp_cmd("AT+CWMODE=1", 2000u);    /* station */
    (void)esp_cmd("AT+CIPMUX=0", 1000u);    /* single connection */
    (void)esp_cmd("AT+CIPMODE=0", 1000u);   /* normal (non-transparent) */
    return ESP_OK;
}

/* AT strings must escape  "  ,  \  with a backslash */
static int append_escaped(char *dst, int pos, int max, const char *s)
{
    while (*s && pos < max - 2) {
        if (*s == '"' || *s == ',' || *s == '\\') dst[pos++] = '\\';
        dst[pos++] = *s++;
    }
    dst[pos] = '\0';
    return pos;
}

int esp_get_ip(char *ip, int ipmax)
{
    char buf[200];
    const char *p, *q;
    int n;

    if (cmd_capture("AT+CIFSR", buf, (int)sizeof(buf), 3000u) != ESP_OK) return ESP_ERR_NO_REPLY;
    p = strstr(buf, "STAIP,\"");
    if (!p) return ESP_ERR_RESPONSE;
    p += 7;
    q = strchr(p, '"');
    if (!q) return ESP_ERR_RESPONSE;
    n = (int)(q - p);
    if (n >= ipmax) n = ipmax - 1;
    memcpy(ip, p, (size_t)n);
    ip[n] = '\0';
    if (strcmp(ip, "0.0.0.0") == 0) return ESP_ERR_CONNECT;
    return ESP_OK;
}

int esp_join(const char *ssid, const char *pass, char *ip, int ipmax)
{
    static const char *const tok[] = { "OK\r\n", "FAIL", "ERROR" };
    char cmd[160];
    int pos;

    strcpy(cmd, "AT+CWJAP=\"");
    pos = append_escaped(cmd, (int)strlen(cmd), (int)sizeof(cmd), ssid);
    if (pos < (int)sizeof(cmd) - 4) { strcat(cmd, "\",\""); pos += 3; }
    pos = append_escaped(cmd, pos, (int)sizeof(cmd), pass);
    if (pos < (int)sizeof(cmd) - 2) strcat(cmd, "\"");

    send_line(cmd);
    if (wait_tokens(tok, 3, 20000u, 0, 0) != 0) return ESP_ERR_CONNECT;
    return esp_get_ip(ip, ipmax);
}

int esp_link_up(void)
{
    char buf[160];
    const char *p;
    if (cmd_capture("AT+CIPSTATUS", buf, (int)sizeof(buf), 2000u) != ESP_OK) return 0;
    p = strstr(buf, "STATUS:");
    if (!p) return 0;
    return (p[7] >= '2' && p[7] <= '4') ? 1 : 0;   /* 5 = not connected to AP */
}

int esp_version(char *out, int outmax)
{
    char buf[300];
    const char *p;
    int n = 0;

    if (cmd_capture("AT+GMR", buf, (int)sizeof(buf), 2000u) != ESP_OK) return ESP_ERR_NO_REPLY;
    p = buf;
    while (*p == '\r' || *p == '\n') p++;
    if (strncmp(p, "AT+GMR", 6) == 0) {                 /* echo was on */
        while (*p && *p != '\n') p++;
        while (*p == '\r' || *p == '\n') p++;
    }
    while (p[n] && p[n] != '\r' && p[n] != '\n' && n < outmax - 1) n++;
    memcpy(out, p, (size_t)n);
    out[n] = '\0';
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/* HTTP POST                                                            */
/* ------------------------------------------------------------------ */

/* Collect +IPD payloads until CLOSED, a complete body, or timeout. */
static int read_ipd_response(char *resp, int respmax, uint32_t timeout_ms, int *closed)
{
    enum { S_SCAN, S_LEN, S_DATA } st = S_SCAN;
    char win[12];
    int wl = 0, rl = 0, ipd_len = 0, remaining = 0;
    uint32_t t0 = millis();
    char c;

    *closed = 0;
    resp[0] = '\0';
    while ((millis() - t0) < timeout_ms) {
        if (!uart_getc(&c)) continue;

        if (st == S_DATA) {
            if (rl < respmax - 1) { resp[rl++] = c; resp[rl] = '\0'; }
            if (--remaining == 0) {
                const char *hdr_end;
                st = S_SCAN;
                wl = 0;
                /* stop early when Content-Length bytes have arrived */
                hdr_end = strstr(resp, "\r\n\r\n");
                if (hdr_end) {
                    const char *cl = strstr(resp, "Content-Length:");
                    if (!cl) cl = strstr(resp, "content-length:");
                    if (cl && cl < hdr_end) {
                        int want = atoi(cl + 15);
                        int have = rl - (int)((hdr_end + 4) - resp);
                        if (have >= want) return rl;
                    }
                }
            }
            continue;
        }

        if (st == S_LEN) {
            if (c >= '0' && c <= '9') { ipd_len = ipd_len * 10 + (c - '0'); continue; }
            if (c == ':' && ipd_len > 0) { st = S_DATA; remaining = ipd_len; continue; }
            st = S_SCAN;                 /* malformed header: resync */
            wl = 0;
            continue;
        }

        /* S_SCAN */
        if (wl == (int)sizeof(win) - 1) { memmove(win, win + 1, sizeof(win) - 2u); wl--; }
        win[wl++] = c;
        win[wl] = '\0';
        if (wl >= 5 && memcmp(&win[wl - 5], "+IPD,", 5) == 0) {
            st = S_LEN;
            ipd_len = 0;
        } else if (wl >= 6 && memcmp(&win[wl - 6], "CLOSED", 6) == 0) {
            *closed = 1;
            return rl;
        }
    }
    return rl;
}

static int parse_http(const char *resp, int *status, char *body, int bodymax)
{
    const char *p = strstr(resp, "HTTP/1.");
    const char *b;
    int n;

    if (!p) return ESP_ERR_RESPONSE;
    p = strchr(p, ' ');
    if (!p) return ESP_ERR_RESPONSE;
    *status = atoi(p + 1);

    b = strstr(resp, "\r\n\r\n");
    if (b) {
        b += 4;
        n = (int)strlen(b);
        if (n >= bodymax) n = bodymax - 1;
        memcpy(body, b, (size_t)n);
        body[n] = '\0';
    }
    return (*status > 0) ? ESP_OK : ESP_ERR_RESPONSE;
}

int esp_http_post(const char *path, const char *json,
                  int *status, char *body, int bodymax)
{
    static const char *const tok_conn[] = { "OK\r\n", "ERROR", "ALREADY" };
    static const char *const tok_send[] = { ">", "ERROR", "link is not" };
    static const char *const tok_sent[] = { "SEND OK", "SEND FAIL", "ERROR" };
    static const char *const tok_close[] = { "OK\r\n", "ERROR" };
    static char req[512];
    static char resp[RESP_MAX];
    char cmd[64];
    int r, len, closed = 0, attempt;

    *status = 0;
    if (body && bodymax > 0) body[0] = '\0';

    for (attempt = 0; attempt < 2; attempt++) {
        snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d", SERVER_IP, SERVER_PORT);
        send_line(cmd);
        r = wait_tokens(tok_conn, 3, 6000u, 0, 0);
        if (r == 0) break;
        if (r == 2) {                               /* stale connection: close, retry */
            (void)wait_tokens(tok_close, 2, 500u, 0, 0);
            send_line("AT+CIPCLOSE");
            (void)wait_tokens(tok_close, 2, 1000u, 0, 0);
            continue;
        }
        return ESP_ERR_CONNECT;
    }
    if (r != 0) return ESP_ERR_CONNECT;

    len = snprintf(req, sizeof(req),
                   "POST %s HTTP/1.1\r\n"
                   "Host: %s:%d\r\n"
                   "Content-Type: application/json\r\n"
                   "Content-Length: %u\r\n"
                   "Connection: close\r\n"
                   "\r\n"
                   "%s",
                   path, SERVER_IP, SERVER_PORT, (unsigned)strlen(json), json);
    if (len <= 0 || len >= (int)sizeof(req)) return ESP_ERR_SEND;

    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d", len);
    send_line(cmd);
    if (wait_tokens(tok_send, 3, 3000u, 0, 0) != 0) {
        send_line("AT+CIPCLOSE");
        (void)wait_tokens(tok_close, 2, 1000u, 0, 0);
        return ESP_ERR_SEND;
    }

    uart_write(req, (uint32_t)len);              /* no flush: keep what follows */
    if (wait_tokens(tok_sent, 3, 5000u, 0, 0) != 0) {
        send_line("AT+CIPCLOSE");
        (void)wait_tokens(tok_close, 2, 1000u, 0, 0);
        return ESP_ERR_SEND;
    }

    (void)read_ipd_response(resp, (int)sizeof(resp), 6000u, &closed);
    if (!closed) {
        send_line("AT+CIPCLOSE");
        (void)wait_tokens(tok_close, 2, 1000u, 0, 0);
    }
    return parse_http(resp, status, body, bodymax);
}

/* ------------------------------------------------------------------ */
/* tiny JSON helpers (flat objects only)                                */
/* ------------------------------------------------------------------ */

static const char *json_find(const char *json, const char *key)
{
    char pat[40];
    const char *p;
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    p = strstr(json, pat);
    if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != ':') return 0;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

int json_get_int(const char *json, const char *key, int32_t *out)
{
    const char *p = json_find(json, key);
    if (!p || !(*p == '-' || (*p >= '0' && *p <= '9'))) return 0;
    *out = (int32_t)strtol(p, 0, 10);
    return 1;
}

int json_get_str(const char *json, const char *key, char *out, int outmax)
{
    const char *p = json_find(json, key);
    int n = 0;
    if (!p || *p != '"') return 0;
    p++;
    while (*p && *p != '"' && n < outmax - 1) {
        if (*p == '\\' && p[1]) p++;
        out[n++] = *p++;
    }
    out[n] = '\0';
    return 1;
}

/* ======================================================================== */
/* app.c */
/* ======================================================================== */
/*
 * app.c - Smart Student ID application
 *
 *  Boot     : status messages, Wi-Fi join
 *  Loop     : SOS (EXTI flag), Mode button, RFID taps, heart rate,
 *             library noise, restricted-area entry, temperature,
 *             periodic uploads and Wi-Fi retry
 */
/* ------------------------------------------------------------------ */
/* Keil Watch variables                                                 */
/* ------------------------------------------------------------------ */
volatile char    g_last_uid[9]   = "";
volatile int32_t g_last_bpm      = 0;
volatile int32_t g_sound_level   = 0;
volatile int32_t g_distance_cm   = 0;
volatile float   g_temperature_c = 0.0f;

/* ------------------------------------------------------------------ */
typedef enum { MODE_ATTENDANCE = 0, MODE_MESS = 1 } app_mode_t;

static app_mode_t s_mode = MODE_ATTENDANCE;
static int  s_rc522_ok = 0;
static int  s_max_ok = 0;
static int  s_esp_present = 0;
static int  s_wifi_ok = 0;
static char s_ip[20] = "";

static uint32_t s_msg_until = 0;       /* LCD message hold time */
static char     s_prev_uid[9] = "";
static uint32_t s_prev_uid_ms = 0;
static int32_t  s_temp_c10 = 0;
static int      s_temp_valid = 0;

/* ------------------------------------------------------------------ */
static void show_msg(const char *l1, const char *l2, uint32_t hold_ms)
{
    lcd_show(l1, l2);
    s_msg_until = millis() + hold_ms;
}

static int msg_active(void)
{
    return (int32_t)(s_msg_until - millis()) > 0;
}

static void boot_msg(const char *l1, const char *l2)
{
    lcd_show(l1, l2);
    delay_ms(BOOT_MSG_MS);
}

static void set_last_uid(const char *uid)
{
    int i;
    for (i = 0; i < 8 && uid[i]; i++) g_last_uid[i] = uid[i];
    g_last_uid[i] = '\0';
}

static void get_last_uid(char out[9])
{
    int i;
    for (i = 0; i < 8 && g_last_uid[i]; i++) out[i] = g_last_uid[i];
    out[i] = '\0';
}

static void c10_to_str(int32_t c10, char *out, int n)
{
    int32_t a = (c10 < 0) ? -c10 : c10;
    snprintf(out, (size_t)n, "%s%ld.%ld", (c10 < 0) ? "-" : "", (long)(a / 10), (long)(a % 10));
}

/* ------------------------------------------------------------------ */
/* Wi-Fi                                                                */
/* ------------------------------------------------------------------ */
static void wifi_connect(int show_on_lcd)
{
    if (show_on_lcd) lcd_show("Connecting WiFi", "");

    if (!s_esp_present) s_esp_present = (esp_init() == ESP_OK);
    if (s_esp_present && esp_join(WIFI_SSID, WIFI_PASS, s_ip, (int)sizeof(s_ip)) == ESP_OK) {
        s_wifi_ok = 1;
    } else {
        s_wifi_ok = 0;
        s_ip[0] = '\0';
    }

    if (show_on_lcd) {
        if (s_wifi_ok) boot_msg("WiFi connected", s_ip);
        else           boot_msg("WiFi unavailable", s_esp_present ? "Check SSID/pass" : "ESP not found");
    }
}

/* POST and return the HTTP status, or -1 if Wi-Fi/server is unreachable */
static int post_json(const char *path, const char *json, char *body, int bodymax)
{
    int status = 0;
    int r;

    if (!s_wifi_ok) return -1;
    r = esp_http_post(path, json, &status, body, bodymax);
    if (r != ESP_OK) {
        if (!esp_link_up()) s_wifi_ok = 0;    /* retried every WIFI_RETRY_PERIOD_MS */
        return -1;
    }
    return status;
}

/* ------------------------------------------------------------------ */
/* Card tap                                                             */
/* ------------------------------------------------------------------ */
static void handle_card(const char *uid)
{
    char json[160], body[256], ts[20], l2[17], name[17];
    int32_t bal = 0;
    int status;

    set_last_uid(uid);
    buzzer_beep(1u, 80u, 0u);
    lcd_show(uid, "Please wait...");
    ds3231_timestamp(ts);

    if (s_mode == MODE_ATTENDANCE) {
        snprintf(json, sizeof(json), "{\"uid\":\"%s\",\"timestamp\":\"%s\"}", uid, ts);
        status = post_json("/api/attendance", json, body, (int)sizeof(body));
    } else {
        snprintf(json, sizeof(json), "{\"uid\":\"%s\",\"amount\":%d,\"timestamp\":\"%s\"}",
                 uid, MEAL_COST, ts);
        status = post_json("/api/mess", json, body, (int)sizeof(body));
    }

    name[0] = '\0';
    (void)json_get_str(body, "name", name, (int)sizeof(name));

    if (status < 0) {
        show_msg("WiFi/server off", uid, RESULT_MSG_MS);
        buzzer_beep(2u, 60u, 60u);
    } else if (status == 404) {
        show_msg("Enroll on server", uid, RESULT_MSG_MS + 1500u);
        buzzer_beep(2u, 60u, 60u);
    } else if (status == 402) {
        if (json_get_int(body, "balance", &bal)) snprintf(l2, sizeof(l2), "Bal: Rs %ld", (long)bal);
        else strcpy(l2, uid);
        show_msg("Insufficient", l2, RESULT_MSG_MS);
        buzzer_beep(2u, 60u, 60u);
    } else if (status == 200 && s_mode == MODE_ATTENDANCE) {
        show_msg("ATTENDANCE OK", name[0] ? name : uid, RESULT_MSG_MS);
    } else if (status == 200) {
        if (json_get_int(body, "balance", &bal)) snprintf(l2, sizeof(l2), "Bal: Rs %ld", (long)bal);
        else strcpy(l2, "Bal: Rs ?");
        show_msg("Meal payment OK", l2, RESULT_MSG_MS);
    } else {
        snprintf(l2, sizeof(l2), "HTTP %d", status);
        show_msg("Server error", l2, RESULT_MSG_MS);
    }
}

/* ------------------------------------------------------------------ */
/* Alerts                                                               */
/* ------------------------------------------------------------------ */
static void handle_sos(void)
{
    char json[96], body[64], ts[20], uid[9];
    int status;

    buzzer_beep(3u, 200u, 150u);
    lcd_show("!!!SOS ALERT!!!", "Sending alert...");
    ds3231_timestamp(ts);
    get_last_uid(uid);
    snprintf(json, sizeof(json), "{\"uid\":\"%s\",\"timestamp\":\"%s\"}", uid, ts);
    status = post_json("/api/sos", json, body, (int)sizeof(body));
    show_msg("!!!SOS ALERT!!!", (status == 200) ? "Alert sent" : "Local alert only", ALERT_MSG_MS + 1000u);
}

static void handle_noise(int level)
{
    char json[96], body[64], ts[20];

    buzzer_beep(2u, 150u, 120u);
    show_msg("LIBRARY NOISE!", "PLEASE BE QUIET", ALERT_MSG_MS);
    ds3231_timestamp(ts);
    snprintf(json, sizeof(json), "{\"level\":%d,\"timestamp\":\"%s\"}", level, ts);
    (void)post_json("/api/noise", json, body, (int)sizeof(body));
}

static void handle_entry(void)
{
    buzzer_beep(3u, 150u, 100u);
    show_msg("RESTRICTED AREA", "ENTRY DETECTED!", ALERT_MSG_MS);
}

/* ------------------------------------------------------------------ */
/* Idle screen                                                          */
/* ------------------------------------------------------------------ */
static void idle_screen(void)
{
    char l1[17], l2[17], hm[6] = "--:--";
    rtc_time_t t;

    if (hr_finger_present()) {
        int bpm = hr_bpm();
        if (bpm > 0) snprintf(l2, sizeof(l2), "HR: %d BPM", bpm);
        else         strcpy(l2, "HR: -- BPM");
        lcd_show("HEART RATE", l2);
        return;
    }

    if (ds3231_read(&t) == 0) snprintf(hm, sizeof(hm), "%02u:%02u", (unsigned)t.hour, (unsigned)t.minute);

    if (s_mode == MODE_ATTENDANCE) {
        strcpy(l1, "ATTENDANCE MODE");
        snprintf(l2, sizeof(l2), "%-11s%s", s_wifi_ok ? "Tap card" : "No WiFi", hm);
    } else {
        strcpy(l1, "MESS PAYMENT");
        snprintf(l2, sizeof(l2), "%-11s%s", s_wifi_ok ? "Meal Rs50" : "No WiFi", hm);
    }
    lcd_show(l1, l2);
}

/* ------------------------------------------------------------------ */
/* Heart rate: drain the MAX30102 FIFO into the beat detector           */
/* ------------------------------------------------------------------ */
static void heart_rate_service(void)
{
    uint32_t red[32], ir[32];
    int n, i, ovf = 0;

    if (!s_max_ok) return;
    n = max30102_read_fifo(red, ir, 32, &ovf);
    if (ovf) hr_mark_gap();                  /* loop was blocked (e.g. Wi-Fi) */
    for (i = 0; i < n; i++) (void)hr_add_sample(ir[i]);
    g_last_bpm = hr_bpm();
}

/* ------------------------------------------------------------------ */
void app_run(void)
{
    uint32_t now, boot_ms;
    uint32_t t_rfid = 0, t_noise = 0, t_entry = 0, t_temp = 0, t_led = 0;
    uint32_t t_health = 0, t_temp_post = 0, t_wifi = 0;
    uint32_t noise_block_until = 0, entry_block_until = 0;
    int entry_near_count = 0, entry_armed = 1;

    boot_ms = millis();
    i2c_init();
    (void)lcd_init();
    buzzer_init();
    buttons_init();
    sensors_init();
    hr_reset();

    boot_msg("SMART STUDENT ID", "Starting...");
#if SINGLE_BUTTON
    boot_msg("Button: 1x MODE", "        3x SOS");
#endif

    s_rc522_ok = (rc522_init() == 0);
    if (!s_rc522_ok) boot_msg("RC522 not found", "Check SPI wiring");

    s_max_ok = (max30102_init() == 0);
    if (!s_max_ok) boot_msg("MAX30102 missing", "HR disabled");

    if (ds3231_init() != 0) boot_msg("DS3231 missing", "Time = 2000-01-01");

    while ((millis() - boot_ms) < ESP_BOOT_WAIT_MS) { }   /* NodeMCU boot time */
    wifi_connect(1);

    now = millis();
    t_temp_post = now - TEMP_POST_PERIOD_MS + 5000u;       /* first upload ~5 s after boot */
    t_wifi = now;
    s_msg_until = now;

    for (;;) {
        now = millis();

        /* ---- SOS (set by EXTI9 interrupt) ---- */
        if (g_sos_flag) {
            g_sos_flag = 0u;
            handle_sos();
            continue;
        }

        /* ---- Mode button ---- */
        if (button_mode_event()) {
            s_mode = (s_mode == MODE_ATTENDANCE) ? MODE_MESS : MODE_ATTENDANCE;
            buzzer_beep(1u, 40u, 0u);
            show_msg("MODE CHANGED", (s_mode == MODE_ATTENDANCE) ? "ATTENDANCE" : "MESS PAYMENT", 1500u);
        }

        /* ---- RFID ---- */
        if (s_rc522_ok && (now - t_rfid) >= RFID_POLL_MS) {
            uint8_t raw[4];
            char uid[9];
            t_rfid = now;
            if (rc522_read_uid(raw)) {
                rc522_uid_to_hex(raw, uid);
                if (!(strcmp(uid, s_prev_uid) == 0 && (now - s_prev_uid_ms) < CARD_REPEAT_BLOCK_MS)) {
                    strcpy(s_prev_uid, uid);
                    handle_card(uid);
                }
                s_prev_uid_ms = millis();
            }
        }

        /* ---- Heart rate ---- */
        heart_rate_service();

        /* ---- Library noise ---- */
        if ((now - t_noise) >= NOISE_POLL_MS) {
            int p2p;
            t_noise = now;
            p2p = sound_p2p(NOISE_WINDOW_MS, 0);
            g_sound_level = p2p;
            if (p2p > NOISE_THRESHOLD_P2P && (int32_t)(now - noise_block_until) >= 0) {
                noise_block_until = now + NOISE_COOLDOWN_MS;
                handle_noise(p2p);
            }
        }

        /* ---- Restricted-area entry (needs 2 close readings in a row) ---- */
        if ((now - t_entry) >= ENTRY_POLL_MS) {
            int cm;
            t_entry = now;
            cm = ultrasonic_cm();
            g_distance_cm = cm;
            if (cm > 0 && cm < ENTRY_DISTANCE_CM) {
                if (++entry_near_count >= 2 && entry_armed &&
                    (int32_t)(now - entry_block_until) >= 0) {
                    entry_armed = 0;
                    entry_block_until = now + ENTRY_COOLDOWN_MS;
                    handle_entry();
                }
            } else {
                entry_near_count = 0;
                if (cm == US_OUT_OF_RANGE || cm >= ENTRY_DISTANCE_CM + 10) entry_armed = 1;
            }
        }

        /* ---- Temperature ---- */
        if ((now - t_temp) >= TEMP_READ_PERIOD_MS) {
            int c10;
            t_temp = now;
            c10 = lm35_read_c10();
            s_temp_valid = (c10 > -100 && c10 < 1000);
            if (s_temp_valid) {
                s_temp_c10 = c10;
                g_temperature_c = (float)c10 / 10.0f;
            }
        }
        if ((now - t_temp_post) >= TEMP_POST_PERIOD_MS) {
            t_temp_post = now;
            if (s_temp_valid) {
                char json[96], body[64], ts[20], cs[12];
                ds3231_timestamp(ts);
                c10_to_str(s_temp_c10, cs, (int)sizeof(cs));
                snprintf(json, sizeof(json), "{\"celsius\":%s,\"timestamp\":\"%s\"}", cs, ts);
                (void)post_json("/api/temperature", json, body, (int)sizeof(body));
            }
        }

        /* ---- Health upload every 10 s while BPM is valid ---- */
        if (g_last_bpm > 0 && (now - t_health) >= HEALTH_POST_PERIOD_MS) {
            char json[112], body[64], ts[20], uid[9];
            t_health = now;
            ds3231_timestamp(ts);
            get_last_uid(uid);
            snprintf(json, sizeof(json), "{\"uid\":\"%s\",\"bpm\":%ld,\"timestamp\":\"%s\"}",
                     uid, (long)g_last_bpm, ts);
            (void)post_json("/api/health", json, body, (int)sizeof(body));
        }

        /* ---- Wi-Fi retry every 60 s ---- */
        if (!s_wifi_ok && (now - t_wifi) >= WIFI_RETRY_PERIOD_MS) {
            t_wifi = now;
            wifi_connect(!msg_active());
            hr_mark_gap();
        }
        if (s_wifi_ok) t_wifi = now;

        /* ---- LCD + heartbeat LED ---- */
        if (!msg_active()) idle_screen();
        if ((now - t_led) >= 500u) { t_led = now; led_toggle(); }
    }
}

/* ======================================================================== */
/* component_tests.c */
/* ======================================================================== */
/*
 * component_tests.c - bring-up tests, one component at a time
 *
 *  1 I2C scan        2 buzzer       3 buttons     4 RC522      5 DS3231
 *  6 MAX30102        7 KY-037       8 HC-SR04     9 ESP8266    10 LM35
 *  11 pin toggle (checks every header pin used by the project)
 *
 * Every test shows its result on the LCD (when present) and updates
 * g_test_alive / g_test_ok / g_test_value / g_test_text for the Watch window.
 */
volatile uint32_t g_test_alive = 0;
volatile int32_t  g_test_ok    = 0;
volatile int32_t  g_test_value = 0;
volatile char     g_test_text[32] = "";

static void set_text(const char *s)
{
    int i;
    for (i = 0; i < 31 && s[i]; i++) g_test_text[i] = s[i];
    g_test_text[i] = '\0';
}

static void tick(void)
{
    g_test_alive++;
    led_toggle();
}

/* ---------------- 1: I2C scan ----------------
 * Before each scan the two bus pins are read as inputs with the internal
 * pull-DOWN on. A module's pull-up resistors pull a connected line to 1;
 * a line that reads 0 is not reaching any module (loose wire, wrong pin,
 * module unpowered).                                                     */
volatile int32_t g_i2c_scl = -1;     /* 1 = SCL wire reaches a module    */
volatile int32_t g_i2c_sda = -1;     /* 1 = SDA wire reaches a module    */
volatile int32_t g_i2c_err = 0;      /* probe of 0x27: -1 NACK, -2 stuck */

static void i2c_line_check(void)
{
    RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;
    gpio_pull(GPIOB, 6u, GPIO_PULL_DOWN);
    gpio_pull(GPIOB, 7u, GPIO_PULL_DOWN);
    gpio_mode(GPIOB, 6u, GPIO_MODE_INPUT);
    gpio_mode(GPIOB, 7u, GPIO_MODE_INPUT);
    delay_ms(5);
    g_i2c_scl = gpio_read(GPIOB, 6u);
    g_i2c_sda = gpio_read(GPIOB, 7u);
    i2c_init();                                    /* back to I2C mode */
    g_i2c_err = i2c_write(I2C_ADDR_LCD_A, 0, 0u);
}

static void test_i2c_scan(void)
{
    char list[32], l2[17];
    uint8_t a;
    int count, pos, lcd_found, eep, max, rtc;

    for (;;) {
        i2c_line_check();
        count = 0; pos = 0; list[0] = '\0';
        lcd_found = eep = max = rtc = 0;
        for (a = 0x08u; a < 0x78u; a++) {
            if (i2c_probe(a)) {
                count++;
                if (pos < (int)sizeof(list) - 4) pos += snprintf(&list[pos], sizeof(list) - (size_t)pos, "%02X ", a);
                if (a == I2C_ADDR_LCD_A || a == I2C_ADDR_LCD_B) lcd_found = 1;
                if (a == I2C_ADDR_EEPROM)   eep = 1;
                if (a == I2C_ADDR_MAX30102) max = 1;
                if (a == I2C_ADDR_DS3231)   rtc = 1;
            }
        }
        g_test_value = count;
        g_test_ok = (lcd_found && eep && max && rtc) ? 1 : 0;
        if (count == 0) {
            snprintf(list, sizeof(list), "none SCL=%d SDA=%d err=%d",
                     (int)g_i2c_scl, (int)g_i2c_sda, (int)g_i2c_err);
        }
        set_text(list);
        snprintf(l2, sizeof(l2), "%d dev %s", count, g_test_ok ? "ALL OK" : "missing");
        lcd_show(list, l2);
        tick();
        delay_ms(2000);
    }
}

/* ---------------- 2: buzzer ---------------- */
static void test_buzzer(void)
{
    char l2[17];
    buzzer_init();
    g_test_ok = 1;
    for (;;) {
        g_test_value++;
        snprintf(l2, sizeof(l2), "Beep #%ld", (long)g_test_value);
        lcd_show("TEST 2 BUZZER", l2);
        set_text(l2);
        buzzer_beep(1u, 200u, 0u);
        tick();
        delay_ms(1000);
    }
}

/* ---------------- 3: buttons ---------------- */
static void test_buttons(void)
{
    char l1[17], l2[17];
    uint32_t mode_count = 0, sos_count = 0;
    buttons_init();
    for (;;) {
        if (button_mode_event()) mode_count++;
        if (g_sos_flag) { g_sos_flag = 0u; sos_count++; buzzer_beep(3u, 150u, 100u); }
        g_test_value = (int32_t)g_btn_last_clicks;
        g_test_ok = (mode_count > 0u && sos_count > 0u) ? 1 : 0;
#if SINGLE_BUTTON
        snprintf(l1, sizeof(l1), "BTN:%d last:%lux", button_sos_down(), (unsigned long)g_btn_last_clicks);
#else
        snprintf(l1, sizeof(l1), "MODE:%d SOS:%d", button_mode_down(), button_sos_down());
#endif
        snprintf(l2, sizeof(l2), "MODE#%lu SOS#%lu", (unsigned long)mode_count, (unsigned long)sos_count);
        set_text(l1);
        lcd_show(l1, l2);
        tick();
        delay_ms(20);
    }
}

/* ---------------- 4: RC522 ---------------- */
static void test_rc522(void)
{
    char l1[17], l2[17] = "Tap a card";
    uint8_t raw[4];
    char uid[9];
    int ok = (rc522_init() == 0);
    uint8_t v = rc522_version();

    g_test_value = v;
    g_test_ok = ok;
    /* line 1: version + SPI read-back check (rb must be 26) */
    snprintf(l1, sizeof(l1), "v:%02X rb:%02X %s", v, (unsigned)g_rc522_readback, ok ? "OK" : "BAD");
    set_text(ok ? "SPI ok, tap card" : "SPI read-back failed");
    for (;;) {
        if (ok && rc522_read_uid(raw)) {
            rc522_uid_to_hex(raw, uid);
            snprintf(l2, sizeof(l2), "UID %s", uid);
            set_text(uid);
            buzzer_beep(1u, 60u, 0u);
        } else if (!ok) {
            strcpy(l2, "Check MISO/SPI");
        } else if (l2[0] != 'U') {
            /* still waiting: show poll count and how many polls saw a card */
            snprintf(l2, sizeof(l2), "Tap P%lu C%lu", (unsigned long)(g_rc522_polls % 10000u),
                     (unsigned long)g_rc522_atqa);
        }
        lcd_show(l1, l2);
        tick();
        delay_ms(100);
    }
}

/* ---------------- 5: DS3231 ---------------- */
static void test_ds3231(void)
{
    rtc_time_t t;
    char ts[20], l1[17], l2[17];
    (void)ds3231_init();
    for (;;) {
        if (ds3231_read(&t) == 0) {
            ds3231_format(&t, ts);
            memcpy(l1, ts, 10); l1[10] = '\0';
            snprintf(l2, sizeof(l2), "%s %s", &ts[11], ds3231_lost_power() ? "OSF" : "");
            g_test_ok = (t.year >= 2024u) ? 1 : 0;
            g_test_value = t.second;
            set_text(ts);
            lcd_show(l1, l2);
        } else {
            g_test_ok = 0;
            set_text("DS3231 no reply");
            lcd_show("DS3231", "no reply @0x68");
        }
        tick();
        delay_ms(500);
    }
}

/* ---------------- 6: MAX30102 ---------------- */
static void test_max30102(void)
{
    uint32_t red[32], ir[32];
    char l1[17], l2[17];
    uint32_t t_show = 0;
    int ok = (max30102_init() == 0);
    int n, i, ovf;

    hr_reset();
    g_test_ok = ok;
    for (;;) {
        if (!ok) {
            snprintf(l2, sizeof(l2), "ID 0x%02X != 0x15", max30102_part_id());
            lcd_show("MAX30102 missing", l2);
            set_text("no reply @0x57");
            tick();
            delay_ms(500);
            continue;
        }
        n = max30102_read_fifo(red, ir, 32, &ovf);
        if (ovf) hr_mark_gap();
        for (i = 0; i < n; i++) (void)hr_add_sample(ir[i]);
        if (n > 0) g_test_value = (int32_t)ir[n - 1];
        g_last_bpm = hr_bpm();

        if ((millis() - t_show) >= 300u) {
            t_show = millis();
            snprintf(l1, sizeof(l1), "IR:%ld", (long)g_test_value);
            if (!hr_finger_present())   strcpy(l2, "Place finger");
            else if (g_last_bpm > 0)    snprintf(l2, sizeof(l2), "BPM: %ld", (long)g_last_bpm);
            else                        strcpy(l2, "BPM: measuring");
            set_text(l2);
            lcd_show(l1, l2);
            tick();
        }
        delay_ms(10);
    }
}

/* ---------------- 7: KY-037 ---------------- */
static void test_sound(void)
{
    char l1[17], l2[17];
    uint16_t raw = 0;
    sensors_init();
    for (;;) {
        int p2p = sound_p2p(NOISE_WINDOW_MS, &raw);
        g_sound_level = p2p;
        g_test_value = p2p;
        g_test_ok = (raw > 0u && raw < 4095u) ? 1 : 0;
        snprintf(l1, sizeof(l1), "P2P:%d %s", p2p, (p2p > NOISE_THRESHOLD_P2P) ? "LOUD" : "");
        snprintf(l2, sizeof(l2), "RAW:%u TH:%d", (unsigned)raw, NOISE_THRESHOLD_P2P);
        set_text(l1);
        lcd_show(l1, l2);
        tick();
        delay_ms(150);
    }
}

/* ---------------- 8: HC-SR04 ---------------- */
static void test_ultrasonic(void)
{
    char l2[17];
    sensors_init();
    for (;;) {
        int cm = ultrasonic_cm();
        g_distance_cm = cm;
        g_test_value = cm;
        g_test_ok = (cm != US_NO_SENSOR) ? 1 : 0;
        if (cm == US_NO_SENSOR)          strcpy(l2, "No echo: wiring");
        else if (cm == US_OUT_OF_RANGE)  strcpy(l2, "Out of range");
        else snprintf(l2, sizeof(l2), "%d cm %s", cm, (cm < ENTRY_DISTANCE_CM) ? "ENTRY" : "");
        set_text(l2);
        lcd_show("HC-SR04 DIST", l2);
        tick();
        delay_ms(200);
    }
}

/* ---------------- 9: ESP8266 ---------------- */
static void test_esp8266(void)
{
    char ver[40], l1[17], l2[17];
    delay_ms(ESP_BOOT_WAIT_MS);
    for (;;) {
        int ok = (esp_init() == ESP_OK);
        g_test_ok = ok;
        if (ok && esp_version(ver, (int)sizeof(ver)) == ESP_OK) {
            strcpy(l1, "ESP AT: OK");
            memcpy(l2, ver, 16); l2[16] = '\0';
            set_text(ver);
            g_test_value = 1;
        } else {
            strcpy(l1, ok ? "ESP AT: OK" : "ESP: no reply");
            strcpy(l2, ok ? "GMR failed" : "Check TX/RX/baud");
            set_text(l2);
            g_test_value = 0;
        }
        lcd_show(l1, l2);
        tick();
        delay_ms(3000);
    }
}

/* ---------------- 10: LM35 ---------------- */
static void test_lm35(void)
{
    char l1[17], l2[17];
    sensors_init();
    for (;;) {
        int c10 = lm35_read_c10();
        uint16_t raw = adc_read(ADC_CH_LM35);
        g_test_value = c10;
        g_temperature_c = (float)c10 / 10.0f;
        g_test_ok = (c10 > 50 && c10 < 600) ? 1 : 0;      /* 5..60 C plausible */
        snprintf(l1, sizeof(l1), "LM35 %d.%d C", c10 / 10, (c10 < 0 ? -c10 : c10) % 10);
        snprintf(l2, sizeof(l2), "ADC:%u %s", (unsigned)raw, g_test_ok ? "OK" : "check");
        set_text(l1);
        lcd_show(l1, l2);
        tick();
        delay_ms(500);
    }
}

/* ---------------- 11: pin toggle (header contact check) ----------------
 * Unplug every module first. All project pins become plain outputs and
 * switch 0 V / 3.3 V together every second, in step with the PC13 LED.
 * Probe each pin with a multimeter (black probe on G) or an LED + 330 ohm
 * to G: a pin that does not follow the LED is not making contact.        */
static void test_pin_toggle(void)
{
    static GPIO_TypeDef *const port[] = {
        GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA,
        GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB
    };
    static const uint8_t pin[] = {
        1, 2, 4, 5, 6, 7, 9, 10,          /* A1 A2 A4 A5 A6 A7 A9 A10       */
        0, 4, 6, 7, 8, 9, 12, 13          /* B0 B4 B6 B7 B8 B9 B12 B13      */
    };
    uint32_t i, n = sizeof(pin);
    int high = 0;

    RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;  /* release B6/B7 from I2C */
    for (i = 0; i < n; i++) {
        gpio_open_drain(port[i], pin[i], 0);
        gpio_pull(port[i], pin[i], GPIO_PULL_NONE);
        gpio_clr(port[i], pin[i]);
        gpio_mode(port[i], pin[i], GPIO_MODE_OUTPUT);
    }
    g_test_ok = 1;
    for (;;) {
        high = !high;
        for (i = 0; i < n; i++) {
            if (high) gpio_set(port[i], pin[i]);
            else      gpio_clr(port[i], pin[i]);
        }
        if (high) gpio_clr(LED_PORT, LED_PIN);    /* LED on (active low) */
        else      gpio_set(LED_PORT, LED_PIN);
        g_test_value = high ? 3300 : 0;           /* expected mV on every pin */
        set_text(high ? "pins HIGH 3.3V" : "pins LOW 0V");
        g_test_alive++;
        delay_ms(1000);
    }
}

/* ---------------- 12: RC522 deep debug ---------------- */
volatile uint8_t  g_dbg_spi_errors   = 0;     /* SPI pattern test: must be 0 */
volatile uint8_t  g_dbg_spi_badbits  = 0;     /* bits that read back wrong */
volatile int32_t  g_dbg_selftest     = 0;     /* 2 = v2.0, 1 = v1.0, 0 = clone, -1 = failed */
volatile uint8_t  g_dbg_selftest_bytes[64];
volatile uint8_t  g_dbg_regs[8];             /* Cmd TxCtl TxASK RFCfg Mode TMode Stat1 Stat2 */
volatile uint32_t g_dbg_polls        = 0;
volatile uint32_t g_dbg_clean        = 0;     /* card answered correctly */
volatile uint32_t g_dbg_garbled      = 0;     /* something answered but corrupted */
volatile uint32_t g_dbg_nothing      = 0;     /* no reply at all */
volatile uint32_t g_dbg_swtimeout    = 0;     /* RC522 did not finish the command */
volatile uint8_t  g_dbg_gain         = 0;     /* RFCfgReg in use */

static void dbg_hold(const char *l1, const char *l2, const char *watch)
{
    lcd_show(l1, l2);
    set_text(watch);
    tick();
    delay_ms(3500);
}

static void test_rc522_debug(void)
{
    static const uint8_t gains[] = { 0x70, 0x60, 0x50, 0x40 };   /* 48, 43, 38, 33 dB */
    static const uint8_t regs[]  = { 0x01, 0x14, 0x15, 0x26, 0x11, 0x2A, 0x07, 0x08 };
    char l1[17], l2[17];
    uint8_t st[64], raw[4], bad = 0, v;
    char uid[9];
    rc522_probe_t p;
    uint32_t i, t_cfg, gi = 0;
    int errors, init_ok, st_res;

    /* --- step 1: does the chip answer? --- */
    init_ok = (rc522_init() == 0);
    v = rc522_version();
    snprintf(l1, sizeof(l1), "1 VER:%02X RB:%02X", v, (unsigned)g_rc522_readback);
    snprintf(l2, sizeof(l2), "%s", init_ok ? "chip answers" : "NO/BAD REPLY");
    dbg_hold(l1, l2, l2);

    /* --- step 2: SPI integrity with 32 patterns --- */
    errors = rc522_spi_pattern_test(&bad);
    g_dbg_spi_errors = (uint8_t)errors;
    g_dbg_spi_badbits = bad;
    snprintf(l1, sizeof(l1), "2 SPI err:%d", errors);
    snprintf(l2, sizeof(l2), errors ? "bad bits:%02X" : "SPI OK", bad);
    dbg_hold(l1, l2, errors ? "SPI errors" : "SPI OK");

    /* --- step 3: chip self test --- */
    st_res = rc522_self_test(st);
    g_dbg_selftest = st_res;
    for (i = 0; i < 64u; i++) g_dbg_selftest_bytes[i] = st[i];
    snprintf(l1, sizeof(l1), "3 ST %02X%02X%02X%02X", st[0], st[1], st[2], st[3]);
    snprintf(l2, sizeof(l2), "%s",
             st_res == 2 ? "genuine v2.0" : st_res == 1 ? "genuine v1.0" :
             st_res == 0 ? "clone chip" : "self test FAIL");
    dbg_hold(l1, l2, l2);

    /* --- step 4: register dump (after re-init) --- */
    for (i = 0; i < 8u; i++) g_dbg_regs[i] = rc522_reg_read(regs[i]);
    snprintf(l1, sizeof(l1), "4 TX:%02X ASK:%02X", g_dbg_regs[1], g_dbg_regs[2]);
    snprintf(l2, sizeof(l2), "RF:%02X S2:%02X", g_dbg_regs[3], g_dbg_regs[7]);
    dbg_hold(l1, l2, ((g_dbg_regs[1] & 3u) == 3u) ? "antenna ON" : "ANTENNA OFF");

    /* --- step 5: live polling, REQA and WUPA alternately, gain changes every 5 s --- */
    rc522_reg_write(0x26, gains[0]);
    g_dbg_gain = gains[0];
    t_cfg = millis();
    for (;;) {
        if ((millis() - t_cfg) >= 5000u) {
            t_cfg = millis();
            gi = (gi + 1u) % sizeof(gains);
            rc522_reg_write(0x26, gains[gi]);
            g_dbg_gain = gains[gi];
        }

        rc522_probe((int)(g_dbg_polls & 1u), &p);
        g_dbg_polls++;
        if (p.result == 0)       g_dbg_clean++;
        else if (p.result == 2)  g_dbg_garbled++;
        else if (p.result == 1)  g_dbg_nothing++;
        else                     g_dbg_swtimeout++;

        /* a clean answer: try to read the UID too */
        if (p.result == 0) {
            /* the probe left the card in READY state: switch the field off
             * briefly so it returns to IDLE, then do the normal UID read */
            v = rc522_reg_read(0x14);
            rc522_reg_write(0x14, (uint8_t)(v & 0xFCu));
            delay_ms(5);
            rc522_reg_write(0x14, (uint8_t)(v | 0x03u));
            delay_ms(5);
        }
        if (p.result == 0 && rc522_read_uid(raw)) {
            rc522_uid_to_hex(raw, uid);
            snprintf(l2, sizeof(l2), "UID %s", uid);
            set_text(uid);
            buzzer_beep(1u, 60u, 0u);
            lcd_show("5 CARD READ OK", l2);
            delay_ms(1500);
        }

        snprintf(l1, sizeof(l1), "C%lu G%lu N%lu",
                 (unsigned long)(g_dbg_clean % 1000u),
                 (unsigned long)(g_dbg_garbled % 1000u),
                 (unsigned long)(g_dbg_nothing % 1000u));
        snprintf(l2, sizeof(l2), "I%02X E%02X F%u g%02X", p.irq, p.err, (unsigned)p.fifo, g_dbg_gain);
        lcd_show(l1, l2);
        g_test_value = (int32_t)g_dbg_clean;
        g_test_ok = (g_dbg_clean > 0u) ? 1 : 0;
        tick();
        delay_ms(100);
    }
}

void component_test_run(int test)
{
    char l2[17];

    i2c_init();
    (void)lcd_init();
    buzzer_init();
    snprintf(l2, sizeof(l2), "Component #%d", test);
    lcd_show("TEST MODE", l2);
    delay_ms(1000);

    switch (test) {
    case 1:  test_i2c_scan();    break;
    case 2:  test_buzzer();      break;
    case 3:  test_buttons();     break;
    case 4:  test_rc522();       break;
    case 5:  test_ds3231();      break;
    case 6:  test_max30102();    break;
    case 7:  test_sound();       break;
    case 8:  test_ultrasonic();  break;
    case 9:  test_esp8266();     break;
    case 10: test_lm35();        break;
    case 11: test_pin_toggle();  break;
    case 12: test_rc522_debug(); break;
    default: break;
    }
    lcd_show("TEST_COMPONENT", "invalid (0..12)");
    for (;;) { tick(); delay_ms(500); }
}

/* ======================================================================== */
/* main.c */
/* ======================================================================== */
/*
 * main - Smart Student ID
 *
 * TEST_COMPONENT in board.h selects the normal application (0) or a single
 * component bring-up test (1..10).
 */
int main(void)
{
    /* enable the FPU (also done by the pack SystemInit; harmless twice) */
    SCB_CPACR |= (0xFu << 20);
    __asm volatile ("dsb" ::: "memory");
    __asm volatile ("isb" ::: "memory");

    system_init();          /* 84 MHz, SysTick 1 ms, TIM2 1 us, LED */

#if TEST_COMPONENT == 0
    app_run();
#else
    component_test_run(TEST_COMPONENT);
#endif

    for (;;) { }
}

