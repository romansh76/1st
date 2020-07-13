/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "usbd_cdc_if.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
VAR v;
DMA_HandleTypeDef adc_dma;
osRtxMutex_t *mt;
osRtxThread_t *main_thread;
osRtxThread_t *adc_thread;

DECLARE_THREAD_ATTR(main,256,osPriorityNormal);
DECLARE_THREAD_ATTR(adc,256,osPriorityNormal);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Вычисление среднего значения входного массива.
  * @param  buf - входной массив
  * @param  cnt - размер массива
  * @retval average value
  */

U32 os_calc_average(U16 *buf,U32 cnt)
{
 U32 res,i;
 res=0;
 for(i=0;i<cnt;i++)
  res+=buf[i];
 return res/cnt;
}

/**
  * @brief  Функция осуществляет перевод числа в строку. Строка без завершающего "0" 
  * @param  val - входное значение
  * @param  str - выходная строка
  * @param  symbol_qty - максимальное число значащих символов(разрядов)
  * @retval длина выходной строки
  */

U32 os_int2str(U32 val,U8* str,U8 symbol_qty)
{
U32 sval,i;	
 if ((str==0)||(symbol_qty==0))
  return 0;
 if (val==0)//проверка значения на ноль
  {str[0]='0';return 1;}// результирующая строка -"0" длиной 1
 i=0;sval=val;//подсчет десятичных разрядов в исходном числе
 while(val!=0)
 {
  i++;
  val=val/10;
 }
 if (i>symbol_qty)//Количество разрядов превышает заданое?
  i=symbol_qty;//если превышает,то установить заданое
 symbol_qty=i; //установить возвращаемую длину
 while((sval!=0)&&(i!=0))//проверка числа на 0 и на нулевую позицию
 {
  str[i-1]='0'+sval%10;//запись разряда числа в позицию справа
  sval/=10;//отбрость разряд
  i--;//сдвинуть позицию символа на 1 влево
 }
 return symbol_qty;
}


U32 HAL_GetTick(void)
{
return osRtxInfo.kernel.tick;
//return 0;
//return os_core.dwt->CYCCNT/(SystemCoreClock/1000);
}


/**
  * @brief  Вывод буфера через ITM-порт .
  * @param  buf - входной массив
  * @param  cnt - размер массива
  * @retval none
  */

void os_keil_write(U8* buf,U32 cnt)
{
 U32 i;
 for(i=0;i<cnt;i++)
 {
  ITM_SendChar(buf[i]);
 }
}

/**
  * @brief  Инициализация DMA ADC
  * @param  dma - адрес управляющей структуры DMA_HandleTypeDef
  * @retval none
  */

void os_adc_dma_init(DMA_HandleTypeDef *dma)
{
    dma->Instance = DMA2_Stream0;
    dma->Init.Channel = DMA_CHANNEL_0;
    dma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma->Init.PeriphInc = DMA_PINC_DISABLE;
    dma->Init.MemInc = DMA_MINC_ENABLE;
    dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dma->Init.Mode = DMA_NORMAL;
    dma->Init.Priority = DMA_PRIORITY_LOW;
    dma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(dma);
}

/**
  * @brief  Функция обратного вызова,срабатывающая при возникновении события обновления счетчика таймера
  * @param  htim - адрес управляющей структуры TIM_HandleTypeDef
  * @retval none
  */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
 if (htim==&htim2)
 {
  v.dac_cnt=(v.dac_cnt+1)%(ADC_SAMPLE_RATE/DAC_SAMPLE_RATE);// Увеличение счетчика количества прерываний таймера по модулю ADC_SAMPLE_RATE/DAC_SAMPLE_RATE
  if (v.dac_cnt==0)
  {
   LL_DAC_ConvertData12LeftAligned
      (DAC1,
       LL_DAC_CHANNEL_1,
       os_calc_average(v.buf.dac[v.dac_position],ADC_SAMPLE_RATE/DAC_SAMPLE_RATE)
      );// Запись в ЦАП усредненного значения в 100 выборок
   v.dac_position=(v.dac_position+1)%DAC_SAMPLE_RATE;// Увеличение счетчика позиции во входном буфере
  }
 }
}

/**
  * @brief  Функция работы АЦП.
  * @retval none
  */
void os_adc_run()
{
 while(1)
 {
  HAL_DMA_Start(&adc_dma,OS_TYPE(U32,&ADC1->DR),OS_TYPE(U32,&v.buf),10000);//Установка источника и приемника данных для DMA. Установка размера передаваемых данных и разрешение работы DMA*/ 
  LL_ADC_REG_SetDMATransfer(ADC1,LL_ADC_REG_DMA_TRANSFER_LIMITED);//Включение режима чтения данных АЦП через DMA
  LL_ADC_Enable(ADC1);// Включение АЦП
  LL_ADC_ClearFlag_EOCS(ADC1);//Очистка флага окончание преобразования
  LL_ADC_ClearFlag_OVR(ADC1);//Очистка флага переполнения. Обеспечение отсутствия неизвестного состояния от предыдущих измерений
  
  HAL_TIM_Base_Start_IT(&htim2);//Старт таймера TIM2 с устанавливаемым флагом прерывания по обновлению счетчика
  if (HAL_DMA_PollForTransfer(&adc_dma,HAL_DMA_FULL_TRANSFER,1500)==HAL_TIMEOUT) // Опрос флага окончания передачи DMA
  {
   MX_ADC1_Init();//при таймауте - переинициализация АЦП и 
   continue;//прозвести измерения снова
  }
  else break;
 }  
 HAL_TIM_Base_Stop(&htim2);//останов TIM2   
 LL_ADC_Disable(ADC1);//Выключение АЦП
 CLEAR_BIT(ADC1->CR2,ADC_CR2_DMA);//Выключение режима DMA
 return;
}

/**
  * @brief  Функция основного потока
  * @retval none
  */
__NO_RETURN void main_threadf (void *argument) 
{
 U32 len;
  MX_GPIO_Init();//Инициализация GPIO
  MX_USB_DEVICE_Init();//Инициализация USB CDC
  mt=osMutexNew(NULL);//Создание мютекса для доступа к разделяемой переменной v.average
  osThreadFlagsSet(adc_thread,APP_THREAD_FLAGS_START);// Сигнал потоку adc_thread для начала работы
  while (1)
  {
   osThreadFlagsWait(APP_THREAD_FLAGS_END_MEASURE,osFlagsWaitAny,osWaitForever);// Ожидание сигнала окончания преобразования
   if (osMutexAcquire(mt,10)==osOK)//Блокировать мютекс в течении 10 мс
   {
    len=os_int2str(v.average,v.out,4);//Перевод полученного среднего значения в строку
    memcpy(v.out+len," mV\r\n",6);// Добавление к этой строке смыслового окончания
    len=strlen(OS_TYPE(const char*,v.out));//Подсчет общей длины строки
    os_keil_write(v.out,len);//Вывод в терминал Keil'a строки
    if (hUsbDeviceFS.dev_state==USBD_STATE_CONFIGURED)// Проверить состояние устройства USB CDC
     CDC_Transmit_FS(v.out,len);// Если сконфигурировано,то вывести в USB
    osMutexRelease(mt); // Освобождение мютекса
   }
   else __nop(); 
  }
}

/**
  * @brief  Функция потока, производящего измерение с АЦП
  * @retval none
  */
__NO_RETURN void adc_threadf (void *argument) 
{
  MX_DMA_Init();// Включить тактирование DMA
  MX_ADC1_Init();// Инициализация АЦП
  os_adc_dma_init(&adc_dma);//Инициализация DMA
  MX_DAC_Init();//Инициализация ЦАП
  MX_TIM2_Init();//Инициализация таймера TIM2
  osThreadFlagsWait(APP_THREAD_FLAGS_START,osFlagsWaitAny,osWaitForever);// Ожидать окончания инициализации периферийных устройств в главном потоке
  while (1)
  {
   os_adc_run();// Произвести измерения
   v.counter++;// Изменение счетчика "живучести"
   if (osMutexAcquire(mt,50)==osOK)//Блокировать мютекс в течении 50 мс
   {
    v.average=os_calc_average(v.buf.adc_sample,ADC_SAMPLE_RATE);//Подсчет среднего значения
    osMutexRelease(mt); //Освободить мютекс
    osThreadFlagsSet(main_thread,APP_THREAD_FLAGS_END_MEASURE);// Сигнализировать главному потоку об окончании цикла измерения
   } 
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
 __enable_irq();
	osKernelInitialize();                 // Инициализация CMSIS-RTOS
	main_thread=osThreadNew(main_threadf, NULL, &os_main_thread_attr);  // Создание главного потока
	adc_thread =osThreadNew(adc_threadf, NULL, &os_adc_thread_attr);    // Создание потока АЦП
	if (osKernelGetState() == osKernelReady)
	{
		osKernelStart();                    // Старт RTOS
	}
  /* USER CODE END SysInit */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
