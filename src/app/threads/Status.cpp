#include "Status.hpp"

Status::Status()
	: m_writers(0), m_readers(0)
{

}

Status::~Status()
{

}

Status::Status(const Status & other)
: m_writers(other.m_writers), m_readers(other.m_readers)
{

}

Status & Status::operator=(const Status & other)
{
	m_writers = other.m_writers;
	m_readers = other.m_readers;
	return *this;
}

Status::Status(Status && other)
: m_writers(other.m_writers), m_readers(other.m_readers)
{

}

Status & Status::operator=(Status && other)
{
	m_writers = other.m_writers;
	m_readers = other.m_readers;
	return *this;
}

void Status::addReader()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cv.wait(lock, [this](){return m_writers == 0;});
	m_readers++;
}

bool Status::tryAddReader()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_writers == 0)
	{
		m_readers++;
		return true;
	}
	return false;
}

void Status::removeReader()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_readers--;
	if (m_readers == 0)
		m_cv.notify_all();
}

void Status::addWriter()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_cv.wait(lock, [this](){return m_writers == 0 && m_readers == 0;});
	m_writers++;
}

bool Status::tryAddWriter()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (m_writers == 0 && m_readers == 0)
	{
		m_writers++;
		return true;
	}
	return false;
}

void Status::removeWriter()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_writers--;
	m_cv.notify_all();
}

bool Status::hasWriters() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_writers > 0;
}

bool Status::hasReaders() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_readers > 0;
}

bool Status::isReadable() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if(m_writers < 0)
		LOG_DEBUG("OK I FUCKED UP");
	return m_writers == 0;
}

bool Status::isWritable() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_writers == 0 && m_readers == 0;
}
