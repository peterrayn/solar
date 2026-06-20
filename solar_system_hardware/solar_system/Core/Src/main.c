/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
# include "ST7735.h"
# include "GFX_FUNCTIONS.h"
# include "stdio.h"
# include "ina226.h"
# include "OneWire.h"
#include "bh1750.h"
#include "ESP8266_STM32.h"
#include "stdlib.h"

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
uint8_t opeCode;              
uint8_t g_ucaDataBuff[2];     
float ftmp;                   
extern uint32_t read_DATA[5];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern float Temp[MAXDEVICES_ON_THE_BUS];


extern uint32_t read_DATA[5];
char str_V[30],str_I[30],str_P[30],str_temper[40],str_Lux[30],str_data[50],str_spray[20],str_time[40];
float current,voltage,power;
uint16_t status0=0;



void sleep(){
	uint32_t timer=0;
//	printf("sleep\r\n");

    fillScreen(BLACK);
    ST7735_WriteString(10, 2, "SLEEPING...",Font_7x10, WHITE, BLACK);

	while(timer<3600){
	HAL_IWDG_Refresh(&hiwdg);
	HAL_Delay(1000);
	timer++;
	}
	status0 = 0;
    BKP->DR2 = status0;

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

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */

  

  INA226_init();
  ST7735_Init(0);

  fillScreen(BLACK);

  get_ROMid();

  BH1750_Init();
  opeCode= BH1750_CONT_HI_RSLT_1;
  BH1750_WriteOpecode(&opeCode,1);

  HAL_IWDG_Refresh(&hiwdg);


  
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();

    
    HAL_PWR_EnableBkUpAccess();
//


    
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
      {
     
          status0 = BKP->DR2;

  
          __HAL_RCC_CLEAR_RESET_FLAGS();
      }
      else
      {

      
    		status0 = 0;
    	    BKP->DR2 = status0;
          __HAL_RCC_CLEAR_RESET_FLAGS();
      }




      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 0);
    fillScreen(BLACK);
    ST7735_WriteString(0, 2, "START RUNNING",Font_7x10, WHITE, BLACK);
    HAL_Delay(1000);


    HAL_Delay(1000);
    HAL_IWDG_Refresh(&hiwdg);

    char buf[30]="";
   	status0++;
   	BKP->DR2 = status0;
   	sprintf(buf,"restart num-> %u",status0);
   	ST7735_WriteString(2, 30, buf,Font_7x10, WHITE, BLACK);
   if (status0>3)sleep();






   HAL_IWDG_Refresh(&hiwdg);


   ESP_Init();
   fillScreen(BLACK);
  ST7735_WriteString(0, 2, "wifi connected",Font_7x10, WHITE, BLACK);
HAL_Delay(500);





HAL_IWDG_Refresh(&hiwdg);
fillScreen(BLACK);
ST7735_WriteString(2, 20, "ready to building connection",Font_11x18, WHITE, BLACK);
HAL_Delay(500);
if (stupid_SCAU("SCAUNET")!=ESP8266_OK){
	while(1);

}



  while (1)
  {
	  get_Temperature();


      BH1750_ReadData(g_ucaDataBuff,2);
      ftmp = (g_ucaDataBuff[0]<<8 | g_ucaDataBuff[1]) / 1.2f + 0.5f;


      voltage=INA226_GetBusV();
      current=INA226_GetCurrent();
      power=INA226_GetPower();


//      ESP_interact_once("1.1.1.1",Temp[0],Temp[1],Temp[2],Temp[4],Temp[3],ftmp,current, power, voltage);
      if(Interaction("1.2.3.4",1234,Temp[0],Temp[1],Temp[2],Temp[4],Temp[3],ftmp,current, power, voltage)==ESP8266_OK)
      {HAL_IWDG_Refresh(&hiwdg);
		status0 = 0;
	    BKP->DR2 = status0;

	  	  if (read_DATA[2]==1){
	  		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 1);
	  	  }
	  	  else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 0);

      }
      else HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, 0);


/

	  	sprintf(str_V,"voltage: %.2f",voltage);
	  	sprintf(str_I,"current: %.2f",current);
	  	sprintf(str_P,"power: %.2f",power);
	  	sprintf(str_Lux,"illum %.2f",ftmp);
	  	sprintf(str_temper,"%.2f,%.2f,%.2f,%.2f,%.2f",Temp[0],Temp[1],Temp[2],Temp[3],Temp[4]);
	  	sprintf(str_spray,"is_spray -> %lu",read_DATA[2]);
	  	sprintf(str_time,"%lu--%lu",read_DATA[0],read_DATA[1]);

	  	  fillScreen(BLACK);
	  	  ST7735_WriteString(2, 2, str_V,Font_7x10, BLUE, BLACK);
	  	  ST7735_WriteString(2, 17, str_I,Font_7x10, BLUE, BLACK);
	  	  ST7735_WriteString(2, 32,str_P,Font_7x10, BLUE, BLACK);
	  	ST7735_WriteString(2, 47,str_Lux,Font_7x10, WHITE, BLACK);
	  	ST7735_WriteString(2, 62,"temperature",Font_7x10, WHITE, BLACK);
	  	ST7735_WriteString(2, 77,str_temper,Font_7x10, WHITE, BLACK);
	  	ST7735_WriteString(2, 107,"VPS connected",Font_7x10, GREEN, BLACK);



//	  	ST7735_WriteString(2, 125,str_time,Font_7x10, WHITE, BLACK);
//	  	ST7735_WriteString(2, 145,str_spray,Font_7x10, WHITE, BLACK);

	  	  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);



	  	  HAL_Delay(200);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
