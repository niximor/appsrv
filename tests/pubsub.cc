#include <bandit/bandit.h>

#include "../src/modules/rtjs/pubsub.h"

using namespace gcm::pubsub;
using namespace bandit;

go_bandit([](){
    describe("pubsub", [](){
        it("starts and stops", [](){
            PubSub<std::string, std::string> pubsub;
        });

        it("subscribes to channel", [](){
            PubSub<std::string, std::string> pubsub;
            AssertThat(pubsub.subscribe("test"), Equals(1));
        });

        it("publishes message", [](){
            PubSub<std::string, std::string> pubsub;
            pubsub.publish("test", "hello");
        });

        it("receives published message", [](){
            const std::string channel_name{"test"};
            const std::string test_message{"hello"};

            PubSub<std::string, std::string> pubsub;
            auto id = pubsub.subscribe(channel_name);

            pubsub.publish(channel_name, test_message);

            auto res = pubsub.poll(channel_name, id);

            AssertThat(res.size(), Equals(1));
            AssertThat(res[0], Equals(test_message));
        });

        it("waits for published message", [](){
            const std::string channel_name{"test"};
            const std::string test_message{"abc"};

            PubSub<std::string, std::string> pubsub;
            auto id = pubsub.subscribe(channel_name);

            auto future = std::async(std::launch::async, [&](){
                // Need sleep here, to give poll time to start waiting.
                // TODO: Would be great to solve this properly.

                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                pubsub.publish(channel_name, test_message);
                return true;
            });

            // Wait for most 20 seconds for message.
            auto res = pubsub.poll(channel_name, id, std::chrono::seconds(20));

            future.get();

            AssertThat(res.size(), Equals(1));
            AssertThat(res[0], Equals(test_message));
        });

        it("throws on unknown session", [](){
            PubSub<std::string, std::string> pubsub;

            AssertThrows(SessionNotFound, pubsub.poll("test", 12345));
        });
    });
});