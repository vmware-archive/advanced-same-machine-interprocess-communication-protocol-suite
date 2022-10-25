/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package toroni.tp.detail;

/**
 * Binary serializer for a topic message.
 * The layout is the following:
 * - 8b readerGen
 * - 1b postToDescendants
 * - Xb channelName (zero-terminated)
 * - Yb data
 */
public class TopicMsgSerializer {

  /**
   * The size of the topic message including system data.
   * 
   * @param channelName
   * @param dataLen
   * @return size in bytes
   */
  public static int sizeOf(String channelName, int dataLen) {
    return 8 // readerGen
        + 1 // postToDescendants
        + (channelName.length() + 1) // channelName (plus the zero at the end)
        + dataLen; // message itself
  }

  /**
   * Serialize to binary a topic message.
   * 
   * @param rbMsg:           preallocated memory to serialize onto
   * @param readerGen
   * @param postToDescedants
   * @param channelName
   * @param msg
   */
  public static void serialize(byte[] rbMsg, long readerGen, boolean postToDescedants,
      String channelName, byte[] msg) {

    int rbMsgInd = 0;

    // write readerGen
    byte[] readerGenByteArray = Util.longToByteArray(readerGen);
    System.arraycopy(readerGenByteArray, 0, rbMsg, rbMsgInd, 8);
    rbMsgInd += 8;

    // add postToDescendants
    rbMsg[rbMsgInd++] = (postToDescedants ? (byte) 1 : (byte) 0);

    // add channleName
    byte[] channelNameByteArray = channelName.getBytes();
    System.arraycopy(channelNameByteArray, 0, rbMsg, rbMsgInd, channelNameByteArray.length);
    rbMsgInd += channelNameByteArray.length;
    rbMsg[rbMsgInd++] = (byte) 0;

    // add msg
    System.arraycopy(msg, 0, rbMsg, rbMsgInd, msg.length);
  }
}
