#pragma once
#include "connections.hpp"
#include <set>

#ifndef __SERVER_HPP__
#define __SERVER_HPP__


class server_mode
{
	asio::io_context &io_context;
	asio::io_context &network_io;
	user_settings current_settings;
	std::unique_ptr<rfc8489::stun_header> stun_header;
	std::atomic<uint16_t> external_ipv4_port;
	std::atomic<uint32_t> external_ipv4_address;
	std::atomic<uint16_t> external_ipv6_port;
	std::shared_mutex mutex_ipv6;
	std::array<uint8_t, 16> external_ipv6_address;
	const std::array<uint8_t, 16> zero_value_array;


	std::map<asio::ip::port_type, std::unique_ptr<udp_server>> udp_servers;

	std::shared_mutex mutex_wrapper_session_map_to_source_udp;
	std::map<std::shared_ptr<data_wrapper<udp_server, std::unique_ptr<udp_client>>>, udp::endpoint, std::owner_less<>> wrapper_session_map_to_source_udp;

	std::shared_mutex mutex_wrapper_channels;
	std::map<uint32_t, std::shared_ptr<data_wrapper<udp_server, std::unique_ptr<udp_client>>>> wrapper_channels;

	std::mutex mutex_expiring_wrapper;
	std::map<std::shared_ptr<data_wrapper<udp_server, std::unique_ptr<udp_client>>>, int64_t, std::owner_less<>> expiring_wrapper;

	asio::steady_timer timer_expiring_wrapper;
	asio::steady_timer timer_find_timeout;
	asio::steady_timer timer_stun;
	asio::steady_timer timer_keep_alive;
	ttp::task_group_pool &sequence_task_pool_local;
	ttp::task_group_pool &sequence_task_pool_peer;
	const size_t task_limit;

	std::unique_ptr<udp::endpoint> udp_target;

	void udp_server_incoming(std::unique_ptr<uint8_t[]> data, size_t data_size, udp::endpoint peer, asio::ip::port_type port_number);
	void udp_server_incoming_unpack(std::unique_ptr<uint8_t[]> data, size_t plain_size, udp::endpoint peer, asio::ip::port_type port_number);
	void udp_client_incoming(std::unique_ptr<uint8_t[]> data, size_t data_size, udp::endpoint peer, asio::ip::port_type port_number, std::shared_ptr<data_wrapper<udp_server, std::unique_ptr<udp_client>>> wrapper_session);

	void udp_server_incoming_new_connection(std::unique_ptr<uint8_t[]> data, size_t data_size, udp::endpoint peer, asio::ip::port_type port_number);

	bool create_new_udp_connection(std::unique_ptr<uint8_t[]> data, const uint8_t *data_ptr, size_t data_size, std::shared_ptr<data_wrapper<udp_server, std::unique_ptr<udp_client>>> wrapper, udp::endpoint peer);

	udp::endpoint get_remote_address(std::shared_ptr<data_wrapper<udp_server, std::unique_ptr<udp_client>>> wrapper_ptr);
	bool update_local_udp_target(udp_client *target_connector);
	void save_external_ip_address(uint32_t ipv4_address, uint16_t ipv4_port, const std::array<uint8_t, 16> &ipv6_address, uint16_t ipv6_port);

	void cleanup_expiring_data_connections();
	void loop_timeout_sessions();
	void loop_keep_alive();
	void send_stun_request(const asio::error_code &e);
	void find_expires(const asio::error_code &e);
	void expiring_wrapper_loops(const asio::error_code &e);
	void keep_alive(const asio::error_code& e);

public:
	server_mode() = delete;
	server_mode(const server_mode &) = delete;
	server_mode& operator=(const server_mode &) = delete;

	server_mode(asio::io_context &io_context_ref, asio::io_context &net_io, ttp::task_group_pool &seq_task_pool_local, ttp::task_group_pool &seq_task_pool_peer, size_t task_count_limit, const user_settings &settings)
		: io_context(io_context_ref),
		network_io(net_io),
		timer_expiring_wrapper(io_context),
		timer_find_timeout(io_context),
		timer_stun(io_context),
		timer_keep_alive(io_context),
		sequence_task_pool_local(seq_task_pool_local),
		sequence_task_pool_peer(seq_task_pool_peer),
		task_limit(task_count_limit),
		external_ipv4_port(0),
		external_ipv4_address(0),
		external_ipv6_port(0),
		external_ipv6_address{},
		zero_value_array{},
		current_settings(settings) {}

	server_mode(server_mode &&existing_server) noexcept
		: io_context(existing_server.io_context),
		network_io(existing_server.network_io),
		timer_expiring_wrapper(std::move(existing_server.timer_expiring_wrapper)),
		timer_find_timeout(std::move(existing_server.timer_find_timeout)),
		timer_stun(std::move(existing_server.timer_stun)),
		timer_keep_alive(std::move(existing_server.timer_keep_alive)),
		sequence_task_pool_local(existing_server.sequence_task_pool_local),
		sequence_task_pool_peer(existing_server.sequence_task_pool_peer),
		task_limit(existing_server.task_limit),
		external_ipv4_port(existing_server.external_ipv4_port.load()),
		external_ipv4_address(existing_server.external_ipv4_address.load()),
		external_ipv6_port(existing_server.external_ipv6_port.load()),
		external_ipv6_address{ existing_server.external_ipv6_address },
		zero_value_array{},
		current_settings(std::move(existing_server.current_settings)) {}

	~server_mode();
public:
	bool start();
};

#endif // !__SERVER_HPP__
