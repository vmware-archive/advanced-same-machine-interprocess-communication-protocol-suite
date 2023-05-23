/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.tp.detail;

import java.nio.ByteBuffer;
import java.util.Arrays;

public class TopicMsgDeserializer {

  public static class ResultMessagePair {
    public boolean result;
    public ByteBuffer message;

    public ResultMessagePair(boolean result, ByteBuffer message) {
      this.result = result;
      this.message = message;
    }
  }

  /**
   * Deserializes a topic message if its topic matches. If it matches in the end
   * data contains only the message without the topic.
   *
   * @param data
   * @param length            number of bytes available in data
   * @param readerGen
   * @param channelName
   * @param handleDescendants
   * @return if topic message matches then a ResultMessagePair with result=true
   *         and message=the remaining message without the topic information,
   *         otherwise a ResultMessagePiar wiht result=false and message=null
   */
  public static ResultMessagePair deserializeAndFilter(byte[] data, int length, long readerGen,
      String channelName, boolean handleDescendants) {
    assert (length <= data.length);
    assert (length >= TopicMsgSerializer.sizeOf("", 0));

    long writerReaderGen = Util.readLongValue(data, 0);
    if (writerReaderGen < readerGen) {
      return new ResultMessagePair(false, null);
    }

    boolean writerPd = (data[8] == 1);

    int msgInd;
    ByteBuffer writerChannelBB;
    for (int i = 9;; i++) {
      if (data[i] == 0) {
        writerChannelBB = ByteBuffer.wrap(data, 9, i - 9).slice();
        msgInd = i + 1;
        break;
      }
    }

    final ByteBuffer bb = ByteBuffer.wrap(data, msgInd, length - msgInd).slice();
    if (topicMatches(channelName, handleDescendants, writerChannelBB,
        writerPd)) {
      return new ResultMessagePair(true, bb);
    }

    return new ResultMessagePair(false, null);
  }

  /**
   * Match writer+postToDescendents and reader+handleDescendents topics.
   *
   * @param readerChannel
   * @param handleDescendants
   * @param writerChannel
   * @param postToDescendants
   * @return true if they match, false otherwise
   */
  public static boolean topicMatches(String readerChannel,
      boolean handleDescendants,
      String writerChannel,
      boolean postToDescendants) {

    ByteBuffer writerChannelBB = ByteBuffer.wrap(writerChannel.getBytes());
    return topicMatches(readerChannel, handleDescendants, writerChannelBB,
        postToDescendants);
  }

  public static boolean topicMatches(String readerChannel,
      boolean handleDescendants,
      ByteBuffer writerChannel,
      boolean postToDescendants) {
    int mlen = Math.min(readerChannel.length(), writerChannel.limit());
    int clen = 0;

    while (clen < mlen &&
        readerChannel.charAt(clen) == (char) writerChannel.get(clen)) {
      clen++;
    }

    if (clen == readerChannel.length() && clen == writerChannel.limit()) {
      return true;
    }

    if (postToDescendants && clen == writerChannel.limit()) {
      return true;
    }

    if (handleDescendants && clen == readerChannel.length()) {
      return true;
    }

    return false;
  }
}
