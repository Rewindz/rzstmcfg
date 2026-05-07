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

#include <ValveFileVDF/vdf_parser.hpp>

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



class SteamAccInfo
{
public:

    static std::vector<SteamAccInfo> GetAllAccounts()
    {
        std::vector<SteamAccInfo> res;
        auto steamPath = getSteamPath();
        if(!steamPath)
            return res;

        auto loginUsersPath = steamPath.value() / "config" / "loginusers.vdf"; 

        std::ifstream loginUsers(loginUsersPath);
        if(loginUsers.is_open()){
            auto root = tyti::vdf::read(loginUsers);
            loginUsers.close();
            for(const auto& [id64, child] : root.childs)
            {
                res.push_back(SteamAccInfo(id64, child->attribs["PersonaName"]));
            }
        } 

        auto userdataPath = steamPath.value() / "userdata";
        for(const auto &dir : std::filesystem::directory_iterator(userdataPath)){
            if(dir.is_directory()){
                auto id3 = dir.path().filename().string();
                auto id64 = id3to64(id3);
                if(!std::any_of(res.begin(), res.end(), [&id64](const SteamAccInfo& acc){
                    return id64 == acc.id64;
                })){
                    // Probably webscrape for the username when it is unknown (unfortunate, but the API is too much work for this)
                    res.push_back(SteamAccInfo(id64, std::format("Unknown {}", id64.substr(id64.length() - 4))));
                }
            }
        }

        return res;
    }

    bool hasGameDataDir(const std::string& gameId) const
    {
        auto gameDataPath = userdataPath / gameId;
        return (std::filesystem::exists(gameDataPath) && std::filesystem::is_directory(gameDataPath));
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