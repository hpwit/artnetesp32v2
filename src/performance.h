

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "sdkconfig.h"

#include "esp_log.h"
static const char *TAG2 = "perfmon";

static uint32_t idle0Calls = 0;
static uint32_t idle1Calls = 0;

static uint32_t ti = 0 ;

#if defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_240)
static const uint64_t MaxIdleCalls = 1855000;
#elif defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_160)
static const uint64_t MaxIdleCalls = 1233100;
#else
#error "Unsupported CPU frequency"
#endif

#define INCLUDE_xTaskGetIdleTaskHandle  0
#define configUSE_TRACE_FACILITY  1
#define configGENERATE_RUN_TIME_STATS  1
//#if ( configUSE_TRACE_FACILITY == 1 )

    UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                      const UBaseType_t uxArraySize,
                                      uint32_t * const pulTotalRunTime )
    {
        UBaseType_t uxTask = 0, uxQueue = configMAX_PRIORITIES;

#ifdef ESP_PLATFORM // IDF-3755
        //taskENTER_CRITICAL();
#else
        vTaskSuspendAll();
#endif // ESP_PLATFORM
        {
            /* Is there a space in the array for each task in the system? */
            if( uxArraySize >= 0 ) //uxCurrentNumberOfTasks
            {
                /* Fill in an TaskStatus_t structure with information on each
                 * task in the Ready state. */
                do
                {
                    uxQueue--;
                    uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &( pxReadyTasksLists[ uxQueue ] ), eReady );
                } while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

                /* Fill in an TaskStatus_t structure with information on each
                 * task in the Blocked state. */
               // uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), ( List_t * ) pxDelayedTaskList, eBlocked );
               // uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), ( List_t * ) pxOverflowDelayedTaskList, eBlocked );

                #if ( INCLUDE_vTaskDelete == 1 )
                    {
                        /* Fill in an TaskStatus_t structure with information on
                         * each task that has been deleted but not yet cleaned up. */
                        //uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &xTasksWaitingTermination, eDeleted );
                    }
                #endif

                #if ( INCLUDE_vTaskSuspend == 1 )
                    {
                        /* Fill in an TaskStatus_t structure with information on
                         * each task in the Suspended state. */
                        //uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &xSuspendedTaskList, eSuspended );
                    }
                #endif

                #if ( configGENERATE_RUN_TIME_STATS == 1 )
                    {
                        if( pulTotalRunTime != NULL )
                        {
                            #ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
                                portALT_GET_RUN_TIME_COUNTER_VALUE( ( *pulTotalRunTime ) );
                            #else
                                *pulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
                            #endif
                        }
                    }
                #else /* if ( configGENERATE_RUN_TIME_STATS == 1 ) */
                    {
                        if( pulTotalRunTime != NULL )
                        {
                            *pulTotalRunTime = 0;
                        }
                    }
                #endif /* if ( configGENERATE_RUN_TIME_STATS == 1 ) */
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
#ifdef ESP_PLATFORM // IDF-3755
      //  taskEXIT_CRITICAL();
#else
        ( void ) xTaskResumeAll();
#endif // ESP_PLATFORM

        return uxTask;
    }

//#endif /* configUSE_TRACE_FACILITY */
/*----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )

    TaskHandle_t xTaskGetIdleTaskHandle( void )
    {
        /* If xTaskGetIdleTaskHandle() is called before the scheduler has been
         * started, then xIdleTaskHandle will be NULL. */
        configASSERT( ( xIdleTaskHandle[xPortGetCoreID()] != NULL ) );
        return xIdleTaskHandle[xPortGetCoreID()];
    }

    TaskHandle_t xTaskGetIdleTaskHandleForCPU( UBaseType_t cpuid )
    {
        configASSERT( cpuid < configNUM_CORES );
        configASSERT( ( xIdleTaskHandle[cpuid] != NULL ) );
        return xIdleTaskHandle[cpuid];
    }
#endif /* INCLUDE_xTaskGetIdleTaskHandle */

#ifndef _TICK



static bool idle_task_0()
{
	idle0Calls += 1;
	return true;
}

static bool idle_task_1()
{
	idle1Calls += 1;
	return true;
}

#else
static void idle_task_0()
{
	idle0Calls += 1;
	//return false;
}

static void idle_task_1()
{
	idle1Calls += 1;
	//return false;
}
#endif
static char _stats[5000];
void vTaskGetRunTimeStats(  char *pcWriteBuffer )
{
TaskStatus_t *pxTaskStatusArray;
volatile UBaseType_t uxArraySize, x;
uint32_t ulTotalRunTime, ulStatsAsPercentage;

   /* Make sure the write buffer does not contain a string. */
   *pcWriteBuffer = 0x00;

   /* Take a snapshot of the number of tasks in case it changes while this
   function is executing. */
   uxArraySize = uxTaskGetNumberOfTasks();

   /* Allocate a TaskStatus_t structure for each task.  An array could be
   allocated statically at compile time. */
   pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

   if( pxTaskStatusArray != NULL )
   {
      /* Generate raw status information about each task. */
      uxArraySize = uxTaskGetSystemState( pxTaskStatusArray,
                                 uxArraySize,
                                 &ulTotalRunTime );

      /* For percentage calculations. */
      ulTotalRunTime /= 100UL;

      /* Avoid divide by zero errors. */
      if( ulTotalRunTime > 0 )
      {
         /* For each populated position in the pxTaskStatusArray array,
         format the raw data as human readable ASCII data. */
         for( x = 0; x < uxArraySize; x++ )
         {
            /* What percentage of the total run time has the task used?
            This will always be rounded down to the nearest integer.
            ulTotalRunTimeDiv100 has already been divided by 100. */
            ulStatsAsPercentage =
                  pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;

            if( ulStatsAsPercentage > 0UL )
            {
               sprintf( pcWriteBuffer, "%stt%lutt%lu%%rn",
                                 pxTaskStatusArray[ x ].pcTaskName,
                                 pxTaskStatusArray[ x ].ulRunTimeCounter,
                                 ulStatsAsPercentage );
            }
            else
            {
               /* If the percentage is zero here then the task has
               consumed less than 1% of the total run time. */
               sprintf( pcWriteBuffer, "%stt%lutt<1%%rn",
                                 pxTaskStatusArray[ x ].pcTaskName,
                                 pxTaskStatusArray[ x ].ulRunTimeCounter );
            }

            pcWriteBuffer += strlen( ( char * ) pcWriteBuffer );
         }
      }

      /* The array is no longer needed, free the memory it consumes. */
      vPortFree( pxTaskStatusArray );
   }
}

static void perfmon_task(void *args)
{
	while (1)
	{
        
        long f=ESP.getCycleCount();
        float idle0 = idle0Calls;
		float idle1 = idle1Calls;
		idle0Calls = 0;
		idle1Calls = 0;

		//int cpu0 = 100.f -  idle0 / MaxIdleCalls * 100.f;
		//int cpu1 = 100.f - idle1 / MaxIdleCalls * 100.f;
        /*
        ESP_LOGI(TAG2, "max  %.2f", (float)(f-ti)/240000);
		ESP_LOGI(TAG2, "Core 0 at %f", idle0);
		ESP_LOGI(TAG2, "Core 1 at %f", idle1);
        ti=f;
        */
       vTaskGetRunTimeStats(_stats);
       ESP_LOGI(TAG2, "Core 0 at %s", _stats);
		// TODO configurable delay
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
	vTaskDelete(NULL);
}

esp_err_t perfmon_start()
{
	//ESP_ERROR_CHECK(esp_register_freertos_idle_hook_for_cpu(idle_task_0, 0));
	//ESP_ERROR_CHECK(esp_register_freertos_idle_hook_for_cpu(idle_task_1, 1));
	// TODO calculate optimal stack size
	xTaskCreate(perfmon_task, "perfmon", 2048, NULL, 0, NULL);
	return ESP_OK;
}