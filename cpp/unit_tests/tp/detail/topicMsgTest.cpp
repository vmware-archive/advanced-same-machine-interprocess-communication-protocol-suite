/*
 * Copyright 2022 VMware, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "toroni/tp/detail/topicMsgBinaryDeserializer.hpp"
#include "toroni/tp/detail/topicMsgBinarySerializer.hpp"

#include <gmock/gmock-more-matchers.h>
#include <gtest/gtest.h>

#include <string>

using namespace toroni::tp;
using namespace std;
using namespace ::testing;

static bool TopicMatches(const std::string &readerChannel,
                         bool handleDescendents, const string &writerChannel,
                         bool postToDescendents) {
  return TopicMsgBinaryDeserializer::TopicMatches(
      readerChannel, handleDescendents, writerChannel.c_str(),
      writerChannel.size(), postToDescendents);
}

TEST(TopicMsg, Match) {
  EXPECT_TRUE(TopicMatches("/", false, "/", false));
  EXPECT_TRUE(TopicMatches("/", false, "/", true));
  EXPECT_FALSE(TopicMatches("/", false, "/a/b1", false));
  EXPECT_FALSE(TopicMatches("/", false, "/a", false));
  EXPECT_FALSE(TopicMatches("/a/b1", false, "/", false));
  EXPECT_TRUE(TopicMatches("/a/b1", false, "/", true));
  EXPECT_TRUE(TopicMatches("/a/b1", false, "/a/b1", false));
  EXPECT_TRUE(TopicMatches("/a/b1", false, "/a", true));
  EXPECT_FALSE(TopicMatches("/a", true, "/", false));
  EXPECT_TRUE(TopicMatches("/a", true, "/", true));
  EXPECT_TRUE(TopicMatches("/a", true, "/a/b1", false));
  EXPECT_TRUE(TopicMatches("/a", true, "/a", false));
  EXPECT_FALSE(TopicMatches("/a/b2", false, "/", false));
  EXPECT_TRUE(TopicMatches("/a/b2", false, "/", true));
  EXPECT_FALSE(TopicMatches("/a/b2", false, "/a/b1", false));
  EXPECT_TRUE(TopicMatches("/a/b2", false, "/a", true));
}

TEST(TopicMsg, SerializeDeserializeHappyCase) {
  const string topic{"topic"};
  const string data{"ab"};

  vector<char> binary(TopicMsgBinarySerializer::SizeOf(topic, data.size()));
  TopicMsgBinarySerializer::Serialize(binary.data(), 5, true, topic,
                                      data.c_str(), data.size());

  const char *outData = binary.data();
  uint32_t outDataLen = binary.size();

  EXPECT_TRUE(TopicMsgBinaryDeserializer::DeserializeAndFilter(
      outData, outDataLen, 5, topic, true));
  EXPECT_THAT(outData, StartsWith(data));
  EXPECT_EQ(outDataLen, data.size());
}

TEST(TopicMsg, SerializeDeserializeGenEarlier) {
  const string topic{"topic"};
  const string data{"ab"};

  vector<char> binary(TopicMsgBinarySerializer::SizeOf(topic, data.size()));
  TopicMsgBinarySerializer::Serialize(binary.data(), 5, true, topic,
                                      data.c_str(), data.size());

  const char *outData = binary.data();
  uint32_t outDataLen = binary.size();

  EXPECT_FALSE(TopicMsgBinaryDeserializer::DeserializeAndFilter(
      outData, outDataLen, 7, topic, true));
}

TEST(TopicMsg, SerializeDeserializeTopicDoesNotMatch) {
  const string topic{"topic"};
  const string data{"ab"};

  vector<char> binary(TopicMsgBinarySerializer::SizeOf(topic, data.size()));
  TopicMsgBinarySerializer::Serialize(binary.data(), 5, false, topic,
                                      data.c_str(), data.size());

  const char *outData = binary.data();
  uint32_t outDataLen = binary.size();

  EXPECT_FALSE(TopicMsgBinaryDeserializer::DeserializeAndFilter(
      outData, outDataLen, 5, "topicA", true));
}

TEST(TopicMsg, EmptyAll) {
  const string topic;
  const string data;

  vector<char> binary(TopicMsgBinarySerializer::SizeOf(topic, data.size()));
  TopicMsgBinarySerializer::Serialize(binary.data(), 5, false, topic,
                                      data.c_str(), data.size());

  const char *outData = binary.data();
  uint32_t outDataLen = binary.size();

  EXPECT_TRUE(TopicMsgBinaryDeserializer::DeserializeAndFilter(
      outData, outDataLen, 5, "", true));
}

TEST(TopicMsg, AssertOnInvalid) {
  vector<char> binary = {'x'};
  const char *outData = binary.data();
  uint32_t outDataLen = binary.size();

  EXPECT_DEBUG_DEATH(TopicMsgBinaryDeserializer::DeserializeAndFilter(
                         outData, outDataLen, 5, "", true),
                     "");
}