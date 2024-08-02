#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <cstdint>
#include <optional>
#include <stack>
#include <functional>
#include <boost/dll/shared_library.hpp>
#include <boost/function.hpp>

namespace tomboys
{
    enum class Token
    {
        TRUE,
        FALSE
    };

    struct envs;

    using ft = void(envs&, unsigned int);
    using token_t = std::variant<Token, std::string, std::int64_t, double>;
    using op_t = std::function<void(envs&, token_t& l, token_t& r)>;

    struct envs
    {
        std::vector<std::variant<Token, std::string, std::int64_t, double>> stack;
        std::map<std::string, std::variant<Token, std::string, std::int64_t, double>> vars;
        std::map<std::string, unsigned int> subroutines;
        std::map<std::string, boost::function<ft>> func;
        unsigned int current = 0;
        std::stack<bool> intrueblock;
        bool infunc;
    };

    [[noreturn]]
    inline void inputerr(unsigned int line, std::string place, std::string reason = "")
    {
        std::cerr << "Invalid input: line-" << line-1 << " " << place << " (" << reason << ")\n";
        std::exit(EXIT_FAILURE);
    }

    void fdebug(std::vector<std::string>& stack, token_t& e, unsigned int ln)
    {
        if (std::holds_alternative<Token>(e))
        {
            Token token = std::get<Token>(e);
            if (token == Token::TRUE)
            {
                stack.push_back("TRUE");
            }
            else if (token == Token::FALSE)
            {
                stack.push_back("FALSE");
            }
            else
            {
                inputerr(ln, "DEBUG", "Unknown token type.");
            }
        }   
        else if (std::holds_alternative<std::int64_t>(e))
        {
            stack.push_back(std::to_string(std::get<std::int64_t>(e)));
        }
        else if (std::holds_alternative<double>(e))
        {
            stack.push_back(std::to_string(std::get<double>(e)));
        }
        else
        {
            stack.push_back(std::get<std::string>(e));
        }
    }

    void opdo(unsigned int ln, envs& env, op_t&& fii, op_t&& fdd, op_t&& fdi, op_t&& fid)
    {
        token_t l, r;
        try
        {
            r = env.stack.back();
            l = env.stack.at(env.stack.size()-2);
        }
        catch(const std::exception& e)
        {
            inputerr(ln, "op", e.what());
        }

        if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
        {
            env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
            fii(env, l, r);
        }
        else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
        {
            env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
            fdd(env, l, r);
        }
        else if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<double>(r))
        {
            env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
            fid(env, l, r);              
        }
        else if (std::holds_alternative<double>(l) && std::holds_alternative<std::int64_t>(r))
        {
            env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
            fdi(env, l, r);                
        }
        else
        {
            inputerr(ln, "op", "Invalied value type of operand.");
        }
    }

    bool distbool(token_t& t)
    {
        if (std::holds_alternative<Token>(t))
        {
            if (std::get<Token>(t) == Token::TRUE)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else if (std::holds_alternative<std::string>(t))
        {
            if (std::get<std::string>(t) != "")
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else if (std::holds_alternative<double>(t))
        {
            if (std::get<double>(t))
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else if (std::holds_alternative<std::int64_t>(t))
        {
            if (std::get<std::int64_t>(t))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        return false;
    }
}
