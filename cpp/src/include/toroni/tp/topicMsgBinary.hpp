#ifndef TORONI_TP_TOPICMSGBINARY_HPP
#define TORONI_TP_TOPICMSGBINARY_HPP

#include "toroni/ref.hpp"

#include <string>
#include <vector>

namespace toroni {
namespace tp {

using TopicMsgBinary = std::vector<char>;
using TopicMsgBinaryPtr = Ref<TopicMsgBinary>;

} // namespace tp
} // namespace toroni

#endif // TORONI_TP_TOPICMSGBINARY_HPP
