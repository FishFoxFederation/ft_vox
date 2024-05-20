#pragma once

#include "define.hpp"
#include <unordered_map>
#include <exception>

#include "logger.hpp"
#include "ServerSocket.hpp"
#include "Connection.hpp"
#include "Poller.hpp"

class Server
{
public:
	Server (int port);
	~Server();

	Server(const Server& other) = delete;
	Server& operator=(const Server& other) = delete;
	Server(Server&& other) = delete;
	Server& operator=(Server&& other) = delete;

	/**
	 * @brief start the server and blocks until the server is stopped
	 * you might wanna run this in a separate thread
	 */
	void run();

	class ClientDisconnected : public std::exception
	{
	public:
		ClientDisconnected(uint64_t id) : m_id(id) {}
		virtual const char* what() const throw()
		{
			return "Client disconnected";
		}
		uint64_t id() const { return m_id; }
	private:
		uint64_t m_id;
	};
private:

	bool										m_running;
	Poller										m_poller;
	ServerSocket								m_server_socket;
	std::unordered_map<uint64_t, Connection>	m_connections;


	uint64_t									m_ids_counter;

	uint64_t get_new_id();

	int		read_data(Connection & connection, uint64_t id);
	int		send_data(Connection & connection, uint64_t id);
};
