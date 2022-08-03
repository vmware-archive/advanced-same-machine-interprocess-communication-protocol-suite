#ifndef TORONI_TP_CHANNELREADER_HPP
#define TORONI_TP_CHANNELREADER_HPP

#include "readerInfo.hpp"

#include <functional>
#include <string>

namespace toroni {
namespace tp {

class Reader;

/**
 * @brief  A channel reader for a topic.
 * @note   Callback is invoked when there is a message for the topic only for
 * those
 * * messages that are created before the channel reader is created.
 * @retval None
 */
class ChannelReader {
public:
  using Handler = std::function<void(const void *, size_t)>;

  ChannelReader(const std::string &name, const Handler &handler,
                bool handleDescendents, ReaderGen readerGen);

private:
  const std::string _name;
  const Handler _handler;
  const bool _handleDescendents;
  const ReaderGen _readerGen;

  friend class Reader;
};

/**
 * @brief  Channel reader constructor.
 * @note
 * @param  &name: Topic to listen for.
 * @param  &handler: Invoked when there is a message for the topic only for
 * those messages that are created before the channel reader is created.
 * @param  handleDescendents: If set to true and reader listens for topic /a,
 * * receive also messages for topics /a/'*'
 * @param  readerGen: Unique identifier of the set of created chanelreaders
 * @retval None
 */

inline ChannelReader::ChannelReader(const std::string &name,
                                    const Handler &handler,
                                    bool handleDescendents, ReaderGen readerGen)
    : _name(name), _handler(handler), _handleDescendents(handleDescendents),
      _readerGen(readerGen) {}

} // namespace tp
} // namespace toroni

#endif // TORONI_TP_CHANNELREADER_HPP
