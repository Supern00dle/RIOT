/*
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 * Copyright (C) 2016 Eistec AB
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup  sys_xtimer Timers
 * @ingroup   sys
 * @brief     Provides a high level timer module to register
 *            timers, get current system time, and let a thread sleep for
 *            a certain amount of time.
 *
 * The implementation takes one low-level timer and multiplexes it.
 *
 * Insertion and removal of timers has O(n) complexity with (n) being the
 * number of active timers.  The reason for this is that multiplexing is
 * realized by next-first singly linked lists.
 *
 * @{
 * @file
 * @brief   xtimer interface definitions
 * @author  Kaspar Schleiser <kaspar@schleiser.de>
 * @author  Joakim Nohlgård <joakim.nohlgard@eistec.se>
 */
#ifndef XTIMER_H
#define XTIMER_H

#include <stdint.h>
#include "timex.h"
#include "msg.h"

#include "board.h"
#include "periph_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief xtimer callback type
 */
typedef void (*timer_callback_t)(void*);

/**
 * @brief xtimer timer structure
 */
typedef struct xtimer {
    struct xtimer *next;        /**< reference to next timer in timer lists */
    uint32_t target;            /**< lower 32bit absolute target time */
    uint32_t long_target;       /**< upper 32bit absolute target time */
    timer_callback_t callback;  /**< callback function to call when timer
                                     expires */
    void *arg;                  /**< argument to pass to callback function */
} xtimer_t;

/**
 * @brief get the current system time as 32bit microsecond value
 *
 * @note    Overflows every ~71minutes, thus returns xtimer_now64() % 32,
 *          but is more efficient.
 *
 * @return  current time as 32bit microsecond value
 */
static inline uint32_t xtimer_now(void);

/**
 * @brief get the current system time as 64bit microsecond value
 *
 * @return  current time as 64bit microsecond value
 */
uint64_t xtimer_now64(void);

/**
 * @brief get the current system time into a timex_t
 *
 * @param[out] out  pointer to timex_t the time will be written to
 */
void xtimer_now_timex(timex_t *out);

/**
 * @brief xtimer initialization function
 *
 * This sets up xtimer. Has to be called once at system boot.
 * If @ref auto_init is enabled, it will call this for you.
 */
void xtimer_init(void);

/**
 * @brief Pause the execution of a thread for some seconds
 *
 * When called from an ISR, this function will spin and thus block the MCU in
 * interrupt context for the specified amount in *seconds*, so don't *ever* use
 * it there.
 *
 * @param[in] seconds   the amount of seconds the thread should sleep
 */
static void xtimer_sleep(uint32_t seconds);

/**
 * @brief Pause the execution of a thread for some microseconds
 *
 * When called from an ISR, this function will spin and thus block the MCU for
 * the specified amount in microseconds, so only use it there for *very* short
 * periods, e.g., less than XTIMER_BACKOFF.
 *
 * @param[in] microseconds  the amount of microseconds the thread should sleep
 */
static void xtimer_usleep(uint32_t microseconds);

/**
 * @brief Stop execution of a thread for some time, 64bit version
 *
 * When called from an ISR, this function will spin and thus block the MCU for
 * the specified amount in microseconds, so only use it there for *very* short
 * periods, e.g., less then XTIMER_BACKOFF.
 *
 * @param[in] microseconds  the amount of microseconds the thread should sleep
 */
static inline void xtimer_usleep64(uint64_t microseconds);

/**
 * @brief Stop execution of a thread for some time
 *
 * Don't expect nanosecond accuracy. As of now, this function just calls
 * xtimer_usleep(nanoseconds/1000).
 *
 * When called from an ISR, this function will spin-block, so only use it there
 * for *very* short periods.
 *
 * @param[in] nanoseconds   the amount of nanoseconds the thread should sleep
 */
static void xtimer_nanosleep(uint32_t nanoseconds);

/**
 * @brief Stop execution of a thread for some time, blocking
 *
 * This function will spin-block, so only use it *very* short periods.
 *
 * @param[in] microseconds  the amount of microseconds the thread should spin
 */
static inline void xtimer_spin(uint32_t microseconds);

 /**
 * @brief will cause the calling thread to be suspended until the absolute
 * time (@p last_wakeup + @p interval).
 *
 * When the function returns, @p last_wakeup is set to
 * (@p last_wakeup + @p interval).
 *
 * This function can be used to create periodic wakeups.
 * @c last_wakeup should be set to xtimer_now() before first call of the
 * function.
 *
 * If the result of (@p last_wakeup + usecs) would be in the past, the function
 * sets @p last_wakeup to @p last_wakeup + @p interval and returns immediately.
 *
 * @param[in] last_wakeup   base time for the wakeup
 * @param[in] usecs         time in microseconds that will be added to
 *                          last_wakeup
 */
void xtimer_usleep_until(uint32_t *last_wakeup, uint32_t usecs);

/**
 * @brief Set a timer that sends a message
 *
 * This function sets a timer that will send a message @p offset microseconds
 * from now.
 *
 * The mesage struct specified by msg parameter will not be copied, e.g., it
 * needs to point to valid memory until the message has been delivered.
 *
 * @param[in] timer         timer struct to work with.
 *                          Its xtimer_t::target and xtimer_t::long_target
 *                          fields need to be initialized with 0 on first use.
 * @param[in] offset        microseconds from now
 * @param[in] msg           ptr to msg that will be sent
 * @param[in] target_pid    pid the message will be sent to
 */
void xtimer_set_msg(xtimer_t *timer, uint32_t offset, msg_t *msg, kernel_pid_t target_pid);

/**
 * @brief Set a timer that sends a message, 64bit version
 *
 * This function sets a timer that will send a message @p offset microseconds
 * from now.
 *
 * The mesage struct specified by msg parameter will not be copied, e.g., it
 * needs to point to valid memory until the message has been delivered.
 *
 * @param[in] timer         timer struct to work with.
 *                          Its xtimer_t::target and xtimer_t::long_target
 *                          fields need to be initialized with 0 on first use.
 * @param[in] offset        microseconds from now
 * @param[in] msg           ptr to msg that will be sent
 * @param[in] target_pid    pid the message will be sent to
 */
void xtimer_set_msg64(xtimer_t *timer, uint64_t offset, msg_t *msg, kernel_pid_t target_pid);

/**
 * @brief Set a timer that wakes up a thread
 *
 * This function sets a timer that will wake up a thread when the timer has
 * expired.
 *
 * @param[in] timer         timer struct to work with.
 *                          Its xtimer_t::target and xtimer_t::long_target
 *                          fields need to be initialized with 0 on first use
 * @param[in] offset        microseconds from now
 * @param[in] pid           pid of the thread that will be woken up
 */
void xtimer_set_wakeup(xtimer_t *timer, uint32_t offset, kernel_pid_t pid);

/**
 * @brief Set a timer that wakes up a thread, 64bit version
 *
 * This function sets a timer that will wake up a thread when the timer has
 * expired.
 *
 * @param[in] timer         timer struct to work with.
 *                          Its xtimer_t::target and xtimer_t::long_target
 *                          fields need to be initialized with 0 on first use
 * @param[in] offset        microseconds from now
 * @param[in] pid           pid of the thread that will be woken up
 */
void xtimer_set_wakeup64(xtimer_t *timer, uint64_t offset, kernel_pid_t pid);

/**
 * @brief Set a timer to execute a callback at some time in the future
 *
 * Expects timer->callback to be set.
 *
 * The callback specified in the timer struct will be executed @p offset
 * microseconds in the future.
 *
 * @warning BEWARE! Callbacks from xtimer_set() are being executed in interrupt
 * context (unless offset < XTIMER_BACKOFF). DON'T USE THIS FUNCTION unless you
 * know *exactly* what that means.
 *
 * @param[in] timer     the timer structure to use.
 *                      Its xtimer_t::target and xtimer_t::long_target
 *                      fields need to be initialized with 0 on first use
 * @param[in] offset    time in microseconds from now specifying that timer's
 *                      callback's execution time
 */
void xtimer_set(xtimer_t *timer, uint32_t offset);

/**
 * @brief remove a timer
 *
 * @note this function runs in O(n) with n being the number of active timers
 *
 * @param[in] timer ptr to timer structure that will be removed
 */
void xtimer_remove(xtimer_t *timer);

/**
 * @brief receive a message blocking but with timeout
 *
 * @param[out]  msg     pointer to a msg_t which will be filled in case of
 *                      no timeout
 * @param[in]   us      timeout in microseconds relative
 *
 * @return       < 0 on error, other value otherwise
 */
int xtimer_msg_receive_timeout(msg_t *msg, uint32_t us);

/**
 * @brief receive a message blocking but with timeout, 64bit version
 *
 * @param[out]   msg    pointer to a msg_t which will be filled in case of no
 *                      timeout
 * @param[in]    us     timeout in microseconds relative
 *
 * @return       < 0 on error, other value otherwise
 */
int xtimer_msg_receive_timeout64(msg_t *msg, uint64_t us);

/**
 * @brief xtimer backoff value
 *
 * All timers that are less than XTIMER_BACKOFF microseconds in the future will
 * just spin.
 *
 * This is supposed to be defined per-device in e.g., periph_conf.h.
 */
#ifndef XTIMER_BACKOFF
#define XTIMER_BACKOFF 30
#endif

/**
 * @brief xtimer overhead value
 *
 * This value specifies the time a timer will be late if uncorrected, e.g.,
 * the system-specific xtimer execution time from timer ISR to executing
 * a timer's callback's first instruction.
 *
 * E.g., with XTIMER_OVERHEAD == 0
 * start=xtimer_now();
 * xtimer_set(&timer, X);
 * (in callback:)
 * overhead=xtimer_now()-start-X;
 *
 * xtimer automatically substracts XTIMER_OVERHEAD from a timer's target time,
 * but when the timer triggers, xtimer will spin-lock until a timer's target
 * time is reached, so timers will never trigger early.
 *
 * This is supposed to be defined per-device in e.g., periph_conf.h.
 */
#ifndef XTIMER_OVERHEAD
#define XTIMER_OVERHEAD 20
#endif

#ifndef XTIMER_ISR_BACKOFF
/**
 * @brief   xtimer isr backoff time
 *
 * When scheduling the next isr, if it is less than the backoff time
 * in the future, just spin.
 *
 * This is supposed to be defined per-device in e.g., periph_conf.h.
 */
#define XTIMER_ISR_BACKOFF 20
#endif

#ifndef XTIMER_SHIFT
/**
 * @brief   xtimer prescaler value
 *
 * xtimer assumes it is running with an underlying 1MHz timer.
 * If the timer is slower by a power of two, XTIMER_SHIFT can be used to
 * adjust the difference.
 *
 * This will also initialize the underlying periph timer with
 * us_per_tick == (1<<XTIMER_SHIFT).
 *
 * For example, if the timer is running with 250khz, set XTIMER_SHIFT to 2.
 */
#define XTIMER_SHIFT (0)
#endif

/*
 * Default xtimer configuration
 */
#ifndef XTIMER_DEV
/**
 * @brief Underlying hardware timer device to assign to xtimer
 */
#define XTIMER_DEV TIMER_DEV(0)
/**
 * @brief Underlying hardware timer channel to assign to xtimer
 */
#define XTIMER_CHAN (0)

#if (TIMER_0_MAX_VALUE) == 0xfffffful
#define XTIMER_WIDTH (24)
#elif (TIMER_0_MAX_VALUE) == 0xffff
#define XTIMER_WIDTH (16)
#endif

#endif

#ifndef XTIMER_WIDTH
/**
 * @brief xtimer timer width
 *
 * This value specifies the width (in bits) of the hardware timer used by xtimer.
 * Default is 32.
 */
#define XTIMER_WIDTH (32)
#endif

#if XTIMER_WIDTH != 32
/**
 * @brief xtimer timer mask
 *
 * This value specifies the mask relative to 0xffffffff that the used timer
 * counts to, e.g., 0xffffffff & ~TIMER_MAXVALUE.
 *
 * For a 16bit timer, the mask would be 0xFFFF0000, for a 24bit timer, the mask
 * would be 0xFF000000.
 */
#define XTIMER_MASK ((0xffffffff >> XTIMER_WIDTH) << XTIMER_WIDTH)
#else
#define XTIMER_MASK (0)
#endif

#define XTIMER_MASK_SHIFTED XTIMER_TICKS_TO_USEC(XTIMER_MASK)

#include "xtimer/implementation.h"

#ifdef __cplusplus
}
#endif

/** @} */
#endif /* XTIMER_H */
