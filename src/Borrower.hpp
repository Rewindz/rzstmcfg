#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

struct Borrow
{
    std::string gameId, borrower, borrowee;
};


class Borrower
{
public:

    inline static Borrower& GetBorrower()
    {
        static Borrower res = []() -> Borrower {
            return Borrower();
        }();
        return res;
    }

private:
    Borrower()
    {
        try{
            std::filesystem::path path = "borrow.json";
            if(!std::filesystem::exists(path)){

            } 
            else if(!std::filesystem::is_regular_file(path)) {
                abort();
            }
        } catch (const nlohmann::json::exception& e) {
            std::cerr << std::format("Failed to parse borrow.json\n{}\n", e.what());
        }
    }

    std::vector<Borrow> borrows;
};