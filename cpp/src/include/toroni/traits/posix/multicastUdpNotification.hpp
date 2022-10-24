/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TORONI_TRAITS_POSIX_MULTICASTUDPNOTIFICATION_HPP
#define TORONI_TRAITS_POSIX_MULTICASTUDPNOTIFICATION_HPP

#include "toroni/exception.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <string>
#include <unistd.h>

namespace toroni {
namespace traits {

/**
 * @brief  Multicast UDP Notification
 * @note
 * @retval None
 */
class MulticastUdpNotification {
public:
  MulticastUdpNotification(const char *mcastIp, uint16_t port,
                           const char *localIf);
  void Send() const;
  void Wait() const;
  bool Peek() const;
  int GetSd() const;
  ~MulticastUdpNotification();

private:
  int _sd;
  sockaddr_in _sendSockAddr;
};

inline MulticastUdpNotification::MulticastUdpNotification(const char *mcastIp,
                                                          uint16_t port,
                                                          const char *localIf) {
  _sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (_sd < 0) {
    throw exception("Error opening UdpReader error");
  }

  // Setup receiving
  /* Enable SO_REUSEADDR to allow multiple instances of this */
  /* application to receive copies of the multicast datagrams. */
  int reuse = 1;
  if (setsockopt(_sd, SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<char *>(&reuse), sizeof(reuse)) < 0) {
    close(_sd);
    throw exception("Setting SO_REUSEADDR error");
  }
  /* Bind to the proper port number with the IP address */
  /* specified as INADDR_ANY. */
  sockaddr_in localSock;
  ip_mreq group;
  memset(&localSock, 0, sizeof(localSock));
  localSock.sin_family = AF_INET;
  localSock.sin_port = htons(port);
  localSock.sin_addr.s_addr = INADDR_ANY;
  if (bind(_sd, reinterpret_cast<struct sockaddr *>(&localSock),
           sizeof(localSock))) {
    close(_sd);
    throw exception("Binding datagram socket error");
  }
  group.imr_multiaddr.s_addr = inet_addr(mcastIp);
  group.imr_interface.s_addr = inet_addr(localIf);
  if (setsockopt(_sd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                 reinterpret_cast<char *>(&group), sizeof(group)) < 0) {
    close(_sd);
    throw exception("Adding multicast group error");
  }

  // Setup sending
  _sendSockAddr.sin_family = AF_INET;
  _sendSockAddr.sin_addr.s_addr = inet_addr(mcastIp);
  _sendSockAddr.sin_port = htons(port);

  in_addr localInterface;
  localInterface.s_addr = inet_addr(localIf);
  if (setsockopt(_sd, IPPROTO_IP, IP_MULTICAST_IF,
                 reinterpret_cast<char *>(&localInterface),
                 sizeof(localInterface)) < 0) {
    close(_sd);
    throw exception("Error setting local interface error");
  }
}

/**
 * @brief  Send a notification to all readers
 * @note
 * @retval None
 */
inline void MulticastUdpNotification::Send() const {
  // Send 0 bytes
  if (sendto(_sd, nullptr, 0, MSG_DONTWAIT,
             reinterpret_cast<const sockaddr *>(&_sendSockAddr),
             sizeof(_sendSockAddr)) < 0) {
    throw exception(("Error sending. errno=" + std::to_string(errno)).c_str());
  }
}

/**
 * @brief  Wait for a notification. Returns immediately if one is available, or
 * * blocks if none is available.
 * @note
 * @retval None
 */
inline void MulticastUdpNotification::Wait() const {
  // Read 0 bytes
  ssize_t res = 0;
  do {
    res = recv(_sd, nullptr, 0, 0);
  } while (res < 0 && errno == EINTR);

  if (res < 0) {
    throw exception("Error read " /* + to_string(errno)*/);
  }
}

/**
 * @brief  Peeks if there is a notification
 * @note
 * @retval True if there is a notification, false otherwise.
 */
inline bool MulticastUdpNotification::Peek() const {
  // Read 0 bytes
  ssize_t res = 0;
  do {
    res = recv(_sd, nullptr, 0, MSG_DONTWAIT | MSG_PEEK);
  } while (res < 0 && errno == EINTR);

  return (res == 0) || (res < 0 && errno != EAGAIN);
}

inline MulticastUdpNotification::~MulticastUdpNotification() { close(_sd); }

/**
 * @brief  The socket file descriptor
 * @note
 * @retval The socket file descriptor
 */
inline int MulticastUdpNotification::GetSd() const { return _sd; }

} // namespace traits
} // namespace toroni

#endif // TORONI_TRAITS_POSIX_MULTICASTUDPNOTIFICATION_HPP
