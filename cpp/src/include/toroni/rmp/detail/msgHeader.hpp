#ifndef TORONI_RMP_DETAIL_MSGHEADER_HPP
#define TORONI_RMP_DETAIL_MSGHEADER_HPP

namespace toroni {
namespace rmp {
namespace detail {
#pragma pack(1)
struct MsgHeader {
  enum Type { MSG, PADDING };

  char type;
  uint32_t length;

  bool Valid() const { return (type == MSG || type == PADDING) && length != 0; }
};
#pragma pack(0)
} // namespace detail
} // namespace rmp
} // namespace toroni
#endif // TORONI_RMP_DETAIL_MSGHEADER_HPP
