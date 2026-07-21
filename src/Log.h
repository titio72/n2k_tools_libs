#ifndef LOG_H_
#define LOG_H_

#define LOG()

/*
 * Logger class for debug and trace messages. It provides static methods to log messages with module and action context. The logger can be enabled or disabled, and the debug level can be set.
 * The DO_LOGGER macro controls whether logging is enabled or disabled at compile time. When logging is disabled, the logging methods do nothing.
*/

#ifndef DO_LOGGER
#define DO_LOGGER 1
#endif

#if DO_LOGGER==1
#define LOG_DEBUGX(module, text, action, ...) Log::debugx(module, action, text, ##__VA_ARGS__)
#define LOG_TRACEX(module, text, action, ...) Log::tracex(module, action, text, ##__VA_ARGS__)
#else
#define LOG_DEBUGX(module, text, action, ...)
#define LOG_TRACEX(module, text, action, ...)
#endif

class Log {
public:
	static void debug(const char* text, ...);
	static void trace(const char* text, ...);

	/**
	 * Log a debug message with module and action context. It adds a CR at the end of the message.
	 * @param module The module name
	 * @param action The action being performed
	 * @param text The log message, printf-style format string
	 */
	static void debugx(const char* module, const char* action, const char* text, ...);

	/**
	 * Log a trace message with module and action context. It adds a CR at the end of the message.
	 * Example usage: LOG_TRACEX("GPS", "Setup", "GPS has been initialized with value %d", value);
	 * @param module The module name
	 * @param action The action being performed
	 * @param text The log message, printf-style format string
	 */
	static void tracex(const char* module, const char* action, const char* text, ...);
	
	/**
	 * Log a trace message with module and action context, without a message. It adds a CR at the end of the message.
	 * Example usage: LOG_TRACEX("APP", "App started successfully");
	 * @param module The module name
	 * @param action The action being performed
	 */
	static void tracex(const char* module, const char* action);

	/**
	 * Set the debug level for the logger.
	 */
	static void setdebug();

	/**
	 * Enable the logger.
	 */
	static void enable();

	/**
	 * Disable the logger.
	 */
	static void disable();

	/**
	 * Check if the logger is enabled.
	 */
	static bool is_enabled();
};

#endif /* LOG_H_ */
