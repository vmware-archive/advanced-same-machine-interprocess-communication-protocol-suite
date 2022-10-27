/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

package com.vmware.toroni.tp;

import java.nio.ByteBuffer;

/**
 * A channel reader for a topic.
 */
public class ChannelReader {

  public static interface Handler {
    void run(ByteBuffer data);
  }

  public String _name;
  public Handler _handler;
  public boolean _handleDescendants;
  public long _readerGen;

  /**
   * Channel reader constructor.
   * 
   * @param name:              topic to listen for
   * @param handler:           invoked when there is a message for the topic, only
   *                           for those messages that are created before the
   *                           channel reader is created
   * @param handleDescendants: if set to true and reader listens for topic /a,
   *                           also recieves messages for topic /a/*
   * @param readerGen:         unique identifier of the set of created
   *                           chanelreaders
   */
  public ChannelReader(String name, Handler handler, boolean handleDescendants, long readerGen) {
    _name = name;
    _handler = handler;
    _handleDescendants = handleDescendants;
    _readerGen = readerGen;
  }

}
