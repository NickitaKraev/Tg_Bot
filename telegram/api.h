#pragma once

#include <memory>
#include <string>
#include <exception>
#include <vector>
#include <optional>

namespace telegram {

struct Message {
    std::int64_t update_id;
    std::int64_t chat_id;
    std::string message;
    std::int64_t message_id;
};

struct BotInformation {
    BotInformation(std::int64_t id, bool is_bot, const std::string& first_name,
                   const std::string& username)
        : id{id}, is_bot{is_bot}, first_name{first_name}, username{username} {
    }

    std::int64_t id;
    bool is_bot;
    std::string first_name;
    std::string username;
};

struct ServerError : public std::exception {

    ServerError(const std::string& description);

    ServerError(std::string&& description);

    const char* what() const noexcept override;

    std::string description_;
};

struct ClientError : public std::exception {

    ClientError(const std::string& description, bool ok, std::uint16_t error_code);
    ClientError(const std::string& description);

    const char* what() const noexcept override;

    std::string description_;
    bool ok_;
    std::uint16_t error_code_;
};

struct CreaterSession;

struct CheckerHTTPError;

class Api {
public:
    Api(const std::string& key, std::string_view url);
    BotInformation GetMe();
    void SendMessage(int chat_id, const std::string& message,
                     std::optional<int> message_id = std::nullopt) const;
    std::vector<Message> GetUpdates(std::optional<int> timeout = std::nullopt,
                                    std::optional<int> offset = std::nullopt);
    void RunBot(int timeout = 0);

    ~Api();

private:
    std::string key_;
    std::string url_;

    class WorkerWithMessage;
    std::unique_ptr<WorkerWithMessage> worker_;

    std::string GetOffset();
    void SetOffset(int offset);
};

std::unique_ptr<Api> CreateApi(const std::string& key, std::string_view url);

}  // namespace telegram
