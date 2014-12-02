#include <bandit/bandit.h>

#include "../scr/modules/rtjs/pubsub.h"

using namespace gcm::pubsub;
using namespace bandit;

go_bandit([](){
    describe("pubsub", [](){
        it("starts and stops", [](){
            PubSub<std::string, std::string> pubsub;
        });

        it("subscribes to channel", [](){
            PubSub<std::string, std::string> pubsub;
            AssertThat(pubsub.subscribe("test"), EqualsTo(0));
        });

        it("publishes message", [](){
            PubSub<std::string, std::string> pubsub;
            pubsub.publish("test", "hello");
        });

        it("receives published message", [](){
            PubSub<std::string, std::string> pubsub;
            auto id = pubsub.subscribe("test");

            pubsub.publish("test", "hello");

            AssertThat(pubsub.poll("test", id), EqualsTo("hello"));
        });
    });
});