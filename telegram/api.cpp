#include "api.h"

#include <fstream>
#include <random>
#include <iostream>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/URI.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>

telegram::ServerError::ServerError(const std::string& description) : description_(description) {
}

telegram::ServerError::ServerError(std::string&& description)
    : description_(std::move(description)) {
}

const char* telegram::ServerError::what() const noexcept {
    if (!description_.empty()) {
        return description_.data();
    }
    return "Server Error";
}

telegram::ClientError::ClientError(const std::string& description, bool ok,
                                   std::uint16_t error_code)
    : description_{description}, ok_(ok), error_code_(error_code) {
}

telegram::ClientError::ClientError(const std::string& description) : description_{description} {
}

const char* telegram::ClientError::what() const noexcept {
    return description_.data();
}

struct telegram::CreaterSession {
    static std::unique_ptr<Poco::Net::HTTPClientSession> CreateSession(const Poco::URI& uri);
};

std::unique_ptr<Poco::Net::HTTPClientSession> telegram::CreaterSession::CreateSession(
    const Poco::URI& uri) {
    if (uri.getScheme() == "https") {
        return std::make_unique<Poco::Net::HTTPSClientSession>(uri.getHost(), uri.getPort());
    }

    return std::make_unique<Poco::Net::HTTPClientSession>(uri.getHost(), uri.getPort());
}

struct telegram::CheckerHTTPError {
    static void CheckHTTPError(const Poco::Net::HTTPResponse& response, std::istream& in);
};

void telegram::CheckerHTTPError::CheckHTTPError(const Poco::Net::HTTPResponse& response,
                                                std::istream& in) {
    if (response.getStatus() != Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK) {
        if (response.getStatus() >= 500) {
            throw ServerError(std::string(std::istreambuf_iterator<char>(in), {}));
        }
        if (response.getStatus() >= 400 && response.getStatus() < 500) {
            try {
                Poco::JSON::Parser parser;
                auto json = parser.parse(in);
                auto ptr = json.extract<Poco::JSON::Object::Ptr>();
                throw ClientError(ptr->getValue<std::string>("description"),
                                  ptr->getValue<bool>("ok"),
                                  ptr->getValue<std::uint16_t>("error_code"));
            } catch (const ClientError& ex) {
                throw;
            } catch (...) {
                throw ClientError(response.getReason());
            }
        }
        throw std::runtime_error{"Error"};
    }
}

class telegram::Api::WorkerWithMessage {
public:
    telegram::Message GetMessage(const Poco::SharedPtr<Poco::JSON::Array>& array_ptr, size_t index);
    void MessageProcessing(const telegram::Api& api, const telegram::Message& message);

    ~WorkerWithMessage() = default;
};

telegram::Message telegram::Api::WorkerWithMessage::GetMessage(
    const Poco::SharedPtr<Poco::JSON::Array>& array_ptr, size_t index) {
    telegram::Message result;
    try {
        auto update = array_ptr->getObject(index);
        result.update_id = update->getValue<std::int64_t>("update_id");
        auto message = update->getObject("message");
        result.message = message->getValue<std::string>("text");
        result.message_id = message->getValue<std::int64_t>("message_id");
        auto chat = message->getObject("chat");
        result.chat_id = chat->getValue<std::int64_t>("id");
    } catch (...) {
        throw;
    }

    return result;
}

void telegram::Api::WorkerWithMessage::MessageProcessing(const telegram::Api& api,
                                                         const telegram::Message& message) {
    try {
        if (message.message == "/random") {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, 100);
            api.SendMessage(message.chat_id, std::to_string(distrib(gen)), message.message_id);
        } else if (message.message == "/weather") {
            api.SendMessage(message.chat_id, "Winter Is Coming");
        } else if (message.message == "/styleguide") {
            api.SendMessage(
                message.chat_id,
                "- Lets do a quick code review\n- This little maneuver is gonna cost us 51 years");
        } else if (message.message == "/stop") {
            exit(0);
        } else if (message.message == "/crash") {
            std::abort();
        }
    } catch (...) {
        throw;
    }

    throw std::logic_error{"Error message"};
}

telegram::Api::~Api() = default;

telegram::Api::Api(const std::string& key, std::string_view url)
    : key_{key}, url_{url}, worker_{std::make_unique<WorkerWithMessage>()} {
}

telegram::BotInformation telegram::Api::GetMe() {
    Poco::URI uri{url_};
    uri.setPath("/bot" + key_ + "/getMe");
    Poco::Net::HTTPRequest request{"GET", uri.getPathAndQuery()};

    std::unique_ptr<Poco::Net::HTTPClientSession> session =
        telegram::CreaterSession::CreateSession(uri);

    session->sendRequest(request);
    Poco::Net::HTTPResponse response;
    auto& in = session->receiveResponse(response);

    try {
        telegram::CheckerHTTPError::CheckHTTPError(response, in);
    } catch (...) {
        throw;
    }

    Poco::JSON::Parser parser;
    auto json = parser.parse(in);
    auto ptr = json.extract<Poco::JSON::Object::Ptr>();
    auto result = ptr->getObject("result");
    return BotInformation(result->getValue<std::int64_t>("id"), result->getValue<bool>("is_bot"),
                          result->getValue<std::string>("first_name"),
                          result->getValue<std::string>("username"));
}

void telegram::Api::SendMessage(int chat_id, const std::string& message,
                                std::optional<int> message_id) const {
    Poco::URI uri{url_};
    uri.setPath("/bot" + key_ + "/sendMessage");

    Poco::Net::HTTPRequest request{"POST", uri.getPathAndQuery()};

    request.setContentType("application/json");
    Poco::JSON::Object obj;
    obj.set("chat_id", chat_id);
    obj.set("text", message);
    if (message_id.has_value()) {
        obj.set("reply_to_message_id", message_id.value());
    }

    Poco::JSON::Stringifier stringfier;
    std::stringstream stream;
    stringfier.stringify(obj, stream);
    auto mess = std::move(stream).str();
    request.setContentLength(mess.size());

    std::unique_ptr<Poco::Net::HTTPClientSession> session =
        telegram::CreaterSession::CreateSession(uri);

    auto& out = session->sendRequest(request);
    out << mess;

    Poco::Net::HTTPResponse response;
    auto& in = session->receiveResponse(response);

    try {
        telegram::CheckerHTTPError::CheckHTTPError(response, in);
    } catch (...) {
        throw;
    }
}

std::vector<telegram::Message> telegram::Api::GetUpdates(std::optional<int> timeout,
                                                         std::optional<int> offset) {
    Poco::URI uri{url_};
    uri.setPath("/bot" + key_ + "/getUpdates");

    if (timeout.has_value()) {
        uri.addQueryParameter("timeout", std::to_string(timeout.value()));
    }

    if (offset.has_value()) {
        uri.addQueryParameter("offset", std::to_string(offset.value()));
    }

    Poco::Net::HTTPRequest request{"GET", uri.getPathAndQuery()};

    std::unique_ptr<Poco::Net::HTTPClientSession> session =
        telegram::CreaterSession::CreateSession(uri);

    session->sendRequest(request);

    Poco::Net::HTTPResponse response;
    auto& in = session->receiveResponse(response);

    try {
        telegram::CheckerHTTPError::CheckHTTPError(response, in);
    } catch (...) {
        throw;
    }

    Poco::JSON::Parser parser;
    auto json = parser.parse(in);
    auto ptr = json.extract<Poco::JSON::Object::Ptr>();
    auto updates = ptr->getArray("result");

    std::vector<telegram::Message> messages;
    messages.reserve(updates->size());

    for (size_t i = 0; i < updates->size(); ++i) {
        try {
            messages.push_back(worker_->GetMessage(updates, i));
        } catch (...) {
            continue;
        }
    }

    return messages;
}

std::string telegram::Api::GetOffset() {
    std::ifstream in("offset.txt");
    std::string offset;
    getline(in, offset);
    in.close();
    return offset;
}

void telegram::Api::SetOffset(int offset) {
    std::ofstream out("offset.txt");
    out.clear();
    out << offset;
    out.close();
}

void telegram::Api::RunBot(int timeout) {
    std::string offset;
    std::vector<telegram::Message> updates;
    while (true) {
        offset = GetOffset();
        try {
            updates = offset.empty() ? GetUpdates(timeout) : GetUpdates(timeout, std::stoi(offset));
        } catch (...) {
            throw;
        }

        for (size_t i = 0; i < updates.size(); ++i) {
            SetOffset(updates[i].update_id + 1);
            try {
                worker_->MessageProcessing(*this, updates[i]);
            } catch (std::logic_error& ex) {
                continue;
            } catch (...) {
                throw;
            }
        }
    }
}

std::unique_ptr<telegram::Api> telegram::CreateApi(const std::string& key, std::string_view url) {
    return std::make_unique<telegram::Api>(key, url);
}
