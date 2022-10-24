/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
 
package toroni.tp.detail;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.nio.charset.StandardCharsets;

import org.junit.jupiter.api.Test;

class TopicMessageTest {

  @Test
  void match() {
    assertTrue(TopicMsgDeserializer.topicMatches("/", false,
        "/", false));
    assertTrue(TopicMsgDeserializer.topicMatches("/", false,
        "/", true));
    assertFalse(TopicMsgDeserializer.topicMatches("/", false,
        "/a/b1", false));
    assertFalse(TopicMsgDeserializer.topicMatches("/", false,
        "/a", false));
    assertFalse(TopicMsgDeserializer.topicMatches("/a/b1", false,
        "/", false));
    assertTrue(TopicMsgDeserializer.topicMatches("/a/b1", false,
        "/", true));
    assertTrue(TopicMsgDeserializer.topicMatches("/a/b1", false,
        "/a/b1", true));
    assertTrue(TopicMsgDeserializer.topicMatches("/a/b1", false,
        "/a", true));
    assertFalse(TopicMsgDeserializer.topicMatches("/a", true,
        "/", false));
    assertTrue(TopicMsgDeserializer.topicMatches("/a", true,
        "/", true));
    assertTrue(TopicMsgDeserializer.topicMatches("/a", true,
        "/a/b1", false));
    assertTrue(TopicMsgDeserializer.topicMatches("/a", true,
        "/a", false));
    assertFalse(TopicMsgDeserializer.topicMatches("/a/b2", false,
        "/", false));
    assertTrue(TopicMsgDeserializer.topicMatches("/a/b2", false,
        "/", true));
    assertFalse(TopicMsgDeserializer.topicMatches("/a/b2", false,
        "/a/b1", false));
    assertTrue(TopicMsgDeserializer.topicMatches("/a/b2", false,
        "/a", true));
  }

  @Test
  void serializeDeserializeHappyCase() {
    String topic = "topic";
    String data = "ab";

    byte[] binary = new byte[TopicMsgSerializer.sizeOf(topic, data.length())];
    TopicMsgSerializer.serialize(binary, 5, true, topic, data);

    TopicMsgDeserializer.ResultMessagePair deserialized = TopicMsgDeserializer.deserializeAndFilter(
        binary, 5, topic, true);

    final String message = StandardCharsets.UTF_8.decode(deserialized.message).toString();
    assertTrue(deserialized.result);
    assertTrue(message.startsWith(new String(data)));
    assertEquals(data.length(), deserialized.message.limit());
  }

  @Test
  void serializeDeserializeGenEarlier() {
    String topic = "topic";
    String data = "ab";

    byte[] binary = new byte[TopicMsgSerializer.sizeOf(topic, data.length())];
    TopicMsgSerializer.serialize(binary, 5, true, topic, data);

    TopicMsgDeserializer.ResultMessagePair deserialized = TopicMsgDeserializer.deserializeAndFilter(
        binary, 7, topic, true);
    assertFalse(deserialized.result);
  }

  @Test
  void serializeDeserializeTopicDoesNotMatch() {
    String topic = "topic";
    String data = "ab";

    byte[] binary = new byte[TopicMsgSerializer.sizeOf(topic, data.length())];
    TopicMsgSerializer.serialize(binary, 5, false, topic, data);

    TopicMsgDeserializer.ResultMessagePair deserialized = TopicMsgDeserializer.deserializeAndFilter(
        binary, 5, "topicA", true);
    assertFalse(deserialized.result);
  }

  @Test
  void emptyAll() {
    String topic = "";
    String data = "";

    byte[] binary = new byte[TopicMsgSerializer.sizeOf(topic, data.length())];
    TopicMsgSerializer.serialize(binary, 5, false, topic, data);

    TopicMsgDeserializer.ResultMessagePair deserialized = TopicMsgDeserializer.deserializeAndFilter(
        binary, 5, "", true);
    assertTrue(deserialized.result);
  }

  @Test
  void assertOrInvalid() {
    /**
     * This test requires us to catch a segmentation fault, which under Java is
     * impossible because the JVM dies whenever a segmentation fault happens.
     * 
     * Possible workaround:
     * https://stackoverflow.com/questions/6344674/how-can-i-write-a-junit-test-for-a-jni-call-that-crashes
     */
  }

}
