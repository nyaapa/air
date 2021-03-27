#include "udpclient.hpp"

#include <fmt/core.h>
#include <exception>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

udpclient::udpclient() : fh{0} {
	memset(&servaddr, 0, sizeof(servaddr));
}

udpclient::udpclient(udpclient&& o) : fh{o.fh} {
	memcpy(&servaddr, &o.servaddr, sizeof(servaddr));
	o.fh = 0;
}

udpclient::~udpclient() {
	if (fh)
		close(fh);
}

void udpclient::connect(const std::string& host, ushort port) {
	if (fh)
		throw std::runtime_error("Socket is already open");

	if (fh = socket(AF_INET, SOCK_DGRAM, 0); fh < 0)
		throw std::runtime_error(fmt::format("Failed to create a socket: {}", strerror(errno)));

	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = inet_addr(host.c_str());
}

void udpclient::send(const std::string& data) {
	if (long len = sendto(fh, data.c_str(), data.length(), MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof(servaddr)); len != static_cast<long>(data.length()))
		throw std::runtime_error(fmt::format("Failed to send message, delivered only {}: {}", len, strerror(errno)));
}
