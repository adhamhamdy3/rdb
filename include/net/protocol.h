#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "io/buffer_util.h"
#include "io/parser.h"
#include "storage/buffer.h"
#include <cstdint>
#include <string>
#include <vector>

size_t const SMAX_MSG_LENGTH = (32 << 20); // server
size_t const CMAX_MSG_LENGTH = 4096;       // client
size_t const MAX_ARGS = 200 * 1000;

// data types tags
#define TAG_NIL 0 // nil
#define TAG_ERR 1 // error code + message
#define TAG_STR 2 // string
#define TAG_INT 3 // int64
#define TAG_DBL 4 // double
#define TAG_ARR 5 // array

// error codes
#define ERR_UNKNOWN 1 // unknown command
#define ERR_TOO_BIG 2 // response too big

namespace Protocol {

// for response serialization
size_t response_size(Buffer const& out, size_t header);
void response_begin(Buffer& out, size_t* header);
void response_end(Buffer& out, size_t header);

int32_t serialize_request(std::vector<std::string> const& command, Buffer& out);
int32_t deserialize_request(uint8_t const* request, uint32_t size, std::vector<std::string>& command);

int32_t deserialize_response(Buffer const& buffer);

} // namespace Protocol

#endif
