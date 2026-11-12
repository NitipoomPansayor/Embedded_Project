#include "app_main.h"
#include <stdio.h>

/* Throwaway diagnostic test for today's sensor wiring: prints raw LDR + ultrasonic
   readings over USART2 every second. Not the real beam.c/ultrasonic.c modules. */

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart2;

#define PRINT_INTERVAL_MS  1000U
#define US_TRIG_PULSE_US   10U
#define US_ECHO_TIMEOUT_MS 30U
#define US_ECHO_GPIO_Port  GPIOB
#define US_ECHO_Pin        GPIO_PIN_4

static volatile uint32_t ic_rising_capture = 0;
static volatile uint32_t ic_pulse_width_us = 0;
static volatile uint8_t ic_echo_ready = 0;

static uint32_t last_print_tick = 0;

/* HAL_GetTick() only has 1ms resolution; the HC-SR04 trigger pulse needs ~10us,
   so use the DWT cycle counter for a finer busy-wait delay. */
static void dwt_delay_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000U);
  while ((DWT->CYCCNT - start) < ticks)
  {
  }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    uint32_t capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    if (HAL_GPIO_ReadPin(US_ECHO_GPIO_Port, US_ECHO_Pin) == GPIO_PIN_SET)
    {
      ic_rising_capture = capture;
    }
    else
    {
      /* Truncating a 32-bit subtraction of two 16-bit timer captures to uint16_t
         correctly handles counter wraparound between the rising and falling edge. */
      ic_pulse_width_us = (uint16_t)(capture - ic_rising_capture);
      ic_echo_ready = 1;
    }
  }
}

static uint16_t read_ldr_raw(void)
{
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
  {
    return (uint16_t)HAL_ADC_GetValue(&hadc1);
  }
  return 0;
}

static int32_t ping_ultrasonic_cm(void)
{
  uint32_t start = HAL_GetTick();

  ic_echo_ready = 0;
  HAL_GPIO_WritePin(US_TRIG_GPIO_Port, US_TRIG_Pin, GPIO_PIN_SET);
  delay_us(US_TRIG_PULSE_US);
  HAL_GPIO_WritePin(US_TRIG_GPIO_Port, US_TRIG_Pin, GPIO_PIN_RESET);

  while (!ic_echo_ready && (HAL_GetTick() - start) < US_ECHO_TIMEOUT_MS)
  {
  }

  if (!ic_echo_ready)
  {
    return -1;
  }
  return (int32_t)(ic_pulse_width_us / 58U);
}

void app_init(void)
{
  dwt_delay_init();
  HAL_GPIO_WritePin(LASER_EN_GPIO_Port, LASER_EN_Pin, GPIO_PIN_SET);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
  last_print_tick = HAL_GetTick();
}

void app_loop(void)
{
  if ((HAL_GetTick() - last_print_tick) < PRINT_INTERVAL_MS)
  {
    return;
  }
  last_print_tick = HAL_GetTick();

  uint16_t ldr_raw = read_ldr_raw();
  int32_t us_cm = ping_ultrasonic_cm();

  char line[48];
  int len;
  if (us_cm >= 0)
  {
    len = snprintf(line, sizeof(line), "LDR=%u US=%ldcm\r\n", ldr_raw, (long)us_cm);
  }
  else
  {
    len = snprintf(line, sizeof(line), "LDR=%u US=timeout\r\n", ldr_raw);
  }
  HAL_UART_Transmit(&huart2, (uint8_t *)line, (uint16_t)len, 100);
}
