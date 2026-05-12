#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <charconv>
#include <format>

#include "SteamAccInfo.hpp"

struct Borrow
{
    std::string gameId, borrower, borrowee, borrowId;

    Borrow() = default;

    Borrow(const std::string& _gameId, const std::string& _borrower, const std::string& _borrowee)
        : gameId(_gameId), borrower(_borrower), borrowee(_borrowee)
    {
        borrowId = MakeBorrowId(gameId, borrower, borrowee);
    }

    static std::string MakeBorrowId(const std::string& _gameId, const std::string& _borrower, const std::string& _borrowee)
    {
        auto [first, second] = std::minmax(_borrower, _borrowee);
        return std::string(_gameId + first.substr(first.length() - 5) + second.substr(second.length() - 5));

    }

    bool operator==(const Borrow &b) const = default;

    bool checkEqual(const std::string& _gameId, const std::string& _borrower, const std::string& _borrowee) const
    {
        return (gameId == _gameId) && 
        (borrower == _borrower || borrower == _borrowee) && 
        (borrowee == _borrowee || borrowee == _borrower);
    }

    bool checkEqual(const std::string& _borrowId) const
    {
        return borrowId == _borrowId;
    }

};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Borrow, gameId, borrower, borrowee, borrowId);

enum BorrowStatus
{
    BORROW_OK = 0,
    BORROW_FAIL,
    BORROW_ALR_EXISTS,
    BORROW_NOGAME
};

enum ReturnStatus
{
    RETURN_OK = 0,
    RETURN_FAIL,
};

class Borrower
{
public:

    Borrower(const Borrower&) = delete;
    Borrower& operator=(const Borrower&) = delete;

    Borrower(Borrower&&) = delete;
    Borrower& operator=(Borrower&&) = delete;

    inline static Borrower& GetBorrower()
    {
        static Borrower instance;
        return instance;
    }

    inline BorrowStatus borrow(const std::string& gameId, const SteamAccInfo& borrower, const SteamAccInfo& borrowee)
    {
        if(!std::any_of(borrows.begin(), borrows.end(), [&gameId, &borrower](const Borrow& b){
            return (gameId == b.gameId) && (borrower.id64 == b.borrower); 
        })){
            // No current borrows like this exist. Create borrow
            if(borrower.hasGameDataDir(gameId) && borrowee.hasGameDataDir(gameId)){
                auto& borrow = borrows.emplace_back(gameId, borrower.id64, borrowee.id64);
                std::filesystem::path borrowPath = borrowsDir / borrow.borrowId;
                
                std::error_code ec;
                std::filesystem::create_directory(borrowPath, ec);
                if (ec) {
                    borrows.pop_back();
                    return BORROW_FAIL;
                }
                
                auto borrowerGameData = borrower.getUserGamePath(gameId); 
                std::filesystem::copy(borrowerGameData, borrowPath,
                    std::filesystem::copy_options::recursive, ec);
                if (ec) {
                    borrows.pop_back();
                    return BORROW_FAIL;
                }

                // Clean target directory before copy to ensure no foreign files remain
                std::filesystem::remove_all(borrowerGameData, ec);

                std::filesystem::copy(borrowee.getUserGamePath(gameId), borrowerGameData,
                    std::filesystem::copy_options::recursive, ec);
                if (ec) {
                    borrows.pop_back();
                    return BORROW_FAIL;
                }
                
                save();
                return BORROW_OK;
            } else {
                return BORROW_NOGAME;
            }
        }

        return BORROW_ALR_EXISTS;
    }

    inline ReturnStatus returnBorrow(const Borrow& borrow)
    {
        const auto& opt_borrowerAcc = SteamAccInfo::GetAccFrom64(borrow.borrower);
        
        if(!opt_borrowerAcc.has_value())
            return RETURN_FAIL;

        const auto& borrowerAcc = opt_borrowerAcc.value().get();

        auto userDataPath = borrowerAcc.getUserGamePath(borrow.gameId);
        auto borrowPath = borrowsDir / borrow.borrowId;

        std::error_code ec;
        if (std::filesystem::exists(userDataPath)) {
            std::filesystem::remove_all(userDataPath, ec);
            if (ec) return RETURN_FAIL;
        }

        std::filesystem::copy(borrowPath, userDataPath, 
            std::filesystem::copy_options::recursive, ec);
        if (ec) return RETURN_FAIL;

        std::filesystem::remove_all(borrowPath, ec);
        
        std::erase(borrows, borrow);
        save();
        
        return RETURN_OK;
    }

    inline const std::vector<Borrow>& getBorrows() const
    {
        return borrows;
    }

private:

    ~Borrower()
    {
        save();
    }

    Borrower()
    {
        if(!std::filesystem::exists(borrowsDir)){
            std::filesystem::create_directory(borrowsDir);
        }
        else if(!std::filesystem::is_directory(borrowsDir)){
            abort();
        }

        try{
            if(!std::filesystem::exists(borrowCfgPath)){
                std::ofstream touch(borrowCfgPath, std::ios::app);
                touch << R"({ "borrows": [] })";
                touch.close();
            } 
            else if(!std::filesystem::is_regular_file(borrowCfgPath)) {
                abort();
            }
            
            std::ifstream borrowFile(borrowCfgPath);
            if(borrowFile.is_open()){
                nlohmann::json j;
                borrowFile >> j;
                if(j["borrows"].is_array()){
                    j["borrows"].get_to(borrows);
                }
            }
        } catch (const nlohmann::json::exception& e) {
            std::cerr << std::format("Failed to parse borrow.json\n{}\n", e.what());
        }
    }

    void save()
    {
        std::ofstream borrowFile(borrowCfgPath);
        if(borrowFile.is_open()){
            nlohmann::json j;
            j["borrows"] = borrows;
            borrowFile << j.dump(4);
        }
    }
    
    const std::filesystem::path borrowsDir = "borrows";
    const std::filesystem::path borrowCfgPath = borrowsDir / "borrow.json";
    std::vector<Borrow> borrows;
};