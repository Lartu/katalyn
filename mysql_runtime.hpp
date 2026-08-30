#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace katalyn_mysql
{
struct Cell
{
    bool is_null = true;
    std::string text;
};

struct QueryResult
{
    bool ok = false;
    std::vector<std::string> columns;
    std::vector<std::vector<Cell>> rows;
    std::uint64_t affected_rows = 0;
    std::uint64_t insert_id = 0;
};

bool available();
std::string client_info();
std::int64_t connect(const std::string &host,
                     const std::string &user,
                     const std::string &password,
                     const std::string &database,
                     unsigned int port,
                     const std::string &unix_socket);
bool close(std::int64_t connection);
QueryResult query(std::int64_t connection, const std::string &sql);
std::string error(std::int64_t connection = 0);
unsigned int error_number(std::int64_t connection = 0);
} // namespace katalyn_mysql
