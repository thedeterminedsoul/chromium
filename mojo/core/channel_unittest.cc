// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/core/channel.h"

#include <atomic>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/optional.h"
#include "base/process/process_handle.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "mojo/core/platform_handle_utils.h"
#include "mojo/public/cpp/platform/platform_channel.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace core {
namespace {

class TestChannel : public Channel {
 public:
  TestChannel(Channel::Delegate* delegate)
      : Channel(delegate, Channel::HandlePolicy::kAcceptHandles) {}

  char* GetReadBufferTest(size_t* buffer_capacity) {
    return GetReadBuffer(buffer_capacity);
  }

  bool OnReadCompleteTest(size_t bytes_read, size_t* next_read_size_hint) {
    return OnReadComplete(bytes_read, next_read_size_hint);
  }

  MOCK_METHOD7(GetReadPlatformHandles,
               bool(const void* payload,
                    size_t payload_size,
                    size_t num_handles,
                    const void* extra_header,
                    size_t extra_header_size,
                    std::vector<PlatformHandle>* handles,
                    bool* deferred));
  MOCK_METHOD0(Start, void());
  MOCK_METHOD0(ShutDownImpl, void());
  MOCK_METHOD0(LeakHandle, void());

  void Write(MessagePtr message) override {}

 protected:
  ~TestChannel() override {}
};

// Not using GMock as I don't think it supports movable types.
class MockChannelDelegate : public Channel::Delegate {
 public:
  MockChannelDelegate() {}

  size_t GetReceivedPayloadSize() const { return payload_size_; }

  const void* GetReceivedPayload() const { return payload_.get(); }

 protected:
  void OnChannelMessage(const void* payload,
                        size_t payload_size,
                        std::vector<PlatformHandle> handles) override {
    payload_.reset(new char[payload_size]);
    memcpy(payload_.get(), payload, payload_size);
    payload_size_ = payload_size;
  }

  // Notify that an error has occured and the Channel will cease operation.
  void OnChannelError(Channel::Error error) override {}

 private:
  size_t payload_size_ = 0;
  std::unique_ptr<char[]> payload_;
};

Channel::MessagePtr CreateDefaultMessage(bool legacy_message) {
  const size_t payload_size = 100;
  Channel::MessagePtr message = std::make_unique<Channel::Message>(
      payload_size, 0,
      legacy_message ? Channel::Message::MessageType::NORMAL_LEGACY
                     : Channel::Message::MessageType::NORMAL);
  char* payload = static_cast<char*>(message->mutable_payload());
  for (size_t i = 0; i < payload_size; i++) {
    payload[i] = static_cast<char>(i);
  }
  return message;
}

void TestMemoryEqual(const void* data1,
                     size_t data1_size,
                     const void* data2,
                     size_t data2_size) {
  ASSERT_EQ(data1_size, data2_size);
  const unsigned char* data1_char = static_cast<const unsigned char*>(data1);
  const unsigned char* data2_char = static_cast<const unsigned char*>(data2);
  for (size_t i = 0; i < data1_size; i++) {
    // ASSERT so we don't log tons of errors if the data is different.
    ASSERT_EQ(data1_char[i], data2_char[i]);
  }
}

void TestMessagesAreEqual(Channel::Message* message1,
                          Channel::Message* message2,
                          bool legacy_messages) {
  // If any of the message is null, this is probably not what you wanted to
  // test.
  ASSERT_NE(nullptr, message1);
  ASSERT_NE(nullptr, message2);

  ASSERT_EQ(message1->payload_size(), message2->payload_size());
  EXPECT_EQ(message1->has_handles(), message2->has_handles());

  TestMemoryEqual(message1->payload(), message1->payload_size(),
                  message2->payload(), message2->payload_size());

  if (legacy_messages)
    return;

  ASSERT_EQ(message1->extra_header_size(), message2->extra_header_size());
  TestMemoryEqual(message1->extra_header(), message1->extra_header_size(),
                  message2->extra_header(), message2->extra_header_size());
}

TEST(ChannelTest, LegacyMessageDeserialization) {
  Channel::MessagePtr message = CreateDefaultMessage(true /* legacy_message */);
  Channel::MessagePtr deserialized_message =
      Channel::Message::Deserialize(message->data(), message->data_num_bytes());
  TestMessagesAreEqual(message.get(), deserialized_message.get(),
                       true /* legacy_message */);
}

TEST(ChannelTest, NonLegacyMessageDeserialization) {
  Channel::MessagePtr message =
      CreateDefaultMessage(false /* legacy_message */);
  Channel::MessagePtr deserialized_message =
      Channel::Message::Deserialize(message->data(), message->data_num_bytes());
  TestMessagesAreEqual(message.get(), deserialized_message.get(),
                       false /* legacy_message */);
}

TEST(ChannelTest, OnReadLegacyMessage) {
  size_t buffer_size = 100 * 1024;
  Channel::MessagePtr message = CreateDefaultMessage(true /* legacy_message */);

  MockChannelDelegate channel_delegate;
  scoped_refptr<TestChannel> channel = new TestChannel(&channel_delegate);
  char* read_buffer = channel->GetReadBufferTest(&buffer_size);
  ASSERT_LT(message->data_num_bytes(),
            buffer_size);  // Bad test. Increase buffer
                           // size.
  memcpy(read_buffer, message->data(), message->data_num_bytes());

  size_t next_read_size_hint = 0;
  EXPECT_TRUE(channel->OnReadCompleteTest(message->data_num_bytes(),
                                          &next_read_size_hint));

  TestMemoryEqual(message->payload(), message->payload_size(),
                  channel_delegate.GetReceivedPayload(),
                  channel_delegate.GetReceivedPayloadSize());
}

TEST(ChannelTest, OnReadNonLegacyMessage) {
  size_t buffer_size = 100 * 1024;
  Channel::MessagePtr message =
      CreateDefaultMessage(false /* legacy_message */);

  MockChannelDelegate channel_delegate;
  scoped_refptr<TestChannel> channel = new TestChannel(&channel_delegate);
  char* read_buffer = channel->GetReadBufferTest(&buffer_size);
  ASSERT_LT(message->data_num_bytes(),
            buffer_size);  // Bad test. Increase buffer
                           // size.
  memcpy(read_buffer, message->data(), message->data_num_bytes());

  size_t next_read_size_hint = 0;
  EXPECT_TRUE(channel->OnReadCompleteTest(message->data_num_bytes(),
                                          &next_read_size_hint));

  TestMemoryEqual(message->payload(), message->payload_size(),
                  channel_delegate.GetReceivedPayload(),
                  channel_delegate.GetReceivedPayloadSize());
}

class ChannelTestShutdownAndWriteDelegate : public Channel::Delegate {
 public:
  ChannelTestShutdownAndWriteDelegate(
      PlatformChannelEndpoint endpoint,
      scoped_refptr<base::TaskRunner> task_runner,
      scoped_refptr<Channel> client_channel,
      std::unique_ptr<base::Thread> client_thread,
      base::RepeatingClosure quit_closure)
      : quit_closure_(std::move(quit_closure)),
        client_channel_(std::move(client_channel)),
        client_thread_(std::move(client_thread)) {
    channel_ = Channel::Create(this, ConnectionParams(std::move(endpoint)),
                               Channel::HandlePolicy::kAcceptHandles,
                               std::move(task_runner));
    channel_->Start();
  }
  ~ChannelTestShutdownAndWriteDelegate() override { channel_->ShutDown(); }

  // Channel::Delegate implementation
  void OnChannelMessage(const void* payload,
                        size_t payload_size,
                        std::vector<PlatformHandle> handles) override {
    ++message_count_;

    // If |client_channel_| exists then close it and its thread.
    if (client_channel_) {
      // Write a fresh message, making our channel readable again.
      Channel::MessagePtr message = CreateDefaultMessage(false);
      client_thread_->task_runner()->PostTask(
          FROM_HERE, base::BindOnce(&Channel::Write, client_channel_,
                                    base::Passed(&message)));

      // Close the channel and wait for it to shutdown.
      client_channel_->ShutDown();
      client_channel_ = nullptr;

      client_thread_->Stop();
      client_thread_ = nullptr;
    }

    // Write a message to the channel, to verify whether this triggers an
    // OnChannelError callback before all messages were read.
    Channel::MessagePtr message = CreateDefaultMessage(false);
    channel_->Write(std::move(message));
  }

  void OnChannelError(Channel::Error error) override {
    EXPECT_EQ(2, message_count_);
    quit_closure_.Run();
  }

  base::RepeatingClosure quit_closure_;
  int message_count_ = 0;
  scoped_refptr<Channel> channel_;

  scoped_refptr<Channel> client_channel_;
  std::unique_ptr<base::Thread> client_thread_;
};

TEST(ChannelTest, PeerShutdownDuringRead) {
  base::MessageLoop message_loop(base::MessageLoop::TYPE_IO);
  PlatformChannel channel;

  // Create a "client" Channel with one end of the pipe, and Start() it.
  std::unique_ptr<base::Thread> client_thread =
      std::make_unique<base::Thread>("clientio_thread");
  client_thread->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  scoped_refptr<Channel> client_channel = Channel::Create(
      nullptr, ConnectionParams(channel.TakeRemoteEndpoint()),
      Channel::HandlePolicy::kAcceptHandles, client_thread->task_runner());
  client_channel->Start();

  // On the "client" IO thread, create and write a message.
  Channel::MessagePtr message = CreateDefaultMessage(false);
  client_thread->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&Channel::Write, client_channel, base::Passed(&message)));

  // Create a "server" Channel with the other end of the pipe, and process the
  // messages from it. The |server_delegate| will ShutDown the client end of
  // the pipe after the first message, and quit the RunLoop when OnChannelError
  // is received.
  base::RunLoop run_loop;
  ChannelTestShutdownAndWriteDelegate server_delegate(
      channel.TakeLocalEndpoint(), message_loop.task_runner(),
      std::move(client_channel), std::move(client_thread),
      run_loop.QuitClosure());

  run_loop.Run();
}

class RejectHandlesDelegate : public Channel::Delegate {
 public:
  RejectHandlesDelegate() = default;

  size_t num_messages() const { return num_messages_; }

  // Channel::Delegate:
  void OnChannelMessage(const void* payload,
                        size_t payload_size,
                        std::vector<PlatformHandle> handles) override {
    ++num_messages_;
  }

  void OnChannelError(Channel::Error error) override {
    if (wait_for_error_loop_)
      wait_for_error_loop_->Quit();
  }

  void WaitForError() {
    wait_for_error_loop_.emplace();
    wait_for_error_loop_->Run();
  }

 private:
  size_t num_messages_ = 0;
  base::Optional<base::RunLoop> wait_for_error_loop_;

  DISALLOW_COPY_AND_ASSIGN(RejectHandlesDelegate);
};

TEST(ChannelTest, RejectHandles) {
  base::MessageLoop message_loop(base::MessageLoop::TYPE_IO);
  PlatformChannel platform_channel;

  RejectHandlesDelegate receiver_delegate;
  scoped_refptr<Channel> receiver = Channel::Create(
      &receiver_delegate,
      ConnectionParams(platform_channel.TakeLocalEndpoint()),
      Channel::HandlePolicy::kRejectHandles, message_loop.task_runner());
  receiver->Start();

  RejectHandlesDelegate sender_delegate;
  scoped_refptr<Channel> sender = Channel::Create(
      &sender_delegate, ConnectionParams(platform_channel.TakeRemoteEndpoint()),
      Channel::HandlePolicy::kRejectHandles, message_loop.task_runner());
  sender->Start();

  // Create another platform channel just to stuff one of its endpoint handles
  // into a message. Sending this message to the receiver should cause the
  // receiver to reject it and close the Channel without ever dispatching the
  // message.
  PlatformChannel dummy_channel;
  std::vector<mojo::PlatformHandle> handles;
  handles.push_back(dummy_channel.TakeLocalEndpoint().TakePlatformHandle());
  auto message = std::make_unique<Channel::Message>(0 /* payload_size */,
                                                    1 /* max_handles */);
  message->SetHandles(std::move(handles));
  sender->Write(std::move(message));

  receiver_delegate.WaitForError();
  EXPECT_EQ(0u, receiver_delegate.num_messages());
}

TEST(ChannelTest, DeserializeMessage_BadExtraHeaderSize) {
  // Verifies that a message payload is rejected when the extra header chunk
  // size not properly aligned.
  constexpr uint16_t kBadAlignment = kChannelMessageAlignment + 1;
  constexpr uint16_t kTotalHeaderSize =
      sizeof(Channel::Message::Header) + kBadAlignment;
  constexpr uint32_t kEmptyPayloadSize = 8;
  constexpr uint32_t kMessageSize = kTotalHeaderSize + kEmptyPayloadSize;
  char message[kMessageSize];
  memset(message, 0, kMessageSize);

  Channel::Message::Header* header =
      reinterpret_cast<Channel::Message::Header*>(&message[0]);
  header->num_bytes = kMessageSize;
  header->num_header_bytes = kTotalHeaderSize;
  header->message_type = Channel::Message::MessageType::NORMAL;
  header->num_handles = 0;
  EXPECT_EQ(nullptr, Channel::Message::Deserialize(&message[0], kMessageSize,
                                                   base::kNullProcessHandle));
}

#if !defined(OS_WIN) && !defined(OS_MACOSX) && !defined(OS_FUCHSIA)
TEST(ChannelTest, DeserializeMessage_NonZeroExtraHeaderSize) {
  // Verifies that a message payload is rejected when the extra header chunk
  // size anything but zero on Linux, even if it's aligned.
  constexpr uint16_t kTotalHeaderSize =
      sizeof(Channel::Message::Header) + kChannelMessageAlignment;
  constexpr uint32_t kEmptyPayloadSize = 8;
  constexpr uint32_t kMessageSize = kTotalHeaderSize + kEmptyPayloadSize;
  char message[kMessageSize];
  memset(message, 0, kMessageSize);

  Channel::Message::Header* header =
      reinterpret_cast<Channel::Message::Header*>(&message[0]);
  header->num_bytes = kMessageSize;
  header->num_header_bytes = kTotalHeaderSize;
  header->message_type = Channel::Message::MessageType::NORMAL;
  header->num_handles = 0;
  EXPECT_EQ(nullptr, Channel::Message::Deserialize(&message[0], kMessageSize,
                                                   base::kNullProcessHandle));
}
#endif

class CountingChannelDelegate : public Channel::Delegate {
 public:
  explicit CountingChannelDelegate(base::OnceClosure on_final_message)
      : on_final_message_(std::move(on_final_message)) {}
  ~CountingChannelDelegate() override = default;

  void OnChannelMessage(const void* payload,
                        size_t payload_size,
                        std::vector<PlatformHandle> handles) override {
    // If this is the special "final message", run the closure.
    if (payload_size == 1) {
      auto* payload_str = reinterpret_cast<const char*>(payload);
      if (payload_str[0] == '!') {
        std::move(on_final_message_).Run();
        return;
      }
    }

    ++message_count_;
  }

  void OnChannelError(Channel::Error error) override { ++error_count_; }

  size_t message_count_ = 0;
  size_t error_count_ = 0;

 private:
  base::OnceClosure on_final_message_;
};

TEST(ChannelTest, PeerStressTest) {
  constexpr size_t kLotsOfMessages = 1024;

  base::MessageLoop message_loop(base::MessageLoop::TYPE_IO);
  base::RunLoop run_loop;

  // Both channels should receive all the messages that each is sent. When
  // the count becomes 2 (indicating both channels have received the final
  // message), quit the main test thread's run loop.
  std::atomic_int count_channels_received_final_message(0);
  auto quit_when_both_channels_received_final_message = base::BindRepeating(
      [](std::atomic_int* count_channels_received_final_message,
         base::OnceClosure quit_closure) {
        if (++(*count_channels_received_final_message) == 2) {
          std::move(quit_closure).Run();
        }
      },
      base::Unretained(&count_channels_received_final_message),
      run_loop.QuitClosure());

  // Create a second IO thread for the peer channel.
  base::Thread::Options thread_options;
  thread_options.message_loop_type = base::MessageLoop::TYPE_IO;
  base::Thread peer_thread("peer_b_io");
  peer_thread.StartWithOptions(thread_options);

  // Create two channels that run on separate threads.
  PlatformChannel platform_channel;

  CountingChannelDelegate delegate_a(
      quit_when_both_channels_received_final_message);
  scoped_refptr<Channel> channel_a = Channel::Create(
      &delegate_a, ConnectionParams(platform_channel.TakeLocalEndpoint()),
      Channel::HandlePolicy::kRejectHandles, message_loop.task_runner());

  CountingChannelDelegate delegate_b(
      quit_when_both_channels_received_final_message);
  scoped_refptr<Channel> channel_b = Channel::Create(
      &delegate_b, ConnectionParams(platform_channel.TakeRemoteEndpoint()),
      Channel::HandlePolicy::kRejectHandles, peer_thread.task_runner());

  // Send a lot of messages, followed by a final terminating message.
  auto send_lots_of_messages = [](scoped_refptr<Channel> channel) {
    for (size_t i = 0; i < kLotsOfMessages; ++i) {
      channel->Write(std::make_unique<Channel::Message>(0, 0));
    }
  };
  auto send_final_message = [](scoped_refptr<Channel> channel) {
    auto message = std::make_unique<Channel::Message>(1, 0);
    auto* payload = static_cast<char*>(message->mutable_payload());
    payload[0] = '!';
    channel->Write(std::move(message));
  };

  channel_a->Start();
  channel_b->Start();

  send_lots_of_messages(channel_a);
  send_lots_of_messages(channel_b);

  message_loop.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(send_lots_of_messages, channel_a));
  message_loop.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(send_lots_of_messages, channel_a));
  message_loop.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(send_final_message, channel_a));

  peer_thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(send_lots_of_messages, channel_b));
  peer_thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(send_lots_of_messages, channel_b));
  peer_thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(send_final_message, channel_b));

  // Run until quit_when_both_channels_received_final_message quits the loop.
  run_loop.Run();

  channel_a->ShutDown();
  channel_b->ShutDown();

  peer_thread.StopSoon();

  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(kLotsOfMessages * 3, delegate_a.message_count_);
  EXPECT_EQ(kLotsOfMessages * 3, delegate_b.message_count_);

  EXPECT_EQ(0u, delegate_a.error_count_);
  EXPECT_EQ(0u, delegate_b.error_count_);
}

}  // namespace
}  // namespace core
}  // namespace mojo
