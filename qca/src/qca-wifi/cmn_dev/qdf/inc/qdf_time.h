/*
 * Copyright (c) 2014-2018 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

/**
 * DOC: qdf_time
 * This file abstracts time related functionality.
 */

#ifndef _QDF_OS_TIME_H
#define _QDF_OS_TIME_H

#include <i_qdf_time.h>

typedef __qdf_ktime_t qdf_ktime_t;
typedef __qdf_time_t qdf_time_t;

/**
 * qdf_ns_to_ktime - Converts nanoseconds to a qdf_ktime_t object
 * @ns: time in nanoseconds
 *
 * Return: nanoseconds as qdf_ktime_t object
 */

static inline qdf_ktime_t qdf_ns_to_ktime(uint64_t ns)
{
	return __qdf_ns_to_ktime(ns);
}

/**
 * qdf_ktime_add - Adds two qdf_ktime_t objects and returns
 * a qdf_ktime_t object
 * @ktime1: time as qdf_ktime_t object
 * @ktime2: time as qdf_ktime_t object
 *
 * Return: sum of both qdf_ktime_t as qdf_ktime_t object
 */

static inline qdf_ktime_t qdf_ktime_add(qdf_ktime_t ktime1, qdf_ktime_t ktime2)
{
	return __qdf_ktime_add(ktime1, ktime2);
}

/**
 * qdf_ktime_get - Gets the current time as qdf_ktime_t object
 *
 * Return: current time as qdf_ktime_t object
 */

static inline qdf_ktime_t qdf_ktime_get(void)
{
	return __qdf_ktime_get();
}

/**
 * qdf_ktime_add_ns - Adds qdf_ktime_t object and nanoseconds value and
 * returns the qdf_ktime_t object
 * @ktime: time as qdf_ktime_t object
 * @ns: time in nanoseconds
 *
 * Return: qdf_ktime_t object
 */

static inline qdf_ktime_t qdf_ktime_add_ns(qdf_ktime_t ktime, int64_t ns)
{
	return __qdf_ktime_add_ns(ktime, ns);
}

/**
 * qdf_ktime_to_ns - Convert the qdf_ktime_t object into nanoseconds
 * @ktime: time as qdf_ktime_t object
 *
 * Return: qdf_ktime_t in nanoseconds
 */

static inline int64_t qdf_ktime_to_ns(qdf_ktime_t ktime)
{
	return __qdf_ktime_to_ns(ktime);
}

/**
 * qdf_ktime_sub - subtract two ktime_t variables
 * @lhs: minuend
 * @rhs: subtrahend
 *
 * Return: remainder of subtraction in ktime_t format
 */
static inline qdf_ktime_t qdf_ktime_sub(const qdf_ktime_t lhs,
		const qdf_ktime_t rhs)
{
	return __qdf_ktime_sub(lhs, rhs);
}

/**
 * qdf_ktime_to_us - convert ktime format to milliseconds
 * @kt: ktime_t variable
 *
 * Return: 64 bit variable conversion
 */
static inline int64_t qdf_ktime_to_ms(const qdf_ktime_t kt)
{
	return __qdf_ktime_to_ms(kt);
}

/**
 * qdf_ktime_to_us - convert ktime format to microseconds
 * @kt: ktime_t variable
 *
 * Return: 64 bit variable conversion
 */
static inline int64_t qdf_ktime_to_us(const qdf_ktime_t kt)
{
	return __qdf_ktime_to_us(kt);
}

/**
 * qdf_get_ktime_real - get the real (wall-) time in ktime_t format
 *
 * Return: ktime_t
 */
static inline qdf_ktime_t qdf_ktime_get_real(void)
{
	return __qdf_ktime_get_real();
}

/**
 * qdf_system_ticks - Count the number of ticks elapsed from the time when
 * the system booted
 *
 * Return: ticks
 */
static inline qdf_time_t qdf_system_ticks(void)
{
	return __qdf_system_ticks();
}

/**
 * qdf_system_ticks_to_msecs - convert ticks to milliseconds
 * @clock_ticks: Number of ticks
 *
 * Return: unsigned int Time in milliseconds
 */
static inline uint32_t qdf_system_ticks_to_msecs(unsigned long clock_ticks)
{
	return __qdf_system_ticks_to_msecs(clock_ticks);
}

/**
 * qdf_system_msecs_to_ticks - convert milliseconds to ticks
 * @msec: Time in milliseconds
 *
 * Return: unsigned long number of ticks
 */
static inline qdf_time_t qdf_system_msecs_to_ticks(uint32_t msecs)
{
	return __qdf_system_msecs_to_ticks(msecs);
}

/**
 * qdf_get_system_uptime - Return a monotonically increasing time
 * This increments once per HZ ticks
 *
 * Return: qdf_time_t system up time in ticks
 */
static inline qdf_time_t qdf_get_system_uptime(void)
{
	return __qdf_get_system_uptime();
}

/**
 * qdf_get_system_timestamp - Return current timestamp
 *
 * Return: unsigned long timestamp in ms.
 */
static inline unsigned long qdf_get_system_timestamp(void)
{
	return __qdf_get_system_timestamp();
}

/**
 * qdf_udelay - delay in microseconds
 * @usecs: Number of microseconds to delay
 *
 * Return: none
 */
static inline void qdf_udelay(int usecs)
{
	__qdf_udelay(usecs);
}

/**
 * qdf_mdelay - Delay in milliseconds.
 * @msec: Number of milliseconds to delay
 *
 * Return: none
 */
static inline void qdf_mdelay(int msecs)
{
	__qdf_mdelay(msecs);
}

/**
 * qdf_system_time_after() - Check if a is later than b
 * @a: Time stamp value a
 * @b: Time stamp value b
 *
 * Return:
 * true if a < b else false
 */
static inline bool qdf_system_time_after(qdf_time_t a, qdf_time_t b)
{
	return __qdf_system_time_after(a, b);
}

/**
 * qdf_system_time_before() - Check if a is before b
 * @a: Time stamp value a
 * @b: Time stamp value b
 *
 * Return:
 * true if a is before b else false
 */
static inline bool qdf_system_time_before(qdf_time_t a, qdf_time_t b)
{
	return __qdf_system_time_before(a, b);
}

/**
 * qdf_system_time_after_eq() - Check if a atleast as recent as b, if not
 * later
 * @a: Time stamp value a
 * @b: Time stamp value b
 *
 * Return:
 * true if a >= b else false
 */
static inline bool qdf_system_time_after_eq(qdf_time_t a, qdf_time_t b)
{
	return __qdf_system_time_after_eq(a, b);
}

/**
 * enum qdf_timestamp_unit - what unit the qdf timestamp is in
 * @KERNEL_LOG: boottime time in uS (micro seconds)
 * @QTIMER: QTIME in (1/19200)S
 *
 * This enum is used to distinguish which timer source is used.
 */
enum qdf_timestamp_unit {
	KERNEL_LOG,
	QTIMER,
};

#ifdef QCA_WIFI_3_0_ADRASTEA
#define QDF_LOG_TIMESTAMP_UNIT QTIMER
#else
#define QDF_LOG_TIMESTAMP_UNIT KERNEL_LOG
#endif


/**
 * qdf_get_log_timestamp - get time stamp for logging
 * For adrastea this API returns QTIMER tick which is needed to synchronize
 * host and fw log timestamps
 * For ROME and other discrete solution this API returns system boot time stamp
 *
 * Return:
 * QTIMER ticks(19.2MHz) for adrastea
 * System tick for rome and other future discrete solutions
 */
static inline uint64_t qdf_get_log_timestamp(void)
{
	return __qdf_get_log_timestamp();
}

/**
 * qdf_get_monotonic_boottime - get monotonic kernel boot time
 * This API is similar to qdf_get_system_boottime but it includes
 * time spent in suspend.
 *
 * Return: Time in microseconds
 */
static inline uint64_t qdf_get_monotonic_boottime(void)
{
	return __qdf_get_monotonic_boottime();
}

#endif
