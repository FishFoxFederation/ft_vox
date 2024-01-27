#include "logger.hpp"

//################################################################
//#                                                              #
//#                          FileOutput                          #
//#                                                              #
//################################################################

FileOutput::FileOutput(const std::filesystem::path & path):
	std::ofstream(path)
{
	if (!is_open())
	{
		std::runtime_error("Could not open log file: " + path.string());
	}
}

FileOutput::FileOutput(FileOutput &&other):
	std::ofstream(std::move(other))
{
}

FileOutput::~FileOutput()
{
	close();
}

//################################################################
//#                                                              #
//#                            Logger                            #
//#                                                              #
//################################################################

// global instance of the logger
Logger logger = Logger();

Logger::Logger(): m_file_initialized(false) {}

Logger::Logger(const std::filesystem::path & path)
{
	configure(path);
}

void Logger::configure(const std::filesystem::path & path)
{
	m_log_files[0] = std::make_unique<FileOutput>(path.string() + "/critical.log");
	m_log_files[1] = std::make_unique<FileOutput>(path.string() + "/error.log");
	m_log_files[2] = std::make_unique<FileOutput>(path.string() + "/warning.log");
	m_log_files[3] = std::make_unique<FileOutput>(path.string() + "/info.log");
	m_log_files[4] = std::make_unique<FileOutput>(path.string() + "/debug.log");
	m_log_files[5] = std::make_unique<FileOutput>(path.string() + "/trace.log");
	m_file_initialized = true;
}

void Logger::setLevel(Level level)
{
	m_min_console_level = level;
}

Logger::Level Logger::level() const
{
	return m_min_console_level;
}

void Logger::setTimestamp(bool enabled)
{
	m_timestamp_enabled = enabled;
}

Logger & Logger::operator<<(Level level)
{
	if (!m_current_msg.str().empty())
	{
		std::runtime_error("Cannot change message level while a message is being logged. Please flush the message first with std::endl.");
	}

	m_next_msg_level = level;
	return *this;
}

Logger & Logger::operator<<(std::ostream & (*manipulator)(std::ostream &))
{
	if (manipulator == static_cast<std::ostream & (*)(std::ostream &)>(std::endl))
	{
		_flush();
	}
	else
	{
		m_current_msg << manipulator;
	}

	return *this;
}

void Logger::_flush()
{
	if (m_current_msg.str().empty())
	{
		return;
	}
	_writeToConsole(m_current_msg.str());
	if (m_file_initialized)
		_writeToFile(m_current_msg.str());
	m_current_msg.str("");
}

FileOutput & Logger::_logFile(Level level) const
{
	return *m_log_files[static_cast<int>(level)].get();
}

void Logger::_writeToConsole(std::string const & message)
{
	if (m_next_msg_level <= m_min_console_level)
	{
		std::cout << _timestamp() << _levelHeader(m_next_msg_level) << message << std::endl;
	}
}

void Logger::_writeToFile(std::string const & message)
{

	std::string finalMessage = _timestamp() + _levelHeader(m_next_msg_level, false) + message;
	switch (m_next_msg_level)
	{
		case Level::CRITICAL:
			_logFile(Level::CRITICAL) << finalMessage << std::endl;
			__attribute__ ((fallthrough));
		case Level::ERROR:
			_logFile(Level::ERROR) << finalMessage << std::endl;
			__attribute__ ((fallthrough));
		case Level::WARNING:
			_logFile(Level::WARNING) << finalMessage << std::endl;
			__attribute__ ((fallthrough));
		case Level::INFO:
			_logFile(Level::INFO) << finalMessage << std::endl;
			__attribute__ ((fallthrough));
		case Level::DEBUG:
			_logFile(Level::DEBUG) << finalMessage << std::endl;
			__attribute__ ((fallthrough));
		case Level::TRACE:
			_logFile(Level::TRACE) << finalMessage << std::endl;
			__attribute__ ((fallthrough));
		default:
			break;
	}
}

std::string Logger::_levelHeader(Level level, bool color)
{
	std::stringstream ss;
	ss << (color ? m_level_to_color[static_cast<int>(level)] : "")
		<< m_level_to_string[static_cast<int>(level)]
		<< (color ? "\033[0m " : " ");
	return ss.str();
}

std::string Logger::_timestamp()
{
	if (!m_timestamp_enabled)
	{
		return "";
	}
	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);
	std::stringstream ss;
	ss << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S] ");
	return ss.str();
}