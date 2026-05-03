#pragma once
#include <cstdlib>
#include <string>
#include <optional>
#include <filesystem>
#include <fstream>
#include <vector>
#include <charconv>

#include <ValveFileVDF/vdf_parser.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

constexpr uint64_t STEAM_MAGIC_NUMBER = 76561197960265728;

std::optional<std::filesystem::path> getSteamPath()
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
        return res;
    }

    std::string id64, id3, uname;
    std::filesystem::path userdataPath;     

private:
    

    SteamAccInfo(const std::string &_id64, const std::string &_uname)
        : id64(_id64), uname(_uname)
    {
        uint64_t tmp = 0;
        auto [ptr, ec] = std::from_chars(id64.data(), id64.data() + id64.size(), tmp);
        id3 = std::to_string(tmp - STEAM_MAGIC_NUMBER);

        auto steamPath = getSteamPath();
        if(!steamPath)
            abort();
        userdataPath = steamPath.value() / "userdata" / id3;

    }

};