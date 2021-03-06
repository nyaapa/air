#pragma once

#include <netinet/in.h>
#include <cstddef>
#include <string_view>
#include <memory>

class udpclient {
 public:
	explicit udpclient();
	explicit udpclient(udpclient&&);

	~udpclient();

	void connect(const std::string&, ushort);

	void send(const std::string&);

 private:
	int fh;
	std::unique_ptr<sockaddr> servaddr;
	int servaddr_size;
};
