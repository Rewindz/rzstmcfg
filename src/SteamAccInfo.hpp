#pragma once
#include <cstdlib>
#include <string>
#include <optional>
#include <filesystem>
#include <fstream>
#include <vector>
#include <charconv>
#include <iostream>
#include <format>
#include <algorithm>
#include <thread>
#include <chrono>
#include <atomic>

#include <nlohmann/json.hpp>
#include <ValveFileVDF/vdf_parser.hpp>

#define CPPHTTPLIB_MBEDTLS_SUPPORT
#include <httplib.h>

#include "Games.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

constexpr uint64_t STEAM_MAGIC_NUMBER = 76561197960265728;

inline std::optional<std::filesystem::path> getSteamPath()
{
    static std::optional<std::filesystem::path> path = []() -> std::optional<std::filesystem::path> {
        #ifdef __linux__
            const char *homePath = std::getenv("HOME");
            if(!homePath)
                return std::nullopt;
            return std::filesystem::path(homePath) / ".steam" / "steam";
        #endif
        
        #ifdef  _WIN32
            char steamPath[MAX_PATH];
            DWORD bufferSize = sizeof(steamPath);
            LSTATUS status = RegGetValueA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", RRF_RT_REG_SZ, nullptr, steamPath, &bufferSize);
            if (status == ERROR_SUCCESS) {
                return std::filesystem::path(steamPath);
            }
            return std::nullopt;
        #endif
    }();
    return path;
}

inline std::string id3to64(const std::string& id3)
{
    uint64_t tmp = 0;
    std::from_chars(id3.data(), id3.data() + id3.size(), tmp);
    return std::to_string(tmp + STEAM_MAGIC_NUMBER);
}

inline std::string id64to3(const std::string& id64)
{
    uint64_t tmp = 0;
    std::from_chars(id64.data(), id64.data() + id64.size(), tmp);
    return std::to_string(tmp - STEAM_MAGIC_NUMBER);   
}

/* accs.json

    {
        accs: [
            {
                id64: "nnnnnn",
                name: "nnnnn"
            },
            {
                id64: "nnnnnnn",
                name: "nnnnnn"
            }
        ]
    }

*/

struct KnownAccs
{
    std::string id64;
    std::string name;

    KnownAccs() = default;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(KnownAccs, id64, name);



class SteamAccInfo
{
public:

    static std::vector<SteamAccInfo>& GetAllAccounts(std::atomic<float>* progress = nullptr)
    {
        static std::vector<SteamAccInfo> res;

        if(!res.empty()){
            if(progress) progress->store(1.0f);
            return res;
        }

        auto steamPath = getSteamPath();
        if(!steamPath)
            return res;

        std::map<std::string, std::string> knownAccs;
        if(std::filesystem::exists("accs.json")){
            std::ifstream file("accs.json");
            if(file.is_open()){
                try{
                    nlohmann::json j;
                    file >> j;
                    if(j["accs"].is_array()){
                        for(const KnownAccs acc : j["accs"]){
                            knownAccs.emplace(acc.id64, acc.name);
                        }
                    }
                } catch (const nlohmann::json::exception& e) {
                    std::cerr << "Failed to parse accs.json\n" << e.what() << std::endl;
                }
            }
        }

        auto loginUsersPath = steamPath.value() / "config" / "loginusers.vdf"; 

        std::ifstream loginUsers(loginUsersPath);
        if(loginUsers.is_open()){
            auto root = tyti::vdf::read(loginUsers);
            loginUsers.close();
            for(const auto& [id64, child] : root.childs)
            {
                const auto& uname = child->attribs["PersonaName"];
                res.push_back(SteamAccInfo(id64, uname));
                knownAccs[id64] = uname;
            }
        }

        auto userdataPath = steamPath.value() / "userdata";
        std::vector<std::filesystem::path> userDirs;
        for(const auto& dir : std::filesystem::directory_iterator(userdataPath)){
            if(dir.is_directory())
                userDirs.push_back(dir);
        }

        float totalItems = float(userDirs.size());
        float currentItem = 0.0f;

        httplib::Client cli("https://steamcommunity.com");
        cli.set_keep_alive(true);
        for(const auto& dir : userDirs){
            auto id3 = dir.filename().string();
            auto id64 = id3to64(id3);
            if(!std::any_of(res.begin(), res.end(), [&id64](const SteamAccInfo& acc){
                return id64 == acc.id64;
            })){
                if(knownAccs.contains(id64)){
                    res.push_back(SteamAccInfo(id64, knownAccs.at(id64)));
                } else {
                    auto response = cli.Get(std::format("/profiles/{}", id64));
                    if(response && response->status == httplib::StatusCode::OK_200){
                        // <span class="actual_persona_name">uname</span>
                        std::string html = response->body;
                        std::string startTag = R"(<span class="actual_persona_name">)";
                        std::string endTag = "</span>";
                        bool fail = false;
                        size_t startPos = html.find(startTag);
                        if(startPos != std::string::npos){
                            startPos += startTag.length();
                            size_t endPos = html.find(endTag, startPos);
                            if(endPos != std::string::npos){
                                std::string uname = html.substr(startPos, endPos - startPos);
                                res.push_back(SteamAccInfo(id64, uname));
                                knownAccs[id64] = uname;
                            } else fail = true;
                        } else fail = true;
                        if(fail)
                            res.push_back(SteamAccInfo(id64, std::format("Unknown {}", id3.substr(id3.length() - 4))));
                    } else {
                        res.push_back(SteamAccInfo(id64, std::format("Unknown {}", id3.substr(id3.length() - 4))));
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                }
            }
            currentItem += 1.0f;
            if(progress){
                progress->store(currentItem / totalItems, std::memory_order_relaxed);
            }
        }

        std::ofstream accsFile("accs.json");
        if(accsFile.is_open()){
            nlohmann::json root = nlohmann::json::object();
            nlohmann::json array = nlohmann::json::array(); 
            for(const auto& [id64, uname] : knownAccs){
                nlohmann::json obj = nlohmann::json::object();
                obj["id64"] = id64;
                obj["name"] = uname;
                array.push_back(obj);
            }
            root["accs"] = array;
            accsFile << root.dump(4);
        }

        if(progress) progress->store(1.0f);
        return res;
    }

    inline static std::optional<std::reference_wrapper<const SteamAccInfo>> GetAccFrom64(const std::string& _id64)
    {
        auto& accs = GetAllAccounts();
        auto res = std::ranges::find_if(accs, [&_id64](const SteamAccInfo& acc){
            return acc.id64 == _id64;
        });
        if(res == accs.end())
            return std::nullopt;
        return *res;
    } 

    bool hasGameDataDir(const std::string& gameId) const
    {
        auto gameDataPath = userdataPath / gameId;
        return (std::filesystem::exists(gameDataPath) && std::filesystem::is_directory(gameDataPath));
    }

    std::filesystem::path getUserGamePath(const std::string& gameId) const 
    {
        return userdataPath / gameId;
    }

    std::string id64, id3, uname;
    std::filesystem::path userdataPath;     

private:
    

    SteamAccInfo(const std::string &_id64, const std::string &_uname)
        : id64(_id64), uname(_uname)
    {
        id3 = id64to3(id64);

        auto steamPath = getSteamPath();
        if(!steamPath)
            abort();
        userdataPath = steamPath.value() / "userdata" / id3;

        if(!std::filesystem::exists(userdataPath) || !std::filesystem::is_directory(userdataPath))
            std::cerr << std::format("Userdata path for user: {}, does not exist or isn't a directory!\n", uname);

    }

};