#include "Status.hpp"

Status::Status()
	: m_writer(0), m_readers(0)
{

}

Status::~Status()
{

}

Status::Status(const Status & other)
: m_writer(other.m_writer), m_readers(other.m_readers)
{

}

Status & Status::operator=(const Status & other)
{
	m_writer = other.m_writer;
	m_readers = other.m_readers;
	return *this;
}

Status::Status(Status && other)
: m_writer(other.m_writer), m_readers(other.m_readers)
{

}

Status & Status::operator=(Status && other)
{
	m_writer = other.m_writer;
	m_readers = other.m_readers;
	return *this;
}

void Status::lock_shared()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cv.wait(lock, [this](){return m_writer == 0;});
	m_readers++;
}

bool Status::try_lock_shared()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_writer == 0)
	{
		m_readers++;
		return true;
	}
	return false;
}

void Status::unlock_shared()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_readers == 0)
		LOG_CRITICAL("BIG BIG MISTAKE YOU'VE MADE");
	m_readers--;
	if (m_readers == 0)
		m_cv.notify_all();
}

void Status::lock()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cv.wait(lock, [this](){return m_writer == 0 && m_readers == 0;});
	m_writer = 1;
}

bool Status::try_lock()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_writer == 0 && m_readers == 0)
	{
		m_writer = 1;
		return true;
	}
	return false;
}

void Status::unlock()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_writer == 0)
		LOG_CRITICAL("BIG BIG MISTAKE YOU'VE MADE");
	m_writer = 0;
	m_cv.notify_all();
}

bool Status::isLocked() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_writer > 0;
}

bool Status::isShareLocked() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_readers > 0;
}

bool Status::isLockable() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_writer == 0 && m_readers == 0;
}

bool Status::isShareLockable() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_writer == 0;
}
