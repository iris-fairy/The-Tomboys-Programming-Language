#include "tomboys.hpp"
#include <fstream>
#include <utility>
#include <stdexcept>
#include <regex>
#include <cmath>

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Invalid command argument.\n";
        return EXIT_FAILURE;
    }

    std::ifstream fin(argv[1]);
    if (!fin)
    {
        std::cerr << "Could not open the file.\n";
        return EXIT_FAILURE;
    }

    std::string line;
    tomboys::envs env;
    std::regex str("\".*\""), intre("[+-]?\\d+"), num("[+-]?\\d+(?:\\.\\d+)?"), comment("#.*");
    env.intrueblock.push(true);
    std::vector<std::string> src;
    while (std::getline(fin, line))
    {
        src.push_back(line);
    }
    
    for (unsigned int ln=0; ln<src.size(); ln++)
    {
        line = src.at(ln);
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line == "ENDIF")
        {
            try
            {
                env.intrueblock.pop();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "ENDIF", e.what());
            }
        }
        else if (line == "ENDDEF")
        {
            env.infunc = false;
            if (env.current != 0)
            {
                ln = env.current;
            }
        }
        else if (!env.intrueblock.top())
        {
            continue;
        }
        else if (env.infunc)
        {
            continue;
        }
        else if (line == "STORE-NOREF")
        {
            tomboys::token_t name;
            try
            {
                name = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "STORE-NOREF", e.what());
            }

            if (std::holds_alternative<std::string>(name))
            {
                env.vars.insert_or_assign(std::get<std::string>(name), "");
            }
            else
            {
                tomboys::inputerr(ln, "STORE-NOREF", "The name of the variable must be a string.");
            }
            env.stack.pop_back();
        }
        else if (line == "STORE")
        {
            tomboys::token_t name, value;
            try
            {
                name = env.stack.back(); 
                value = env.stack.at(env.stack.size()-2);
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "STORE", e.what());
            }

            if (std::holds_alternative<std::string>(name))
            {
                env.vars.insert_or_assign(std::get<std::string>(name), value);
            }
            else
            {
                tomboys::inputerr(ln, "STORE", "The name of the variable must be a string.");
            }
            env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
        }
        else if (line == "DEBUG")
        {
            std::vector<std::string> stack, vars;
            for (auto e : env.stack)
            {
                tomboys::fdebug(stack, e, ln);
            }
            std::cout << "[DEBUG] ";
            for (auto e : stack)
            {
                std::cout << e << " ";
            }
            std::cout << "\n";
        }
        else if (line == "LOAD")
        {
            tomboys::token_t name;
            try
            {
                name = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "LOAD", e.what());
            }

            if (std::holds_alternative<std::string>(name))
            {
                try
                {
                    env.stack.push_back(env.vars.at(std::get<std::string>(name)));
                }
                catch (const std::exception& e)
                {
                    tomboys::inputerr(ln, "LOAD", e.what());
                }
            }
            else
            {
                tomboys::inputerr(ln, "LOAD", "The name of the variable must be a string.");
            }
            env.stack.pop_back();
        }
        else if (line == "PRINT")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "PRINT", e.what());
            }

            if (std::holds_alternative<std::string>(token))
            {
                std::cout << std::get<std::string>(token);
            }
            else if (std::holds_alternative<std::int64_t>(token))
            {
                std::cout << std::to_string(std::get<std::int64_t>(token));
            }
            else if (std::holds_alternative<double>(token))
            {
                std::cout << std::to_string(std::get<double>(token));
            }
            else if (std::holds_alternative<tomboys::Token>(token))
            {
                switch (std::get<tomboys::Token>(token))
                {
                    case tomboys::Token::TRUE:
                        std::cout << "TRUE";
                        break;
                    case tomboys::Token::FALSE:
                        std::cout << "FALSE";
                        break;
                    default:
                        tomboys::inputerr(ln, "PRINT", "Unknown token type.");
                }
            }
            env.stack.pop_back();
        }
        else if (line == "NEWLINE")
        {
            env.stack.push_back("\n");
        }
        else if (line == "PRINT-STDERR")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "LOAD", e.what());
            }

            if (std::holds_alternative<std::string>(token))
            {
                std::cerr << std::get<std::string>(token);
            }
            else if (std::holds_alternative<std::int64_t>(token))
            {
                std::cerr << std::to_string(std::get<std::int64_t>(token));
            }
            else if (std::holds_alternative<double>(token))
            {
                std::cerr << std::to_string(std::get<double>(token));
            }
            else if (std::holds_alternative<tomboys::Token>(token))
            {
                switch (std::get<tomboys::Token>(token))
                {
                    case tomboys::Token::TRUE:
                        std::cerr << "TRUE";
                        break;
                    case tomboys::Token::FALSE:
                        std::cerr << "FALSE";
                        break;
                    default:
                        tomboys::inputerr(ln, "PRINT", "Unknown token type.");
                }
            }
            env.stack.pop_back();            
        }
        else if (line == "INPUT")
        {
            std::string s;
            std::getline(std::cin, s);
            env.stack.push_back(s);
        }
        else if (line == "ADD")
        {
            tomboys::opdo(ln, env, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)+std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<double>(l)+std::get<double>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<double>(l)+std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)+std::get<double>(r));
            });
        }
        else if (line == "SUB")
        {
            tomboys::opdo(ln, env, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)-std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<double>(l)-std::get<double>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<double>(l)-std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)-std::get<double>(r));
            });   
        }
        else if (line == "MUL")
        {
            tomboys::opdo(ln, env, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)*std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<double>(l)*std::get<double>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<double>(l)*std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)*std::get<double>(r));
            });
        }
        else if (line == "DIV")
        {
            tomboys::opdo(ln, env, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)/std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<double>(l)/std::get<double>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<double>(l)/std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)/std::get<double>(r));
            });
        }
        else if (line == "MOD")
        {
            tomboys::opdo(ln, env, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::get<std::int64_t>(l)%std::get<std::int64_t>(r));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::remainder(std::get<double>(l), std::get<double>(r)));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::remainder(std::get<double>(l), std::get<std::int64_t>(r)));
            }, [](tomboys::envs& env, tomboys::token_t& l, tomboys::token_t& r){
                env.stack.push_back(std::remainder(std::get<std::int64_t>(l), std::get<double>(r)));
            });
        }
        else if (line == "ADD-STRING")
        {
            tomboys::token_t l, r;
            try
            {
                r = env.stack.back();
                l = env.stack.at(env.stack.size()-2);
            }
            catch(const std::exception& e)
            {
                tomboys::inputerr(ln, "ADD-STRING", e.what());
            }

            if (std::holds_alternative<std::string>(l) && std::holds_alternative<std::string>(r))
            {
                env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
                env.stack.push_back(std::get<std::string>(l)+std::get<std::string>(r));
            }
            else
            {
                tomboys::inputerr(ln, "ADD-STRING", "The operator's operand should be a string.");
            }
        }
        else if (line == "SUBSTR")
        {
            tomboys::token_t begin, end, str;
            try
            {
                str = env.stack.at(env.stack.size()-3);
                begin = env.stack.at(env.stack.size()-2);
                end = env.stack.back();
            }
            catch(const std::exception& e)
            {
                tomboys::inputerr(ln, "SUBSTR", e.what());
            }

            if (std::holds_alternative<std::string>(str) &&
                std::holds_alternative<std::int64_t>(begin) && 
                std::holds_alternative<std::int64_t>(end))
            {
                std::int64_t b = std::get<std::int64_t>(begin), e = std::get<std::int64_t>(end);
                std::string s = std::get<std::string>(str);
                env.stack.erase(env.stack.begin()+(env.stack.size()-3), env.stack.end());
                env.stack.push_back(s.substr(b, e));
            }
            else
            {
                tomboys::inputerr(ln, "SUBSTR", "The operator's operand should be a string and number.");
            }
        }
        else if (line == "IF-TRUE")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "IF", e.what());
            }

            if (tomboys::distbool(token))
            {
                env.intrueblock.push(true);
            }
            else
            {
                env.intrueblock.push(false);
            }
            env.stack.pop_back();
        }
        else if (line == "IF-FALSE")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "IF", e.what());
            }

            if (!tomboys::distbool(token))
            {
                env.intrueblock.push(true);
            }
            else
            {
                env.intrueblock.push(false);
            }
            env.stack.pop_back();            
        }
        else if (line == "GOTO")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "GOTO", e.what());
            }

            if (std::holds_alternative<std::int64_t>(token))
            {
                ln += std::get<std::int64_t>(token);
            }
            else
            {
                tomboys::inputerr(ln, "GOTO", "The operator's operand should be a number.");
            }
            env.stack.pop_back();
        }
        else if (line == "DEF")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "DEF", e.what());
            }
            env.stack.pop_back();

            if (std::holds_alternative<std::string>(token))
            {
                env.subroutines.insert_or_assign(std::get<std::string>(token), ln);
            }
            else
            {
                tomboys::inputerr(ln, "DEF", "Function name should be a string.");
            }
            env.infunc = true;
        }
        else if (line == "CALL")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "CALL", e.what());
            }

            env.current = ln;
            if (std::holds_alternative<std::string>(token))
            {
                ln = env.subroutines.at(std::get<std::string>(token));
            }
            else
            {
                tomboys::inputerr(ln, "CALL", "Operand should be a string.");
            }
            env.stack.pop_back();
        }
        else if (line == "LT")
        {
            tomboys::token_t l, r;
            try
            {
                l = env.stack.at(env.stack.size()-2);
                r = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "LT", e.what());
            }

            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
                env.stack.push_back(std::int64_t(l < r));
            }
            else
            {
                tomboys::inputerr(ln, "LT", "The operator`s operand should be a number.");
            }
        }
        else if (line == "GT")
        {
            tomboys::token_t l, r;
            try
            {
                l = env.stack.at(env.stack.size()-2);
                r = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "GT", e.what());
            }

            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
                env.stack.push_back(std::int64_t(l > r));
            }
            else
            {
                tomboys::inputerr(ln, "GT", "The operator`s operand should be a number.");
            }
        }
        else if (line == "EQ")
        {
            tomboys::token_t l, r;
            try
            {
                l = env.stack.at(env.stack.size()-2);
                r = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "EQ", e.what());
            }

            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
                env.stack.push_back(std::int64_t(l == r));
            }
            else
            {
                tomboys::inputerr(ln, "EQ", "The operator`s operand should be a number.");
            }
        }
        else if (line == "NEQ")
        {
            tomboys::token_t l, r;
            try
            {
                l = env.stack.at(env.stack.size()-2);
                r = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "NEQ", e.what());
            }

            if (std::holds_alternative<std::int64_t>(l) && std::holds_alternative<std::int64_t>(r))
            {
                env.stack.erase(env.stack.begin()+(env.stack.size()-2), env.stack.end());
                env.stack.push_back(std::int64_t(l != r));
            }
            else
            {
                tomboys::inputerr(ln, "NEQ", "The operator`s operand should be a number.");
            }
        }
        else if (line == "NOT")
        {
            tomboys::token_t token;
            env.stack.push_back(!tomboys::distbool(token));
        }
        else if (line == "OR")
        {
            tomboys::token_t l, r;
            try
            {
                l = env.stack.at(env.stack.size()-2);
                r = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "OR", e.what());
            }
            env.stack.push_back(std::int64_t(tomboys::distbool(l) || tomboys::distbool(r)));
        }
        else if (line == "AND")
        {
            tomboys::token_t l, r;
            try
            {
                l = env.stack.at(env.stack.size()-2);
                r = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "AND", e.what());
            }
            env.stack.push_back(std::int64_t(tomboys::distbool(l) && tomboys::distbool(r)));
        }
        else if (line == "USE")
        {
            tomboys::token_t file, func;
            try
            {
                file = env.stack.at(env.stack.size()-2);
                func = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "USE", e.what());
            }

            if (std::holds_alternative<std::string>(file) && std::holds_alternative<std::string>(func))
            {
                try
                {
                    boost::dll::shared_library lib{std::get<std::string>(file)};
                    boost::function<tomboys::ft> funcc = lib.get<tomboys::ft>(std::get<std::string>(func));
                    env.func.insert_or_assign(std::get<std::string>(func), funcc);
                }
                catch (const std::exception& e)
                {
                    tomboys::inputerr(ln, "USE", e.what());
                }
            }
            else
            {
                tomboys::inputerr(ln, "USE", "Function name and module name should be a string.");
            }
        }
        else if (line == "CALL-MODULE")
        {
            tomboys::token_t func;
            try
            {
                func = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "USE", e.what());
            }            
            
            if (std::holds_alternative<std::string>(func))
            {
                env.func.at(std::get<std::string>(func))(env, ln);
            }
            else
            {
                tomboys::inputerr(ln, "CALL=MODULE", "Function name should be a string.");
            }
        }
        else if (line == "ERASE")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "ERASE", e.what());
            }

            if (std::holds_alternative<std::string>(token))
            {
                for (auto element = env.vars.begin(); element != env.vars.end(); element++)
                {
                    if (element->first == std::get<std::string>(token))
                    {
                        env.vars.erase(element);
                    }
                }
                for (auto element = env.func.begin(); element != env.func.end(); element++)
                {
                    if (element->first == std::get<std::string>(token))
                    {
                        env.func.erase(element);
                    }
                }
            }
            else
            {
                tomboys::inputerr(ln, "ERASE", "Key name should be a string.");
            }
        }
        else if (line == "EXIT")
        {
            tomboys::token_t token;
            try
            {
                token = env.stack.back();
            }
            catch (const std::exception& e)
            {
                tomboys::inputerr(ln, "EXIT", e.what());
            }

            if (std::holds_alternative<std::int64_t>(token))
            {
                std::exit(std::get<std::int64_t>(token));
            }
            else
            {
                tomboys::inputerr(ln, "EXIT", "Exit code should be a number.");
            }
        }
        else if (std::regex_match(line, str))
        {
            env.stack.push_back(line.substr(1, line.length()-2));
        }
        else if (std::regex_match(line, intre))
        {
            env.stack.push_back(std::int64_t(std::stoi(line)));
        }
        else if (std::regex_match(line, num))
        {
            env.stack.push_back(std::stod(line));
        }
        else if (std::regex_match(line, comment))
        {
            continue;
        }
        else
        {
            tomboys::inputerr(ln, "main-process", "No such token or value type found.");
        }
    }
}
