#pragma once

#include <netinet/in.h>
#include <string_view>
#include <vector>
#include <cstddef>

class udpclient {
 public:
	explicit udpclient();
	explicit udpclient(udpclient&&);

	~udpclient();

	void connect(const std::string&, ushort);

	void send(const std::string&);

 private:
	int fh;
	std::vector<std::byte> servaddr;
};
