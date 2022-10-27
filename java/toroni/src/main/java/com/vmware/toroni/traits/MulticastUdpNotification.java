/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.traits;

import java.net.DatagramPacket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.MulticastSocket;
import java.net.NetworkInterface;
import java.net.SocketAddress;

/**
 * Multicast UDP Notification
 */
public class MulticastUdpNotification {

  private MulticastSocket _socket;
  private DatagramPacket _sendPacket;
  private DatagramPacket _recievePacket;

  public MulticastUdpNotification(String mcastIp, short port, String localIf) throws Exception {
    _sendPacket = new DatagramPacket(new byte[0], 0,
        InetAddress.getByName(mcastIp), port);
    _recievePacket = new DatagramPacket(new byte[0], 0);

    try {
      _socket = new MulticastSocket(port);
      // _socket.setOption(StandardSocketOptions.SO_REUSEPORT, true);

      SocketAddress group = new InetSocketAddress(mcastIp, port);
      // _socket.setNetworkInterface(NetworkInterface.getByName(localIf));
      NetworkInterface ni = NetworkInterface.getByName(localIf);

      _socket.joinGroup(group, ni);
    } catch (Exception e) {
      throw new Exception("Couldn't create a multicast socket");
    }
  }

  /**
   * Send a notification to all readers.
   */
  public void sendNotification() {
    try {
      _socket.send(_sendPacket);
    } catch (Exception e) {
      throw new Error("Error while sending");
    }
  }

  /**
   * Wait for a notification. Returns immidiately if one is available, or blocks
   * if none is available.
   */
  public void waitForNotification() {
    try {
      _socket.receive(_recievePacket);
    } catch (Exception e) {
      throw new Error("Error while recieving");
    }
  }

}
