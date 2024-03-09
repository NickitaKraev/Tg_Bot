#include "client.h"

Client::Client(const std::string& key, std::string_view url, int timeout)
    : timeout_(timeout), api_(std::make_unique<telegram::Api>(key, url)) {
}

telegram::BotInformation Client::GetMe() {
    try {
        return api_->GetMe();
    } catch (...) {
        throw;
    }
}

void Client::SendMessage(int chat_id, const std::string& message, std::optional<int> message_id) {
    try {
        api_->SendMessage(chat_id, message, message_id);
    } catch (...) {
        throw;
    }
}

std::vector<telegram::Message> Client::GetUpdates(std::optional<int> offset) {
    try {
        return api_->GetUpdates(timeout_, offset);
    } catch (...) {
        throw;
    }
}

void Client::RunBot() {
    try {
        api_->RunBot(timeout_);
    } catch (const telegram::ClientError& ex) {
        sleep(timeout_);
        RunBot();
    } catch (...) {
        throw;
    }
}
