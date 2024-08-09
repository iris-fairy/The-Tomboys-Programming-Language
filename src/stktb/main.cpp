#include "stktb.hpp"
#include "lib.hpp"
#include <cstdint>
#include <ostream>
#include <cstdlib>
#include <list>
#include <string>
#include <regex>
#include <vector>
#include <cmath>
#include <boost/dll/shared_library.hpp>
#include <boost/function.hpp>

using stktb::Token;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Invalid command argument.\n";
        return EXIT_FAILURE;
    }

    auto src = stktb::getvalue<std::vector<std::uint8_t>>(argv[1]);
    auto constants = stktb::getvalue<std::list<std::string>>(argv[2]);

    stktb::envs env;
    env.inblock.push(true);
    std::regex str("\".*\""), intre("[+-]?\\d+"), num("[+-]?\\d+(?:\\.\\d+)?"), comment("#.*");
    for (std::size_t i = 0; i<src.size(); i++)
    {
        std::uint8_t line;
        try
        {
            line = src.at(i);
        }
        catch (const std::exception& e)
        {
            stktb::err(i, "main-process", e.what());
        }

        if (line == Token::ENDIF)
        {
            try
            {
                env.inblock.pop();
            }
            catch (const std::exception& e)
            {
                stktb::err(i, "ENDIF", e.what());
            }
        }
        else if (line == Token::ENDDEF)
        {
            env.insub = false;
            if (env.current != 0)
            {
                i = env.current;
            }
        }
        else if (!env.inblock.top())
        {
            continue;
        }
        else if (env.insub)
        {
            continue;
        }
        
        if (line == Token::PUSH)
        {
            std::string s = constants.front();
            constants.pop_front();
            stktb::token_t t;
            if (std::regex_match(s, str))
            {
                env.stack.push(s.substr(1, s.length()-2));
            }
            else if (std::regex_match(s, num))
            {
                env.stack.push(std::stod(s));
            }
            else if (std::regex_match(s, intre))
            {
                env.stack.push(std::int64_t(std::stoi(s)));
            }
            else
            {
                stktb::err(i, "PUSH", "Invalid constant group value.");
            }
        }
        else if (line == Token::STORE)
        {
            stktb::token_t name = stktb::get(env), value = stktb::get(env);
            if (std::holds_alternative<std::string>(name))
            {
                env.vars.insert_or_assign(std::get<std::string>(name), value);
            }
            else 
            {
                stktb::err(i, "STORE", "The name of the variable must be a string.");
            }
        }
        else if (line == Token::LOAD)
        {
            stktb::token_t name = stktb::get(env);
            if (std::holds_alternative<std::string>(name))
            {
                try
                {
                    env.stack.push(env.vars.at(std::get<std::string>(name)));
                }
                catch (const std::exception& e)
                {
                    stktb::err(i, "LOAD", e.what());
                }
            }
            else
            {
                stktb::err(i, "LOAD", "The name of the variable must be a string.");
            }
        }
        else if (line == Token::ERASE)
        {
            stktb::token_t name = stktb::get(env);
            if (std::holds_alternative<std::string>(name))
            {
                env.vars.erase(std::get<std::string>(name));
            }
            else
            {
                stktb::err(i, "ERASE", "The name of the variable must be a string.");
            }
        }
        else if (line == Token::DEF)
        {
            stktb::token_t name = stktb::get(env);
            if (std::holds_alternative<std::string>(name))
            {
                env.insub = true;
                std::string name_s = std::get<std::string>(name);
                env.subs.insert_or_assign(name_s, i);
            }
            else
            {
                stktb::err(i, "DEF", "The name of the function must be a string.");
            }
        }
        else if (line == Token::CALL)
        {
            stktb::token_t name = stktb::get(env);
            if (std::holds_alternative<std::string>(name))
            {
                try
                {
                    env.current = i;
                    i = env.subs.at(std::get<std::string>(name));
                }
                catch (const std::exception& e)
                {
                    stktb::err(i, "CALL", e.what());
                }
            }
            else
            {
                stktb::err(i, "CALL", "The name of the function must be a string.");
            }
        }
        else if (line == Token::DEBUG)
        {
            stktb::token_t t = stktb::get(env);
            std::visit([](const auto& res){
                std::cout << res << "\n";
            }, t);
        }
        else if (line == Token::IMPORT)
        {
            stktb::token_t modname = stktb::get(env);
            if (std::holds_alternative<std::string>(modname))
            {
                std::string mod = std::get<std::string>(modname);
                boost::dll::shared_library lib{mod};
                env.mods.insert_or_assign(mod, lib);
            }
            else
            {
                stktb::err(i, "IMPORT", "The name of module must be a string.");
            }
        }
        else if (line == Token::USING)
        {
            stktb::token_t modname = stktb::get(env), funcname = stktb::get(env);
            if (std::holds_alternative<std::string>(funcname) && std::holds_alternative<std::string>(modname))
            {
                std::string mod = std::get<std::string>(modname), funcn = std::get<std::string>(funcname);
                try
                {
                    boost::function<void(stktb::envs&, std::size_t)> func = env.mods.at(mod).get<void(stktb::envs&, std::size_t)>(funcn);
                    env.user_funcs.insert_or_assign(mod+"."+funcn, func);
                }
                catch (const std::exception& e)
                {
                    stktb::err(i, "USING", e.what());
                }
            }
            else
            {
                stktb::err(i, "USING", "The name of function or module must be a string.");
            }
        }
        else if (line == Token::RUN)
        {
            stktb::token_t name = stktb::get(env);
            if (std::holds_alternative<std::string>(name))
            {
                env.user_funcs.at(std::get<std::string>(name))(env, i);
            }
            else
            {
                stktb::err(i, "RUN", "The name of function must be a string.");
            }
        }
        else if (line == Token::IF_TRUE)
        {
            stktb::token_t t = stktb::get(env);
            if (stktb::makebool(t))
            {
                env.inblock.push(true);
            }
            else
            {
                env.inblock.push(false);
            }
        }
        else if (line == Token::IF_FALSE)
        {
            stktb::token_t t = stktb::get(env);
            if (!stktb::makebool(t))
            {
                env.inblock.push(true);
            }
            else
            {
                env.inblock.push(false);
            }
        }
        else if (line == Token::GOTO)
        {
            stktb::token_t t = stktb::get(env);
            if (std::holds_alternative<std::int64_t>(t))
            {
                i += std::get<std::int64_t>(t);
            }
            else
            {
                stktb::err(i, "GOTO", "Relative distances must be numeric.");
            }
        }
        else if (line == Token::AND)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            env.stack.push(std::int64_t(stktb::makebool(l) && stktb::makebool(r)));
        }
        else if (line == Token::OR)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            env.stack.push(std::int64_t(stktb::makebool(l) || stktb::makebool(r)));            
        }
        else if (line == Token::GT)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::int64_t(std::get<std::int64_t>(l) > std::get<std::int64_t>(r)));
            }
            else if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::int64_t(std::get<std::int64_t>(l) > std::get<double>(r)));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::int64_t(std::get<double>(l) > std::get<std::int64_t>(r)));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::int64_t(std::get<double>(l) > std::get<double>(r)));
            }
            else
            {
                stktb::err(i, "GT", "Operands should be numbers.");
            }
        }
        else if (line == Token::LT)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::int64_t(std::get<std::int64_t>(l) < std::get<std::int64_t>(r)));
            }
            else if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::int64_t(std::get<std::int64_t>(l) < std::get<double>(r)));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::int64_t(std::get<double>(l) < std::get<std::int64_t>(r)));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::int64_t(std::get<double>(l) < std::get<double>(r)));
            }
            else
            {
                stktb::err(i, "GT", "Operands should be numbers.");
            }
        }
        else if (line == Token::ADD)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) + std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) + std::get<double>(r));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<double>(l) + std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::get<double>(l) + std::get<double>(r));
            }
            else
            {
                stktb::err(i, "GT", "Operands should be numbers.");
            }
        }
        else if (line == Token::SUB)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) - std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) - std::get<double>(r));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<double>(l) - std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::get<double>(l) - std::get<double>(r));
            }
            else
            {
                stktb::err(i, "GT", "Operands should be numbers.");
            }
        }
        else if (line == Token::MUL)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) * std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) * std::get<double>(r));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<double>(l) * std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::get<double>(l) * std::get<double>(r));
            }
            else
            {
                stktb::err(i, "GT", "Operands should be numbers.");
            }
        }
        else if (line == Token::DIV)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) / std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) / std::get<double>(r));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<double>(l) / std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::get<double>(l) / std::get<double>(r));
            }
            else
            {
                stktb::err(i, "GT", "Operands should be numbers.");
            }
        }
        else if (line == Token::MOD)
        {
            stktb::token_t l = stktb::get(env), r = stktb::get(env);
            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::get<std::int64_t>(l) % std::get<std::int64_t>(r));
            }
            else if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::remainder(std::get<std::int64_t>(l), std::get<double>(r)));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.push(std::remainder(std::get<double>(l), std::get<std::int64_t>(r)));
            }
            else if (std::holds_alternative<double>(l) && std::holds_alternative<double>(r))
            {
                env.stack.push(std::remainder(std::get<double>(l), std::get<double>(r)));
            }
            else
            {
                stktb::err(i, "GT", "Operands should be numbers.");
            }
        }
        else
        {
            stktb::err(i, "main-process", "Unknown token type.");
        }
    }
}
