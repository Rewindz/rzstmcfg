#include <vector>
#include <iostream>
#include <format>

#include "SteamAccInfo.hpp"


int main(void)
{
    auto accs = SteamAccInfo::GetAllAccounts();

    for(const auto &acc : accs)
    {
        std::cout << std::format("Username: {}\tID64: {}\tID3: {}\n", acc.uname, acc.id64, acc.id3);
    }

    return 0;
}