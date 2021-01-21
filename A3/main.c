/**
  ******************************************************************************
  * @file    main.c
  * @author  P. Foubet
  * @version V1.0
  * @date    31-December-2018
  * @brief   Default main function.
  ******************************************************************************
*/

#include "FreeRTOS.h"
#include "task.h"
#include "interface.h"
#include "semphr.h"
#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"

TaskHandle_t xProductorHandler, xConsumerHandler;
SemaphoreHandle_t sem = NULL;
int productorPriority = 0;
int bamboo = 0;

void msleep(unsigned int ms)
{
	vTaskDelay(ms/portTICK_RATE_MS);
}

void productBAMBOO(void * args)
{
	while(1)
	{
		if (BSP_PB_GetState(BUTTON_KEY))
		{
			if (uxTaskPriorityGet(xProductorHandler) == 3)
				vTaskPrioritySet(xProductorHandler, 4);
			else
				vTaskPrioritySet(xProductorHandler, 3);
			HAL_Delay(300);
		}

		if (uxTaskPriorityGet(xProductorHandler) > 3)
		{
			vTaskSuspend(xConsumerHandler);
		}
		else
		{
			vTaskResume(xConsumerHandler);
		}

		if (sem != NULL)
		{
			if (xSemaphoreTake(sem, (TickType_t) 10) == pdTRUE)
			{
				bamboo++;
				BSP_LED_On(LED3);
				HAL_Delay(300);
				BSP_LED_Off(LED3);
				xSemaphoreGive(sem);
			}
			else
			{
				//TKT
			}
		}
		msleep(300);
	}
}

void consumeBAMBOO(void * args)
{
	while(1)
	{
		if (bamboo > 0)
			if (sem != NULL)
			{
				if (xSemaphoreTake(sem, (TickType_t) 10) == pdTRUE)
				{
					if (bamboo > productorPriority)
					{
						bamboo--;
						BSP_LED_On(LED4);
						HAL_Delay(300);
						BSP_LED_Off(LED4);
					}
					xSemaphoreGive(sem);
				}
				else
				{
					//TKT
				}
			}
		msleep(300);
	}
}

int main(void)
{
	interface_init();
	char * taskName[]=
	{
			"Producteur",
			"Consommateur",
	};
	//char * buttonTaskName = "PandaBtnTask";

	sem = xSemaphoreCreateMutex();

	/* creation des threads */
	if (!(pdPASS == xTaskCreate( productBAMBOO, taskName[0], 64, (void *) NULL,  3, &xProductorHandler ))) goto err;
	if (!(pdPASS == xTaskCreate( consumeBAMBOO, taskName[1], 64, (void *) NULL,  3, &xConsumerHandler ))) goto err;
	//if (!(pdPASS == xTaskCreate( buttonTask, buttonTaskName, 64, (void *) NULL,  configMAX_PRIORITIES-1, NULL ))) goto err;

	/* lancement du systeme */
	vTaskStartScheduler();
	err:              // en principe jamais atteint !
	  while(1);
  	  return 0;
}
