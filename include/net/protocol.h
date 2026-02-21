#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "io/buffer_util.h"
#include "net/response.h"
#include <cstdint>
#include <string>
#include <vector>

size_t const SMAX_MSG_LENGTH = (32 << 20); // server
size_t const CMAX_MSG_LENGTH = 4096;       // client

size_t const MAX_ARGS = 200 * 1000;

namespace Protocol {

int32_t serialize_request(std::vector<std::string> const& command, std::vector<uint8_t>& out);
int32_t deserialize_request(uint8_t const* request, uint32_t size, std::vector<std::string>& command);

void serialize_response(Response const& resp, std::vector<uint8_t>& outgoing);
int32_t deserialize_response(std::vector<uint8_t> const& buffer);

} // namespace Protocol

#endif
