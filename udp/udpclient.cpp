#include "udpclient.hpp"

#include <fmt/core.h>
#include <exception>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

udpclient::udpclient() : fh{0} {}

udpclient::udpclient(udpclient&& o) : fh{o.fh}, servaddr{std::move(o.servaddr)}, servaddr_size{o.servaddr_size} {
	o.fh = 0;
}

udpclient::~udpclient() {
	if (fh)
		close(fh);
}

void udpclient::connect(const std::string& host, ushort port) {
	if (fh)
		throw std::runtime_error("Socket is already open");

	addrinfo hints, *addrs, *addrs_save;
	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &addrs))
		throw std::runtime_error(fmt::format("Failed to get addr info for '{}': {}", host, strerror(errno)));
	addrs_save = addrs;

	do {
		if (fh = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol); fh >= 0)
			break;
	} while ((addrs = addrs->ai_next));

	servaddr_size = addrs->ai_addrlen;
	servaddr = std::unique_ptr<sockaddr>(static_cast<sockaddr*>(aligned_alloc(alignof(sockaddr), servaddr_size)));
	memcpy(servaddr.get(), addrs->ai_addr, addrs->ai_addrlen);

	freeaddrinfo(addrs_save);
}

void udpclient::send(const std::string& data) {
	if (long len = sendto(fh, data.c_str(), data.length(), MSG_CONFIRM, servaddr.get(), servaddr_size);
	    len != static_cast<long>(data.length()))
		throw std::runtime_error(fmt::format("Failed to send message, delivered only {}: {}", len, strerror(errno)));
}
