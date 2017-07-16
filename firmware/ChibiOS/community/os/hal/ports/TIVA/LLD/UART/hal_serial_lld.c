/*
    Copyright (C) 2014..2016 Marco Veeneman

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    TIVA/LLD/serial_lld.c
 * @brief   Tiva low level serial driver code.
 *
 * @addtogroup SERIAL
 * @{
 */

#include "hal.h"

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/** @brief UART0 serial driver identifier.*/
#if TIVA_SERIAL_USE_UART0 || defined(__DOXYGEN__)
SerialDriver SD1;
#endif

/** @brief UART1 serial driver identifier.*/
#if TIVA_SERIAL_USE_UART1 || defined(__DOXYGEN__)
SerialDriver SD2;
#endif

/** @brief UART2 serial driver identifier.*/
#if TIVA_SERIAL_USE_UART2 || defined(__DOXYGEN__)
SerialDriver SD3;
#endif

/** @brief UART3 serial driver identifier.*/
#if TIVA_SERIAL_USE_UART3 || defined(__DOXYGEN__)
SerialDriver SD4;
#endif

/** @brief UART4 serial driver identifier.*/
#if TIVA_SERIAL_USE_UART4 || defined(__DOXYGEN__)
SerialDriver SD5;
#endif

/** @brief UART5 serial driver identifier.*/
#if TIVA_SERIAL_USE_UART5 || defined(__DOXYGEN__)
SerialDriver SD6;
#endif

/** @brief UART6 serial driver identifier.*/
#if TIVA_SERIAL_USE_UART6 || defined(__DOXYGEN__)
SerialDriver SD7;
#endif

/** @brief UART7 serial driver identifier.*/
#if TIVA_SERIAL_USE_UART7 || defined(__DOXYGEN__)
SerialDriver SD8;
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/** @brief Driver default configuration.*/
static const SerialConfig sd_default_config =
{
  SERIAL_DEFAULT_BITRATE,
  0,
  UART_LCRH_FEN | UART_LCRH_WLEN_8,
  UART_IFLS_TX4_8 | UART_IFLS_RX7_8,
  UART_CC_CS_SYSCLK
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/**
 * @brief   UART initialization.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] config    the architecture-dependent serial driver configuration
 */
static void uart_init(SerialDriver *sdp, const SerialConfig *config)
{
  uint32_t u = sdp->uart;
  uint32_t brd;
  uint32_t speed = config->speed;
  uint32_t clock_source;

  if (config->ctl & UART_CTL_HSE) {
    /* High speed mode is enabled, half the baud rate to compensate
     * for high speed mode.*/
    speed = (speed + 1) / 2;
  }

  if ((config->cc & UART_CC_CS_SYSCLK) == UART_CC_CS_SYSCLK) {
    /* UART is clocked using the SYSCLK.*/
    clock_source = TIVA_SYSCLK * 8;
  }
  else {
    /* UART is clocked using the PIOSC.*/
    clock_source = 16000000 * 8;
  }

  /* Calculate the baud rate divisor */
  brd = ((clock_source / speed) + 1) / 2;

  /* Disable UART.*/
  HWREG(u + UART_O_CTL) &= ~UART_CTL_UARTEN;

  /* Set baud rate.*/
  HWREG(u + UART_O_IBRD) = brd / 64;
  HWREG(u + UART_O_FBRD) = brd % 64;

  /* Line control/*/
  HWREG(u + UART_O_LCRH) = config->lcrh;

  /* Select clock source.*/
  HWREG(u + UART_O_CC) = config->cc & UART_CC_CS_M;

  /* FIFO configuration.*/
  HWREG(u + UART_O_IFLS) = config->ifls & (UART_IFLS_RX_M | UART_IFLS_TX_M);

  /* Note that some bits are enforced.*/
  HWREG(u + UART_O_CTL)  = config->ctl | UART_CTL_RXE | UART_CTL_TXE | UART_CTL_UARTEN;

  /* Enable interrupts.*/
  HWREG(u + UART_O_IM)   = UART_IM_RXIM | UART_IM_TXIM | UART_IM_RTIM;
}

/**
 * @brief   UART de-initialization.
 *
 * @param[in] u         pointer to an UART I/O block
 */
static void uart_deinit(uint32_t u)
{
  HWREG(u + UART_O_CTL) &= ~UART_CTL_UARTEN;
}

/**
 * @brief   Error handling routine.
 *
 * @param[in] sdp       communication channel associated to the UART
 * @param[in] err       UART LSR register value
 */
static void set_error(SerialDriver *sdp, uint16_t err)
{
  eventflags_t sts = 0;
  
  if (err & UART_MIS_FEMIS)
    sts |= SD_FRAMING_ERROR;
  if (err & UART_MIS_PEMIS)
    sts |= SD_PARITY_ERROR;
  if (err & UART_MIS_BEMIS)
    sts |= SD_BREAK_DETECTED;
  if (err & UART_MIS_OEMIS)
    sts |= SD_OVERRUN_ERROR;
  osalSysLockFromISR();
  chnAddFlagsI(sdp, sts);
  osalSysUnlockFromISR();
}

/**
 * @brief   Common IRQ handler.
 * @note    Tries hard to clear all the pending interrupt sources, we don't
 *          want to go through the whole ISR and have another interrupt soon
 *          after.
 *
 * @param[in] u         pointer to an UART I/O block
 * @param[in] sdp       communication channel associated to the UART
 */
static void serial_serve_interrupt(SerialDriver *sdp) 
{
  uint32_t u = sdp->uart;
  uint16_t mis = HWREG(u + UART_O_MIS);
  
  HWREG(u + UART_O_ICR) = mis;		/* clear interrupts */

  if (mis & (UART_MIS_FEMIS | UART_MIS_PEMIS | UART_MIS_BEMIS | UART_MIS_OEMIS)) {
    set_error(sdp, mis);
  }

  if ((mis & UART_MIS_RXMIS) || (mis &  UART_MIS_RTMIS)) {
    osalSysLockFromISR();
    if (iqIsEmptyI(&sdp->iqueue)) {
      chnAddFlagsI(sdp, CHN_INPUT_AVAILABLE);
    }
    osalSysUnlockFromISR();
    while ((HWREG(u + UART_O_FR) & UART_FR_RXFE) == 0) {
      osalSysLockFromISR();
      if (iqPutI(&sdp->iqueue, HWREG(u + UART_O_DR)) < Q_OK) {
        chnAddFlagsI(sdp, SD_OVERRUN_ERROR);
      }
      osalSysUnlockFromISR();
    }
  }

  if (mis & UART_MIS_TXMIS) {
    while ((HWREG(u + UART_O_FR) & UART_FR_TXFF) == 0) {
      msg_t b;
      osalSysLockFromISR();
      b = oqGetI(&sdp->oqueue);
      osalSysUnlockFromISR();
      if (b < Q_OK) {
        HWREG(u + UART_O_IM) &= ~UART_IM_TXIM;
        osalSysLockFromISR();
        chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
        osalSysUnlockFromISR();
        break;
      }
      HWREG(u + UART_O_DR) = b;
    }
  }
}

/**
 * @brief
 */
static void fifo_load(SerialDriver *sdp)
{
  uint32_t u = sdp->uart;

  while ((HWREG(u + UART_O_FR) & UART_FR_TXFF) == 0) {
    msg_t b = oqGetI(&sdp->oqueue);
    if (b < Q_OK) {
      chnAddFlagsI(sdp, CHN_OUTPUT_EMPTY);
      return;
    }
    HWREG(u + UART_O_DR) = b;
  }

  HWREG(u + UART_O_IM) |= UART_IM_TXIM;   /* transmit interrupt enable */
}

/**
 * @brief   Driver SD1 output notification.
 */
#if TIVA_SERIAL_USE_UART0 || defined(__DOXYGEN__)
static void notify1(io_queue_t *qp)
{
  (void)qp;
  fifo_load(&SD1);
}
#endif

/**
 * @brief   Driver SD2 output notification.
 */
#if TIVA_SERIAL_USE_UART1 || defined(__DOXYGEN__)
static void notify2(io_queue_t *qp)
{
  (void)qp;
  fifo_load(&SD2);
}
#endif

/**
 * @brief   Driver SD3 output notification.
 */
#if TIVA_SERIAL_USE_UART2 || defined(__DOXYGEN__)
static void notify3(io_queue_t *qp)
{
  (void)qp;
  fifo_load(&SD3);
}
#endif

/**
 * @brief   Driver SD4 output notification.
 */
#if TIVA_SERIAL_USE_UART3 || defined(__DOXYGEN__)
static void notify4(io_queue_t *qp)
{
  (void)qp;
  fifo_load(&SD4);
}
#endif

/**
 * @brief   Driver SD5 output notification.
 */
#if TIVA_SERIAL_USE_UART4 || defined(__DOXYGEN__)
static void notify5(io_queue_t *qp)
{
  (void)qp;
  fifo_load(&SD5);
}
#endif

/**
 * @brief   Driver SD6 output notification.
 */
#if TIVA_SERIAL_USE_UART5 || defined(__DOXYGEN__)
static void notify6(io_queue_t *qp)
{
  (void)qp;
  fifo_load(&SD6);
}
#endif

/**
 * @brief   Driver SD7 output notification.
 */
#if TIVA_SERIAL_USE_UART6 || defined(__DOXYGEN__)
static void notify7(io_queue_t *qp)
{
  (void)qp;
  fifo_load(&SD7);
}
#endif

/**
 * @brief   Driver SD8 output notification.
 */
#if TIVA_SERIAL_USE_UART7 || defined(__DOXYGEN__)
static void notify8(io_queue_t *qp)
{
  (void)qp;
  fifo_load(&SD8);
}
#endif

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief   UART0 IRQ handler.
 */
#if TIVA_SERIAL_USE_UART0 || defined(__DOXYGEN__)
#if !defined(TIVA_UART0_HANDLER)
#error "TIVA_UART0_HANDLER not defined"
#endif
CH_IRQ_HANDLER(TIVA_UART0_HANDLER)
{
  CH_IRQ_PROLOGUE();

  serial_serve_interrupt(&SD1);

  CH_IRQ_EPILOGUE();
}
#endif

/**
 * @brief   UART1 IRQ handler.
 */
#if TIVA_SERIAL_USE_UART1 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(TIVA_UART1_HANDLER)
{
  CH_IRQ_PROLOGUE();

  serial_serve_interrupt(&SD2);

  CH_IRQ_EPILOGUE();
}
#endif

/**
 * @brief   UART2 IRQ handler.
 */
#if TIVA_SERIAL_USE_UART2 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(TIVA_UART2_HANDLER)
{
  CH_IRQ_PROLOGUE();

  serial_serve_interrupt(&SD3);

  CH_IRQ_EPILOGUE();
}
#endif

/**
 * @brief   UART3 IRQ handler.
 */
#if TIVA_SERIAL_USE_UART3 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(TIVA_UART3_HANDLER)
{
  CH_IRQ_PROLOGUE();

  serial_serve_interrupt(&SD4);

  CH_IRQ_EPILOGUE();
}
#endif

/**
 * @brief   UART4 IRQ handler.
 */
#if TIVA_SERIAL_USE_UART4 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(TIVA_UART4_HANDLER)
{
  CH_IRQ_PROLOGUE();

  serial_serve_interrupt(&SD5);

  CH_IRQ_EPILOGUE();
}
#endif

/**
 * @brief   UART5 IRQ handler.
 */
#if TIVA_SERIAL_USE_UART5 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(TIVA_UART5_HANDLER)
{
  CH_IRQ_PROLOGUE();

  serial_serve_interrupt(&SD6);

  CH_IRQ_EPILOGUE();
}
#endif

/**
 * @brief   UART6 IRQ handler.
 */
#if TIVA_SERIAL_USE_UART6 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(TIVA_UART6_HANDLER)
{
  CH_IRQ_PROLOGUE();

  serial_serve_interrupt(&SD7);

  CH_IRQ_EPILOGUE();
}
#endif

/**
 * @brief   UART7 IRQ handler.
 */
#if TIVA_SERIAL_USE_UART7 || defined(__DOXYGEN__)
CH_IRQ_HANDLER(TIVA_UART7_HANDLER)
{
  CH_IRQ_PROLOGUE();

  serial_serve_interrupt(&SD8);

  CH_IRQ_EPILOGUE();
}
#endif

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level serial driver initialization.
 */
void sd_lld_init(void)
{
#if TIVA_SERIAL_USE_UART0
  sdObjectInit(&SD1, NULL, notify1);
  SD1.uart = UART0_BASE;
#endif

#if TIVA_SERIAL_USE_UART1
  sdObjectInit(&SD2, NULL, notify2);
  SD2.uart = UART1_BASE;
#endif

#if TIVA_SERIAL_USE_UART2
  sdObjectInit(&SD3, NULL, notify3);
  SD3.uart = UART2_BASE;
#endif

#if TIVA_SERIAL_USE_UART3
  sdObjectInit(&SD4, NULL, notify4);
  SD4.uart = UART3_BASE;
#endif

#if TIVA_SERIAL_USE_UART4
  sdObjectInit(&SD5, NULL, notify5);
  SD5.uart = UART4_BASE;
#endif

#if TIVA_SERIAL_USE_UART5
  sdObjectInit(&SD6, NULL, notify6);
  SD6.uart = UART5_BASE;
#endif

#if TIVA_SERIAL_USE_UART6
  sdObjectInit(&SD7, NULL, notify7);
  SD7.uart = UART6_BASE;
#endif

#if TIVA_SERIAL_USE_UART7
  sdObjectInit(&SD8, NULL, notify8);
  SD8.uart = UART7_BASE;
#endif
}

/**
 * @brief   Low level serial driver configuration and (re)start.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] config    the architecture-dependent serial driver configuration.
 *                      If this parameter is set to @p NULL then a default
 *                      configuration is used.
 */
void sd_lld_start(SerialDriver *sdp, const SerialConfig *config)
{
  if (config == NULL)
    config = &sd_default_config;

  if (sdp->state == SD_STOP) {
#if TIVA_SERIAL_USE_UART0
    if (&SD1 == sdp) {
      HWREG(SYSCTL_RCGCUART) |= (1 << 0);

      while (!(HWREG(SYSCTL_PRUART) & (1 << 0)))
        ;

      nvicEnableVector(TIVA_UART0_NUMBER, TIVA_SERIAL_UART0_PRIORITY);
    }
#endif
#if TIVA_SERIAL_USE_UART1
    if (&SD2 == sdp) {
      HWREG(SYSCTL_RCGCUART) |= (1 << 1);

      while (!(HWREG(SYSCTL_PRUART) & (1 << 1)))
        ;

      nvicEnableVector(TIVA_UART1_NUMBER, TIVA_SERIAL_UART1_PRIORITY);
    }
#endif
#if TIVA_SERIAL_USE_UART2
    if (&SD3 == sdp) {
      HWREG(SYSCTL_RCGCUART) |= (1 << 2);

      while (!(HWREG(SYSCTL_PRUART) & (1 << 2)))
        ;

      nvicEnableVector(TIVA_UART2_NUMBER, TIVA_SERIAL_UART2_PRIORITY);
    }
#endif
#if TIVA_SERIAL_USE_UART3
    if (&SD4 == sdp) {
      HWREG(SYSCTL_RCGCUART) |= (1 << 3);

      while (!(HWREG(SYSCTL_PRUART) & (1 << 3)))
        ;

      nvicEnableVector(TIVA_UART3_NUMBER, TIVA_SERIAL_UART3_PRIORITY);
    }
#endif
#if TIVA_SERIAL_USE_UART4
    if (&SD5 == sdp) {
      HWREG(SYSCTL_RCGCUART) |= (1 << 4);

      while (!(HWREG(SYSCTL_PRUART) & (1 << 4)))
        ;

      nvicEnableVector(TIVA_UART4_NUMBER, TIVA_SERIAL_UART4_PRIORITY);
    }
#endif
#if TIVA_SERIAL_USE_UART5
    if (&SD6 == sdp) {
      HWREG(SYSCTL_RCGCUART) |= (1 << 5);

      while (!(HWREG(SYSCTL_PRUART) & (1 << 5)))
        ;

      nvicEnableVector(TIVA_UART5_NUMBER, TIVA_SERIAL_UART5_PRIORITY);
    }
#endif
#if TIVA_SERIAL_USE_UART6
    if (&SD7 == sdp) {
      HWREG(SYSCTL_RCGCUART) |= (1 << 6);

      while (!(HWREG(SYSCTL_PRUART) & (1 << 6)))
        ;

      nvicEnableVector(TIVA_UART6_NUMBER, TIVA_SERIAL_UART6_PRIORITY);
    }
#endif
#if TIVA_SERIAL_USE_UART7
    if (&SD8 == sdp) {
      HWREG(SYSCTL_RCGCUART) |= (1 << 7);

      while (!(HWREG(SYSCTL_PRUART) & (1 << 7)))
        ;

      nvicEnableVector(TIVA_UART7_NUMBER, TIVA_SERIAL_UART7_PRIORITY);
    }
#endif
  }
  uart_init(sdp, config);
}

/**
 * @brief   Low level serial driver stop.
 * @details De-initializes the UART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 */
void sd_lld_stop(SerialDriver *sdp)
{
  if (sdp->state == SD_READY) {
    uart_deinit(sdp->uart);
#if TIVA_SERIAL_USE_UART0
    if (&SD1 == sdp) {
      HWREG(SYSCTL_RCGCUART) &= ~(1 << 0);  /* disable UART0 module */
      nvicDisableVector(TIVA_UART0_NUMBER);
      return;
    }
#endif
#if TIVA_SERIAL_USE_UART1
    if (&SD2 == sdp) {
      HWREG(SYSCTL_RCGCUART) &= ~(1 << 1);  /* disable UART1 module */
      nvicDisableVector(TIVA_UART1_NUMBER);
      return;
    }
#endif
#if TIVA_SERIAL_USE_UART2
    if (&SD3 == sdp) {
      HWREG(SYSCTL_RCGCUART) &= ~(1 << 2);  /* disable UART2 module */
      nvicDisableVector(TIVA_UART2_NUMBER);
      return;
    }
#endif
#if TIVA_SERIAL_USE_UART3
    if (&SD4 == sdp) {
      HWREG(SYSCTL_RCGCUART) &= ~(1 << 3);  /* disable UART3 module */
      nvicDisableVector(TIVA_UART3_NUMBER);
      return;
    }
#endif
#if TIVA_SERIAL_USE_UART4
    if (&SD5 == sdp) {
      HWREG(SYSCTL_RCGCUART) &= ~(1 << 4);  /* disable UART4 module */
      nvicDisableVector(TIVA_UART4_NUMBER);
      return;
    }
#endif
#if TIVA_SERIAL_USE_UART5
    if (&SD6 == sdp) {
      HWREG(SYSCTL_RCGCUART) &= ~(1 << 5);  /* disable UART5 module */
      nvicDisableVector(TIVA_UART5_NUMBER);
      return;
    }
#endif
#if TIVA_SERIAL_USE_UART6
    if (&SD7 == sdp) {
      HWREG(SYSCTL_RCGCUART) &= ~(1 << 6);  /* disable UART6 module */
      nvicDisableVector(TIVA_UART6_NUMBER);
      return;
    }
#endif
#if TIVA_SERIAL_USE_UART7
    if (&SD8 == sdp) {
      HWREG(SYSCTL_RCGCUART) &= ~(1 << 7);  /* disable UART7 module */
      nvicDisableVector(TIVA_UART7_NUMBER);
      return;
    }
#endif
  }
}

#endif /* CH_HAL_USE_SERIAL */

/** @} */
