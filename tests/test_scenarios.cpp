#include <tests/test_scenarios.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <telegram/client.h>

using Catch::Matchers::Message;

void TestSingleGetMe(std::string_view url) {
    auto api = telegram::CreateApi("123", url);
    auto me = api->GetMe();
    REQUIRE(me.id == 1234567);
    REQUIRE(me.is_bot);
    REQUIRE(me.username == "test_bot");
    REQUIRE(me.first_name == "Test Bot");
}

void TestGetMeErrorHandling(std::string_view url) {
    auto api = telegram::CreateApi("123", url);
    REQUIRE_THROWS_MATCHES(api->GetMe(), telegram::ServerError, Message("Internal server error"));

    try {
        api->GetMe();
        FAIL("No throw");
    } catch (const telegram::ClientError& ex) {
        REQUIRE(ex.description_ == "Unauthorized");
        REQUIRE_FALSE(ex.ok_);
        REQUIRE(ex.error_code_ == 401);
    } catch (...) {
        FAIL("Undefined type exception");
    }
}

void TestSingleGetUpdatesAndSendMessages(std::string_view url) {
    auto api = telegram::CreateApi("123", url);
    auto updates = api->GetUpdates();
    REQUIRE(updates.size() == 3);

    REQUIRE(updates[0].update_id == 851793506);
    REQUIRE(updates[0].chat_id == 104519755);
    REQUIRE(updates[0].message == "/start");
    REQUIRE(updates[0].message_id == 1);

    REQUIRE(updates[1].update_id == 851793507);
    REQUIRE(updates[1].chat_id == 104519755);
    REQUIRE(updates[1].message == "/end");
    REQUIRE(updates[1].message_id == 2);

    REQUIRE(updates[2].update_id == 851793508);
    REQUIRE(updates[2].chat_id == -274574250);
    REQUIRE(updates[2].message == "/1234");
    REQUIRE(updates[2].message_id == 11);

    REQUIRE_NOTHROW(api->SendMessage(updates[0].chat_id, "Hi!"));
    REQUIRE_NOTHROW(api->SendMessage(updates[1].chat_id, "Reply", updates[1].message_id));
    REQUIRE_NOTHROW(api->SendMessage(updates[1].chat_id, "Reply", updates[1].message_id));
}

void TestHandleGetUpdatesOffset(std::string_view url) {
    auto api = telegram::CreateApi("123", url);
    int timeout = 5;

    auto updates = api->GetUpdates(timeout);
    REQUIRE(updates.size() == 2);

    REQUIRE(updates[0].update_id == 851793506);
    REQUIRE(updates[0].chat_id == 104519755);
    REQUIRE(updates[0].message == "/start");
    REQUIRE(updates[0].message_id == 1);

    REQUIRE(updates[1].update_id == 851793507);
    REQUIRE(updates[1].chat_id == 104519755);
    REQUIRE(updates[1].message == "/end");
    REQUIRE(updates[1].message_id == 2);

    int max_update_id = updates[1].update_id + 1;

    updates = api->GetUpdates(timeout, max_update_id);
    REQUIRE(updates.empty());

    updates = api->GetUpdates(timeout, max_update_id);
    REQUIRE(updates.size() == 1);

    REQUIRE(updates[0].update_id == 851793508);
    REQUIRE(updates[0].chat_id == -274574250);
    REQUIRE(updates[0].message == "/1234");
    REQUIRE(updates[0].message_id == 11);
}
