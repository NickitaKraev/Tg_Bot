#include "api.h"

class Client {
public:
    Client(const std::string& key, std::string_view url, int timeout = 10);

    telegram::BotInformation GetMe();
    void SendMessage(int chat_id, const std::string& message,
                     std::optional<int> message_id = std::nullopt);
    std::vector<telegram::Message> GetUpdates(std::optional<int> offset = std::nullopt);
    void RunBot();

private:
    int timeout_;
    std::unique_ptr<telegram::Api> api_;
};