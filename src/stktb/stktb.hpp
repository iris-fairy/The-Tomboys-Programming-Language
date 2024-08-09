#pragma once
#include <variant>
#include <string>
#include <cstdint>
#include <stack>
#include <unordered_map>
#include <boost/function.hpp>
#include <boost/dll/shared_library.hpp>

namespace stktb
{
    enum Token
    {
        PUSH,
        LOAD,
        STORE,
        ERASE,
        DEF,
        ENDDEF,
        CALL,
        IMPORT,
        USING,
        RUN,
        IF_TRUE,
        IF_FALSE,
        ENDIF,
        GOTO,
        AND,
        OR,
        GT,
        LT,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        DEBUG
    };

    using token_t = std::variant<Token, std::string, std::int64_t, double>;

    struct envs
    {
        std::stack<token_t> stack;
        std::unordered_map<std::string, token_t> vars;
        std::unordered_map<std::string, std::size_t> subs;
        std::unordered_map<std::string, boost::function<void(envs&, std::size_t)>> user_funcs;
        std::stack<bool> inblock;
        bool insub = false;
        std::size_t current = 0;
        std::unordered_map<std::string, boost::dll::shared_library> mods;
    };
}
