#pragma once
#include <map>
#include <vector>
#include <string>
#include <functional>

enum class GameNames
{
    COUNTER_STRIKE,
    TEST1,
    TEST2
};

struct GameInfo
{
    std::string id;
    std::string name;
};

inline const std::map<GameNames, GameInfo> GAME_IDS = 
{
    {GameNames::COUNTER_STRIKE, {"730", "Counter-Strike 2"}},
    {GameNames::TEST1, {"test", "Test 1"}},
    {GameNames::TEST2, {"test", "Test 2"}}
};

using GameInfoList = std::vector<std::reference_wrapper<const GameInfo>>;

inline const GameInfoList& getAllGameInfos()
{
    static GameInfoList res = []() -> GameInfoList {
        GameInfoList res;
        for(const auto& [gamename, info] : GAME_IDS){
            res.push_back(info);
        }
        return res;
    }();
    return res;
}

inline const char* getGameNameCStr(const GameInfo& gi)
{
    return gi.name.c_str();
}