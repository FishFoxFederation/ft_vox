#pragma once

#include "ft_format.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <memory>
#include <mutex>

#define LOG_CRITICAL(...) logger << Logger::Level::CRITICAL << __VA_ARGS__ << std::endl
#define LOG_ERROR(...) logger << Logger::Level::ERROR << __VA_ARGS__ << std::endl
#define LOG_WARNING(...) logger << Logger::Level::WARNING << __VA_ARGS__ << std::endl
#define LOG_INFO(...) logger << Logger::Level::INFO << __VA_ARGS__ << std::endl
#define LOG_DEBUG(...) logger << Logger::Level::DEBUG << __VA_ARGS__ << std::endl
#define LOG_TRACE(...) logger << Logger::Level::TRACE << __VA_ARGS__ << std::endl

#define LOG_CRITICAL_FMT(fmt, ...) logger << Logger::Level::CRITICAL << ft_format(fmt, __VA_ARGS__) << std::endl
#define LOG_ERROR_FMT(fmt, ...) logger << Logger::Level::ERROR << ft_format(fmt, __VA_ARGS__) << std::endl
#define LOG_WARNING_FMT(fmt, ...) logger << Logger::Level::WARNING << ft_format(fmt, __VA_ARGS__) << std::endl
#define LOG_INFO_FMT(fmt, ...) logger << Logger::Level::INFO << ft_format(fmt, __VA_ARGS__) << std::endl
#define LOG_DEBUG_FMT(fmt, ...) logger << Logger::Level::DEBUG << ft_format(fmt, __VA_ARGS__) << std::endl
#define LOG_TRACE_FMT(fmt, ...) logger << Logger::Level::TRACE << ft_format(fmt, __VA_ARGS__) << std::endl

// #define LOG_CRITICAL_FMT(fmt, ...) logger << Logger::Level::CRITICAL << fmt << std::endl, __VA_ARGS__

/**
 * @brief A class that represents a file used for output with RAII.
*/
class FileOutput: public std::ofstream
{

public:

	/**
	 * @brief Construct a new FileOutput object which opens a file.
	 *
	 * @param path The path to the file.
	 *
	 * @throw std::runtime_error if the file could not be opened.
	*/
	FileOutput(const std::filesystem::path & path);

	/**
	 * @brief Construct a new FileOutput object by moving another FileOutput object.
	*/
	FileOutput(FileOutput &&other);

	/**
	 * @brief Destroy the FileOutput object and close the file.
	*/
	~FileOutput();

};

/**
 * @brief A class for logging messages to the console and to files.
*/
class Logger
{

public:

	/**
	 * @brief The different levels of logging.
	*/
	enum Level
	{
		CRITICAL = 0,
		ERROR = 1,
		WARNING = 2,
		INFO = 3,
		DEBUG = 4,
		TRACE = 5,
		MAX = 6
	};


	/**
	 * @brief Construct a new Logger object.
	*/
	Logger();

	/**
	 * @brief Construct a new Logger object and open 5 different files for logging. If the files exist, they will be overwritten.
	 *
	 * @param path The path to the log files.
	 *
	 * @throw std::runtime_error if a log file could not be opened.
	*/
	Logger(const std::filesystem::path & path);

	/**
	 * @brief Open 5 different files for logging. If the files exist, they will be overwritten.
	 *
	 * @param path The path to the log files.
	 *
	 * @throw std::runtime_error if a log file could not be opened.
	*/
	void configure(const std::filesystem::path & path);

	/**
	 * @brief Set the minimum level of messages to log to the console.
	 *
	 * @param level The level to set.
	*/
	void setLevel(Level level);

	/**
	 * @brief Return the minimum level of messages to log to the console.
	*/
	Level level() const;

	/**
	 * @brief Enable or disable timestamps.
	 *
	 * @param enabled Whether or not to enable timestamps.
	*/
	void setTimestamp(bool enabled);

	/**
	 * @brief Set the message level.
	 *
	 * @param level The level to set.
	 *
	 * @throw std::runtime_error if a message is currently being logged.
	*/
	Logger & operator<<(Level level);

	/**
	 * @brief Transfer a manipulator to the stringstream buffer waiting to be flushed. If the manipulator is std::endl, the buffer will be flushed.
	 *
	 * @param manipulator The manipulator to transfer.
	*/
	Logger & operator<<(std::ostream & (*manipulator)(std::ostream &));

	/**
	 * @brief Transfer a argument to the stringstream buffer waiting to be flushed.
	 *
	 * @param arg The argument to transfer.
	*/
	template <typename T>
	Logger & operator<<(T const & arg)
	{
		m_current_msg << arg;
		return *this;
	}

private:

	/**
	 * @brief Whether or not the log files have been initialized.
	*/
	bool m_file_initialized = false;

	/**
	 * @brief Buffer for the current message.
	*/
	std::stringstream m_current_msg;

	/**
	 * @brief The log files.
	*/
	std::unique_ptr<FileOutput> m_log_files[static_cast<int>(Level::MAX)];

	/**
	 * @brief The minimum level of messages to log to the console.
	*/
	Level m_min_console_level = Level::TRACE;

	/**
	 * @brief The next message level.
	*/
	Level m_next_msg_level = Level::INFO;

	/**
	 * @brief Whether or not timestamps are enabled.
	*/
	bool m_timestamp_enabled = true;

	std::mutex m_mutex;

	/**
	 * @brief A map of the different levels of logging to their string representations.
	*/
	inline static std::string const m_level_to_string[static_cast<uint32_t>(MAX)] =
	{
		"[CRITICAL]:",
		"[ERROR]   :",
		"[WARNING] :",
		"[INFO]    :",
		"[DEBUG]   :",
		"[TRACE]   :"
	};

	/**
	 * @brief A map of the different levels of logging to their color codes.
	*/
	inline static std::string const m_level_to_color[static_cast<uint32_t>(MAX)] =
	{
		"\033[33;41;1m",
		"\033[31m",
		"\033[33m",
		"\033[32m",
		"\033[34m",
		"\033[35m"
	};

	/**
	 * @brief Flush the stringstream buffer to the console and to the appropriates log files.
	*/
	void _flush();

	/**
	 * @brief Return the appropriate log file for the message level.
	*/
	FileOutput & _logFile(Level level) const;

	/**
	 * @brief Write the message level to the console if the message level is greater than or equal to the minimum console level.
	*/
	void _writeToConsole(std::string const & message);

	/**
	 * @brief Write the message level to the appropriate log file.
	*/
	void _writeToFile(std::string const & message);

	/**
	 * @brief Return the header for the message level.
	*/
	std::string _levelHeader(Level level, bool color = true);

	/**
	 * @brief Return the current timestamp.
	*/
	std::string _timestamp();

};

extern Logger logger;
