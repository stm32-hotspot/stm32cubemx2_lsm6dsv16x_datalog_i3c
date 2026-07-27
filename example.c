/**
  ******************************************************************************
  * file           : example.c
  * brief          : example program body
  *                   Called by the main after system initialization
  ******************************************************************************
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
/* Includes ------------------------------------------------------------------*/
#include "example.h"
#include "stm32_utils_i3c.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define BROADCAST_ADDR       0x7EU    /*!< I3C broadcast address */
#define BROADCAST_RSTDAA     0x06U    /*!< Reset Dynamic Address Assignment (broadcast CCC) */
#define BROADCAST_DISEC      0x01U    /*!< Disable Events Command (broadcast CCC) */

/* Private function prototypes -----------------------------------------------*/
int32_t i3c_rstdaa(hal_i3c_handle_t *hi3c);
int32_t i3c_disec(hal_i3c_handle_t *hi3c);
int32_t i3c_set_bus_frequency(hal_i3c_handle_t *hi3c, uint32_t i3c_freq);

/* Private variables */
lsm6dsv16x_object_t *pLsm6dsv16x0; /* pointer referencing the LSM6DSV16X object instance */
lsm6dsv16x_axes_t gAccData;  /* This variable stores the accelerometer measurement (unit: mg) */
lsm6dsv16x_axes_t gGyroData; /* This variable stores the gyroscope measurement (unit: mdps) */
hal_i3c_handle_t *pI3C;   /* Pointer referencing the I3C handle from the generated code */

/**
  * @brief  Send a broadcast RSTDAA CCC to reset all targets dynamic addresses.
  * @param  hi3c the I3C handle
  * @retval 0 in case of success, an error code otherwise
  *
  */
int32_t i3c_rstdaa(hal_i3c_handle_t *hi3c)
{
  int32_t ret = 0;
  /* Transfer context aggregating the control buffer for the broadcast CCC frame */
  hal_i3c_transfer_ctx_t ctx;
  /* Control buffer (one word per broadcast CCC descriptor) computed by the HAL */
  uint32_t control_buffer[1];
  hal_i3c_ccc_desc_t desc;

  desc.tgt_addr = BROADCAST_ADDR;
  desc.ccc = BROADCAST_RSTDAA;
  desc.data_size_byte = 0U;
  desc.direction = HAL_I3C_DIRECTION_WRITE;

  if (HAL_I3C_CTRL_ResetTransferCtx(&ctx) != HAL_OK)
  {
    ret = -1;
  }
  else if (HAL_I3C_CTRL_InitTransferCtxTc(&ctx, control_buffer,
                                          HAL_I3C_GET_CTRL_BUFFER_SIZE_WORD(1U,
                                                                            HAL_I3C_CCC_BROADCAST_WITHOUT_DEFBYTE_RESTART)) != HAL_OK)
  {
    ret = -1;
  }
  else if (HAL_I3C_CTRL_BuildTransferCtxCCC(&ctx, &desc, 1U,
                                            HAL_I3C_CCC_BROADCAST_WITHOUT_DEFBYTE_RESTART) != HAL_OK)
  {
    ret = -1;
  }
  else if (HAL_I3C_CTRL_Transfer(hi3c, &ctx, 0x1000) != HAL_OK)
  {
    ret = -1;
  }

  return ret;
}

/**
  * @brief  Send a broadcast DISEC CCC to disable target events (e.g. Hot-Join).
  * @param  hi3c the I3C handle
  * @retval 0 in case of success, an error code otherwise
  *
  */
int32_t i3c_disec(hal_i3c_handle_t *hi3c)
{
  int32_t ret = 0;
  /* Transfer context aggregating the control/Tx buffers for the broadcast CCC frame */
  hal_i3c_transfer_ctx_t ctx;
  /* Control buffer (one word per broadcast CCC descriptor) computed by the HAL */
  uint32_t control_buffer[1];
  hal_i3c_ccc_desc_t desc;
  uint8_t disec_data[1] = {0x08U};

  desc.tgt_addr = BROADCAST_ADDR;
  desc.ccc = BROADCAST_DISEC;
  desc.data_size_byte = (uint32_t)1U;
  desc.direction = HAL_I3C_DIRECTION_WRITE;

  if (HAL_I3C_CTRL_ResetTransferCtx(&ctx) != HAL_OK)
  {
    ret = -1;
  }
  else if (HAL_I3C_CTRL_InitTransferCtxTc(&ctx, control_buffer, 2U) != HAL_OK)
  {
    ret = -1;
  }
  else if (HAL_I3C_CTRL_InitTransferCtxTx(&ctx, disec_data, 1U) != HAL_OK)
  {
    ret = -1;
  }
  else if (HAL_I3C_CTRL_BuildTransferCtxCCC(&ctx, &desc, 1U,
                                            HAL_I3C_CCC_BROADCAST_WITHOUT_DEFBYTE_RESTART) != HAL_OK)
  {
    ret = -1;
  }
  else if (HAL_I3C_CTRL_Transfer(hi3c, &ctx, 0x1000) != HAL_OK)
  {
    ret = -1;
  }

  return ret;
}

/**
  * @brief  Set the I3C bus frequency.
  * @param  hi3c the I3C handle
  * @param  i3c_freq the bus frequency to be set in Hz
  * @retval 0 in case of success, an error code otherwise
  *
  * @note   The controller timing is computed from the current I3C kernel clock and
  *         applied through the HAL controller configuration.
  */
int32_t i3c_set_bus_frequency(hal_i3c_handle_t *hi3c, uint32_t i3c_freq)
{
  int32_t ret = 0;
  stm32_utils_i3c_ctrl_timing_config_t timing_cfg;
  hal_i3c_ctrl_config_t ctrl_cfg;

  timing_cfg.clock_src_freq_hz   = HAL_RCC_I3C_GetKernelClkFreq((I3C_TypeDef *)((hal_i3c_t)(hi3c->instance)));
  timing_cfg.i3c_pp_freq_hz      = i3c_freq;
  timing_cfg.i2c_od_freq_hz      = 0U;
  timing_cfg.duty_cycle_purcent  = 50U;
  timing_cfg.wait_time           = STM32_UTILS_I3C_ACTIVITY_STATE_0;
  timing_cfg.bus_type            = STM32_UTILS_I3C_PURE_I3C_BUS;

  if (STM32_UTILS_I3C_CTRL_ComputeTiming(&timing_cfg, &ctrl_cfg.timing_reg0, &ctrl_cfg.timing_reg1)
      != STM32_UTILS_I3C_OK)
  {
    ret = -1;
  }
  else if (HAL_I3C_CTRL_SetConfig(hi3c, &ctrl_cfg) != HAL_OK)
  {
    ret = -1;
  }

  return ret;
}

/** ########## Step 1 ##########
  * The init of LSM6DSV16X is triggered by the applicative code
  */
app_status_t app_init(void)
{
  app_status_t return_status = EXEC_STATUS_ERROR;
  hal_i3c_ctrl_config_t orig_config;

  /* Initialize the I3C peripheral */
  pI3C = mx_example_i3c_init();

  /* Save I3C original frequency settings */
  HAL_I3C_CTRL_GetConfig(pI3C, &orig_config);

  /* Set the bus speed to 1MHz for the dynamic addressing phase */
  if (i3c_set_bus_frequency(pI3C, 1000000U) != 0)
  {
    PRINTF("[ERROR] Step 1: I3C frequency set error\n");
    goto _app_init_exit;
  }
  /* Disable target events (e.g. Hot-Join) on the bus */
  if (i3c_disec(pI3C) != 0)
  {
    PRINTF("[ERROR] Step 1: DISEC broadcast error\n");
    goto _app_init_exit;
  }
  /* Reset all targets dynamic addresses */
  if (i3c_rstdaa(pI3C) != 0)
  {
    PRINTF("[ERROR] Step 1: RSTDAA broadcast error\n");
    goto _app_init_exit;
  }
  /* Restore the bus frequency to the original nominal value */
  if (HAL_I3C_CTRL_SetConfig(pI3C, &orig_config) != HAL_OK)
  {
    PRINTF("[ERROR] Step 1: I3C frequency set config error\n");
    goto _app_init_exit;
  }
  /* Retrieve and store the LSM6DSV16X object pointer */
  pLsm6dsv16x0 = MX_LSM6DSV16X_getobject();
  /* Initialize the LSM6DSV16X device 0 */
  if (lsm6dsv16x_drv_init(pLsm6dsv16x0, MX_LSM6DSV16X) != 0)
  {
    PRINTF("[ERROR] Step 1: LSM6DSV16X sensor init error\n");
    goto _app_init_exit;
  }
  PRINTF("[INFO] Step 1: LSM6DSV16X sensor init completed\n");

  /* LSM6DSV16X device 0: enable the accelerometer feature */
  if (lsm6dsv16x_drv_acc_enable(pLsm6dsv16x0) != 0)
  {
    PRINTF("[ERROR] Step 1: Enabling the ACC feature error\n");
    goto _app_init_exit;
  }
  PRINTF("[INFO] Step 1: Enabling the ACC feature completed\n");

  /* LSM6DSV16X device 0: enable the gyroscope feature */
  if (lsm6dsv16x_drv_gyro_enable(pLsm6dsv16x0) != 0)
  {
    PRINTF("[ERROR] Step 1: Enabling the GYRO feature error\n");
    goto _app_init_exit;
  }
  PRINTF("[INFO] Step 1: Enabling the GYRO feature completed\n");


  HAL_Delay(100);
  return_status = EXEC_STATUS_INIT_OK;

_app_init_exit:
  return return_status;
}

/** ########## Step 2 ##########
  * Gets the values from LSM6DSV16X.
  * The values are displayed on the terminal.
  * output: EXEC_STATUS_OK if OK, EXEC_STATUS_ERROR in case of error
  */
app_status_t app_process(void)
{
  app_status_t return_status = EXEC_STATUS_ERROR;

  /* LSM6DSV16X device 0: get the acceleration value and print it */
  if (lsm6dsv16x_drv_acc_get_axes(pLsm6dsv16x0, &gAccData) != 0)
  {
    PRINTF("[ERROR] Step 2: Reading acceleration error\n");
    goto _app_process_exit;
  }
  PRINTF("[INFO] Step 2: ACC X=%" PRIi32 " Y=%" PRIi32 " Z=%" PRIi32 "\n", (int32_t)gAccData.x_axis,
         (int32_t)gAccData.y_axis, (int32_t)gAccData.z_axis);

  /* LSM6DSV16X device 0: get the gyroscope value and print it */
  if (lsm6dsv16x_drv_gyro_get_axes(pLsm6dsv16x0, &gGyroData) != 0)
  {
    PRINTF("[ERROR] Step 2: Reading gyroscope error\n");
    goto _app_process_exit;
  }
  PRINTF("[INFO] Step 2: GYR X=%" PRIi32 " Y=%" PRIi32 " Z=%" PRIi32 "\n", (int32_t)gGyroData.x_axis,
         (int32_t)gGyroData.y_axis, (int32_t)gGyroData.z_axis);


  return_status = EXEC_STATUS_OK;

_app_process_exit:
  return return_status;
}

/** ########## Step 3 ##########
  * In this example, app_deinit is never called and is provided as a reference only.
  */
app_status_t app_deinit(void)
{
  lsm6dsv16x_drv_deinit(pLsm6dsv16x0);

  return EXEC_STATUS_OK;
}
