#include <iostream>
#include <regex>
#include <string>
#include "httplib.h"
#include "alien_data.h"
#include "translator.h"


AlienData send(httplib::Client& client, const std::string& serverUrl, const AlienData& data) {
	auto encoded = encodeAlien(data);
	std::cout << "sending " << data.to_string() << ' ' << encoded << std::endl;
	const std::shared_ptr<httplib::Response> serverResponse = 
		client.Post((serverUrl + "/aliens/send").c_str(), encoded.c_str(), "text/plain");

	if (!serverResponse) {
		std::cout << "Unexpected server response:\nNo response from server" << std::endl;
		return 1;
	}
	
	if (serverResponse->status != 200) {
		std::cout << "Unexpected server response:\nHTTP code: " << serverResponse->status
		          << "\nResponse body: " << serverResponse->body << std::endl;
		return 2;
	}

	std::cout << "Server response: " << serverResponse->body << std::endl;
    return decodeAlien(serverResponse->body);
}


AlienData makeJoinRequest(int64_t playerKey) {
	auto requestTypeData = 2;
	auto unknownVec = std::vector<AlienData>();
	return std::vector<AlienData>({requestTypeData, playerKey, unknownVec});
}


AlienData makeStartRequest(int64_t playerKey, const AlienData& gameResponse) {
	auto requestTypeData = 3;
	auto shipParams = std::vector<AlienData>({2, 3, 4, 5});
	return std::vector<AlienData>({requestTypeData, playerKey, shipParams});
}

AlienData makeMoveCommand(int64_t id, AlienData const& move) {
    return std::vector<AlienData>({0, id, move});
}

AlienData makeDestructCommand(int64_t id) {
    return std::vector<AlienData>({1, id});
}

AlienData makeShootCommand(int64_t id, AlienData const& position, int64_t power) {
    return std::vector<AlienData>({2, id, position, power});
}

AlienData makeCommandsRequest(int64_t playerKey, const std::vector<AlienData>& commandList) {
    auto requestTypeData = 4;
    return std::vector<AlienData>({requestTypeData, playerKey, commandList});
}

int signum(int x) {
    if (x == 0) {
        return 0;
    }
    return x > 0 ? 1 : -1;
}

std::pair<int, int> get_gravity(std::pair<int, int> position) {
    int x = position.first;
    int y = position.second;

    int gx = 0;
    int gy = 0;

    // separate ifs because if x == y gravity works in both directions
    // same for x == -y
    if (std::abs(x) >= std::abs(y)) {
        gx = -signum(x);
    }

    if (std::abs(x) <= std::abs(y)) {
        gy = -signum(y);
    }

    return {gx, gy};
}

// what position and speed we will get if now we have position and speed and send move command
std::pair<std::pair<int, int>, std::pair<int, int>> predict_movement(std::pair<int, int> position, std::pair<int, int> speed, std::pair<int, int> moveCommand) {
    auto gravity = get_gravity(position);

    std::pair newSpeed{speed.first + gravity.first - moveCommand.first,
                       speed.second + gravity.second - moveCommand.second};
    std::pair newPos{position.first + newSpeed.first, position.second + newSpeed.second};

    return {newPos, newSpeed};
}

std::vector<std::pair<int, int>> calculateOrbit(std::pair<int, int> position, std::pair<int, int> speed) {
    std::vector<std::pair<int, int>> result(100);

    for (int i = 0; i < 50; i++) {
        auto tmp = predict_movement(position, speed, {0, 0});
        position = tmp.first;
        speed = tmp.second;
        result[i] = position;
    }

    return result;
}

std::vector<AlienData> runStrategy(const AlienData& gameInfo) {
    std::vector<AlienData> commands;
    std::vector<AlienData> myShips; //todo

    for (auto ship : myShips) {
        int shipid{}; // todo
        int planetSize{}; //todo;
        std::pair<int, int> pos{}; //todo
        std::pair<int, int> speed{}; //todo

        bool safeOrbit = true;
        auto currentOrbit = calculateOrbit(pos, speed);
        for (auto p: currentOrbit) {
            if (std::abs(p.first) <= planetSize && std::abs(p.second) <= planetSize) {
                safeOrbit = false;
                break;
            }
        }

        if (!safeOrbit) {
            auto gravity = get_gravity(pos);
            commands.push_back(makeMoveCommand(shipid, VectorPair<AlienData>(gravity.first + gravity.second, gravity.second - gravity.first)));
        }
    }

    return commands;
}

int main(int argc, char* argv[]) {
	const std::string serverUrl(argv[1]);
	const int64_t playerKey(std::stoll(argv[2]));

	std::cout << "ServerUrl: " << serverUrl << "; PlayerKey: " << playerKey << std::endl;
	
	const std::regex urlRegexp("http://(.+):(\\d+)");
	std::smatch urlMatches;
	if (!std::regex_search(serverUrl, urlMatches, urlRegexp) || urlMatches.size() != 3) {
		std::cout << "Unexpected server response:\nBad server URL" << std::endl;
		return 1;
	}
	const std::string serverName = urlMatches[1];
	const int serverPort = std::stoi(urlMatches[2]);
	httplib::Client client(serverName, serverPort);

	auto joinRequest = makeJoinRequest(playerKey);
    auto gameResponse = send(client, serverUrl, joinRequest);
    std::cout << gameResponse.to_string() << std::endl;
    auto startRequest = makeStartRequest(playerKey, gameResponse);
    gameResponse = send(client, serverUrl, startRequest);

    while (gameResponse.getVector()[1].getNumber() != 2) {
    	std::cout << gameResponse.to_string() << std::endl;
    	auto commands = runStrategy(gameResponse);
        auto commandsRequest = makeCommandsRequest(playerKey, commands);
        gameResponse = send(client, serverUrl, commandsRequest);
	}
    
    std::cout << gameResponse.to_string() << std::endl;
	
	return 0;
}

