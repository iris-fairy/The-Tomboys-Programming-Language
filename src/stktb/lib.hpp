#pragma once
#include "stktb.hpp"
#include <iostream>
#include <cstdint>
#include <string>
#include <cstdlib>
#include <list>
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>

namespace stktb
{
    inline void err(std::uint64_t n, std::string place, std::string reason)
    {
        std::cerr << "Invalid input: " << n+1 << ": " << place << " (" << reason << ")\n"; 
        std::exit(EXIT_FAILURE);
    }

    bool makebool(token_t t)
    {
        if (std::holds_alternative<std::string>(t))
        {
            return std::get<std::string>(t) != "";
        }
        else if (std::holds_alternative<std::int64_t>(t))
        {
            return std::get<std::int64_t>(t);
        }
        else if (std::holds_alternative<double>(t))
        {
            return std::get<double>(t);
        }
        else
        {
            return false;
        }
    }

    template <typename T>
    T getvalue(const std::string& filename)
    {
        T ret;
        std::ifstream fin(filename, std::ios::binary);
        if (fin.fail())
        {
            std::cerr << "Could not open the file.\n";
            std::exit(EXIT_FAILURE);
        }
        boost::archive::binary_iarchive bia(fin);
        bia >> ret;
        return ret;
    }

    token_t get(envs& env)
    {
        token_t t = env.stack.top();
        env.stack.pop();
        return t;
    }
}