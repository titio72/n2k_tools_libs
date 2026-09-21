/*
 * Log.cpp
 *
 *  Created on: Mar 26, 2019
 *      Author: aboni
 */
#ifndef NATIVE
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#else
#include <mutex>
#endif

#include "Log.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#define MAX_TRACE_SIZE 1024

static bool _debug = false;
static bool enabled = false;

static char outbfr[MAX_TRACE_SIZE];

/*
 * outbfr (and the serial port) are shared by every caller. Several tasks log (loop task, N2K task, BLE host
 * task...), so the format+print sequence is serialized: without it concurrent lines interleave.
 * The lock is only ever held for one line. A caller that cannot get it within LOG_LOCK_TIMEOUT_MS drops its
 * line rather than stalling, and callers in an ISR never wait (they drop the line): a mutex cannot be taken there.
 */
#define LOG_LOCK_TIMEOUT_MS 100

#ifndef NATIVE
static SemaphoreHandle_t log_mutex = xSemaphoreCreateMutex(); // NULL only if out of memory: then logging stays unlocked

class LogGuard
{
public:
	LogGuard() : locked(false), proceed(true)
	{
		if (log_mutex)
		{
			if (xPortInIsrContext())
			{
				proceed = false;
			}
			else
			{
				locked = xSemaphoreTake(log_mutex, pdMS_TO_TICKS(LOG_LOCK_TIMEOUT_MS)) == pdTRUE;
				proceed = locked;
			}
		}
	}
	~LogGuard()
	{
		if (locked)
			xSemaphoreGive(log_mutex);
	}
	bool ok() const { return proceed; }

private:
	bool locked;
	bool proceed;
};
#else
static std::mutex log_mutex;

class LogGuard
{
public:
	LogGuard() : lock(log_mutex) {}
	bool ok() const { return true; }

private:
	std::lock_guard<std::mutex> lock;
};
#endif

// snprintf/vsnprintf return the length the text *would* have had: clamp it to what actually fits
// (capacity includes the terminating 0), so the '\n' + 0 appended afterwards stay inside the buffer
static inline int clamp_len(int written, int capacity)
{
	if (written < 0)
		return 0;
	return written < capacity ? written : capacity - 1;
}

inline bool can_trace()
{
#ifndef NATIVE
	return enabled && Serial.availableForWrite();
#else
	return enabled;
#endif
}

const char *_gettime()
{
	static char _buffer[80];
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(_buffer, 80, "%T", timeinfo);
	return _buffer;
}

void _trace(const char *text)
{
#ifndef NATIVE
	Serial.print(text);
#else
	printf("%s", text);
	FILE *f = fopen("/var/log/nmea.log", "a+");
	if (f == NULL)
	{
		f = fopen("./nmea.log", "a+");
	}
	if (f)
	{
		fprintf(f, "%s %s\n", _gettime(), text);
		fclose(f);
	}
#endif
}

void Log::setdebug()
{
	_debug = true;
}

void Log::enable()
{
	enabled = true;
}

void Log::disable()
{
	enabled = false;
}

bool Log::is_enabled()
{
	return enabled;
}

void Log::debug(const char *text, ...)
{
	if (_debug && can_trace())
	{
		LogGuard guard;
		if (!guard.ok())
			return;
		va_list args;
		va_start(args, text);
		vsnprintf(outbfr, MAX_TRACE_SIZE, text, args);
		va_end(args);
		_trace(outbfr);
	}
}

void Log::trace(const char *text, ...)
{
	if (can_trace())
	{
		LogGuard guard;
		if (!guard.ok())
			return;
		va_list args;
		va_start(args, text);
		vsnprintf(outbfr, MAX_TRACE_SIZE, text, args);
		va_end(args);
		_trace(outbfr);
	}
}

void Log::debugx(const char *module, const char *action, const char *text, ...)
{
	if (_debug && can_trace())
	{
		LogGuard guard;
		if (!guard.ok())
			return;
		int l = clamp_len(snprintf(outbfr, MAX_TRACE_SIZE - 2, "[%s] %s: ", module, action), MAX_TRACE_SIZE - 2);
		char* _outbfr = (outbfr + l);
		int avail = MAX_TRACE_SIZE - 2 - l; // always >= 1
		va_list args;
		va_start(args, text);
		int l1 = clamp_len(vsnprintf(_outbfr, avail, text, args), avail);
		va_end(args);
		_outbfr[l1] = '\n';
		_outbfr[l1+1] = 0;
		_trace(outbfr);
	}
}

void Log::tracex(const char *module, const char *action, const char *text, ...)
{
	if (can_trace())
	{
		LogGuard guard;
		if (!guard.ok())
			return;
		int l = clamp_len(snprintf(outbfr, MAX_TRACE_SIZE - 2, "[%s] %s: ", module, action), MAX_TRACE_SIZE - 2);
		char* _outbfr = (outbfr + l);
		int avail = MAX_TRACE_SIZE - 2 - l; // always >= 1
		va_list args;
		va_start(args, text);
		int l1 = clamp_len(vsnprintf(_outbfr, avail, text, args), avail);
		va_end(args);
		_outbfr[l1] = '\n';
		_outbfr[l1+1] = 0;
		_trace(outbfr);
	}
}

void Log::tracex(const char *module, const char *action)
{
	if (can_trace())
	{
		LogGuard guard;
		if (!guard.ok())
			return;
		int l = clamp_len(snprintf(outbfr, MAX_TRACE_SIZE - 2, "[%s] %s", module, action), MAX_TRACE_SIZE - 2);
		outbfr[l] = '\n';
		outbfr[l+1] = 0;
		_trace(outbfr);
	}
}

