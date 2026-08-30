#include "mysql_runtime.hpp"

#include <cstdlib>
#include <dlfcn.h>
#include <map>
#include <mutex>
#include <utility>

namespace katalyn_mysql
{
namespace
{
struct MYSQL;
struct MYSQL_RES;
using MYSQL_ROW = char **;
using my_ulonglong = unsigned long long;

// MYSQL_FIELD begins with `name` in both supported public C APIs. Keeping the
// client optional means this translation unit cannot include either vendor's
// headers, so only that stable leading field is described here.
struct MysqlFieldPrefix
{
    char *name;
};

template <typename T>
bool load_symbol(void *library, const char *name, T &target)
{
    target = reinterpret_cast<T>(dlsym(library, name));
    return target != nullptr;
}

class Driver
{
public:
    using Init = MYSQL *(*)(MYSQL *);
    using RealConnect = MYSQL *(*)(MYSQL *, const char *, const char *, const char *,
                                   const char *, unsigned int, const char *, unsigned long);
    using Close = void (*)(MYSQL *);
    using RealQuery = int (*)(MYSQL *, const char *, unsigned long);
    using StoreResult = MYSQL_RES *(*)(MYSQL *);
    using FreeResult = void (*)(MYSQL_RES *);
    using FetchRow = MYSQL_ROW (*)(MYSQL_RES *);
    using FetchLengths = unsigned long *(*)(MYSQL_RES *);
    using FetchField = void *(*)(MYSQL_RES *);
    using NumFields = unsigned int (*)(MYSQL_RES *);
    using FieldCount = unsigned int (*)(MYSQL *);
    using AffectedRows = my_ulonglong (*)(MYSQL *);
    using InsertId = my_ulonglong (*)(MYSQL *);
    using Error = const char *(*)(MYSQL *);
    using Errno = unsigned int (*)(MYSQL *);
    using Sqlstate = const char *(*)(MYSQL *);
    using SetCharacterSet = int (*)(MYSQL *, const char *);
    using ClientInfo = const char *(*)();

    Driver()
    {
        const char *explicit_path = std::getenv("KATALYN_MYSQL_LIBRARY");
        if (explicit_path && *explicit_path)
            library = dlopen(explicit_path, RTLD_NOW | RTLD_LOCAL);

        static const char *candidates[] = {
            "libmariadb.3.dylib", "libmariadb.dylib",
            "libmysqlclient.24.dylib", "libmysqlclient.23.dylib",
            "libmysqlclient.22.dylib", "libmysqlclient.21.dylib",
            "libmysqlclient.20.dylib", "libmysqlclient.dylib",
            "libmariadb.so.3", "libmariadb.so",
            "libmysqlclient.so.24", "libmysqlclient.so.23",
            "libmysqlclient.so.22", "libmysqlclient.so.21",
            "libmysqlclient.so.20", "libmysqlclient.so"};
        if (!library)
            for (const char *candidate : candidates)
            {
                library = dlopen(candidate, RTLD_NOW | RTLD_LOCAL);
                if (library)
                    break;
            }

        if (!library)
        {
            last_error = "MySQL client library not found. Install MySQL Client or MariaDB Connector/C, "
                         "or set KATALYN_MYSQL_LIBRARY to its full path.";
            return;
        }

        bool complete =
            load_symbol(library, "mysql_init", mysql_init) &&
            load_symbol(library, "mysql_real_connect", mysql_real_connect) &&
            load_symbol(library, "mysql_close", mysql_close) &&
            load_symbol(library, "mysql_real_query", mysql_real_query) &&
            load_symbol(library, "mysql_store_result", mysql_store_result) &&
            load_symbol(library, "mysql_free_result", mysql_free_result) &&
            load_symbol(library, "mysql_fetch_row", mysql_fetch_row) &&
            load_symbol(library, "mysql_fetch_lengths", mysql_fetch_lengths) &&
            load_symbol(library, "mysql_fetch_field", mysql_fetch_field) &&
            load_symbol(library, "mysql_num_fields", mysql_num_fields) &&
            load_symbol(library, "mysql_field_count", mysql_field_count) &&
            load_symbol(library, "mysql_affected_rows", mysql_affected_rows) &&
            load_symbol(library, "mysql_insert_id", mysql_insert_id) &&
            load_symbol(library, "mysql_error", mysql_error) &&
            load_symbol(library, "mysql_errno", mysql_errno) &&
            load_symbol(library, "mysql_sqlstate", mysql_sqlstate) &&
            load_symbol(library, "mysql_set_character_set", mysql_set_character_set) &&
            load_symbol(library, "mysql_get_client_info", mysql_get_client_info);
        if (!complete)
        {
            dlclose(library);
            library = nullptr;
            last_error = "The loaded MySQL client library does not provide the required C API.";
            return;
        }
    }

    ~Driver()
    {
        for (auto &entry : connections)
            mysql_close(entry.second);
        if (library)
            dlclose(library);
    }

    bool is_available() const { return library != nullptr; }

    std::string information()
    {
        std::lock_guard<std::mutex> guard(lock);
        if (!library)
            return std::string();
        const char *info = mysql_get_client_info();
        return info ? std::string(info) : std::string();
    }

    std::int64_t open(const std::string &host, const std::string &user,
                      const std::string &password, const std::string &database,
                      unsigned int port, const std::string &unix_socket)
    {
        std::lock_guard<std::mutex> guard(lock);
        clear_error();
        if (!library)
        {
            last_error = "MySQL client library is unavailable.";
            return 0;
        }

        MYSQL *connection = mysql_init(nullptr);
        if (!connection)
        {
            last_error = "The MySQL client could not allocate a connection.";
            return 0;
        }
        const char *host_value = host.empty() ? nullptr : host.c_str();
        const char *database_value = database.empty() ? nullptr : database.c_str();
        const char *socket_value = unix_socket.empty() ? nullptr : unix_socket.c_str();
        if (!mysql_real_connect(connection, host_value, user.c_str(), password.c_str(),
                                database_value, port, socket_value, 0))
        {
            capture_error(connection);
            mysql_close(connection);
            return 0;
        }

        if (mysql_set_character_set(connection, "utf8mb4") != 0)
            mysql_set_character_set(connection, "utf8");
        const std::int64_t handle = next_handle++;
        connections[handle] = connection;
        return handle;
    }

    bool shut(std::int64_t handle)
    {
        std::lock_guard<std::mutex> guard(lock);
        clear_error();
        auto found = connections.find(handle);
        if (found == connections.end())
        {
            set_invalid_handle();
            return false;
        }
        mysql_close(found->second);
        connections.erase(found);
        return true;
    }

    QueryResult execute(std::int64_t handle, const std::string &sql)
    {
        std::lock_guard<std::mutex> guard(lock);
        QueryResult output;
        clear_error();
        auto found = connections.find(handle);
        if (found == connections.end())
        {
            set_invalid_handle();
            return output;
        }
        MYSQL *connection = found->second;
        if (sql.size() > static_cast<std::size_t>(~0UL))
        {
            last_error = "SQL statement is too large for the MySQL client API.";
            return output;
        }
        if (mysql_real_query(connection, sql.data(), static_cast<unsigned long>(sql.size())) != 0)
        {
            capture_error(connection);
            return output;
        }

        MYSQL_RES *result = mysql_store_result(connection);
        if (!result)
        {
            if (mysql_field_count(connection) != 0)
            {
                capture_error(connection);
                return output;
            }
            output.ok = true;
            output.affected_rows = mysql_affected_rows(connection);
            output.insert_id = mysql_insert_id(connection);
            return output;
        }

        const unsigned int field_count = mysql_num_fields(result);
        output.columns.reserve(field_count);
        for (unsigned int i = 0; i < field_count; ++i)
        {
            void *raw_field = mysql_fetch_field(result);
            const auto *field = reinterpret_cast<const MysqlFieldPrefix *>(raw_field);
            output.columns.emplace_back(field && field->name ? field->name : "");
        }

        while (MYSQL_ROW row = mysql_fetch_row(result))
        {
            unsigned long *lengths = mysql_fetch_lengths(result);
            if (!lengths)
            {
                capture_error(connection);
                mysql_free_result(result);
                return QueryResult();
            }
            std::vector<Cell> converted;
            converted.reserve(field_count);
            for (unsigned int i = 0; i < field_count; ++i)
            {
                Cell cell;
                cell.is_null = row[i] == nullptr;
                if (!cell.is_null)
                    cell.text.assign(row[i], lengths[i]);
                converted.push_back(std::move(cell));
            }
            output.rows.push_back(std::move(converted));
        }
        mysql_free_result(result);
        output.ok = true;
        output.affected_rows = mysql_affected_rows(connection);
        output.insert_id = mysql_insert_id(connection);
        return output;
    }

    std::string get_error(std::int64_t handle)
    {
        std::lock_guard<std::mutex> guard(lock);
        if (handle != 0)
        {
            auto found = connections.find(handle);
            if (found != connections.end())
            {
                const char *message = mysql_error(found->second);
                if (message && *message)
                    return message;
            }
        }
        return last_error;
    }

    unsigned int get_errno(std::int64_t handle)
    {
        std::lock_guard<std::mutex> guard(lock);
        if (handle != 0)
        {
            auto found = connections.find(handle);
            if (found != connections.end())
                return mysql_errno(found->second);
        }
        return last_errno;
    }

private:
    void clear_error()
    {
        last_error.clear();
        last_errno = 0;
    }

    void capture_error(MYSQL *connection)
    {
        const char *message = mysql_error(connection);
        const char *state = mysql_sqlstate(connection);
        last_error = message ? message : "Unknown MySQL error.";
        if (state && *state)
            last_error += " (SQLSTATE " + std::string(state) + ")";
        last_errno = mysql_errno(connection);
    }

    void set_invalid_handle()
    {
        last_error = "Invalid or closed MySQL connection handle.";
        last_errno = 0;
    }

    void *library = nullptr;
    Init mysql_init = nullptr;
    RealConnect mysql_real_connect = nullptr;
    Close mysql_close = nullptr;
    RealQuery mysql_real_query = nullptr;
    StoreResult mysql_store_result = nullptr;
    FreeResult mysql_free_result = nullptr;
    FetchRow mysql_fetch_row = nullptr;
    FetchLengths mysql_fetch_lengths = nullptr;
    FetchField mysql_fetch_field = nullptr;
    NumFields mysql_num_fields = nullptr;
    FieldCount mysql_field_count = nullptr;
    AffectedRows mysql_affected_rows = nullptr;
    InsertId mysql_insert_id = nullptr;
    Error mysql_error = nullptr;
    Errno mysql_errno = nullptr;
    Sqlstate mysql_sqlstate = nullptr;
    SetCharacterSet mysql_set_character_set = nullptr;
    ClientInfo mysql_get_client_info = nullptr;
    std::map<std::int64_t, MYSQL *> connections;
    std::int64_t next_handle = 1;
    std::string last_error;
    unsigned int last_errno = 0;
    std::mutex lock;
};

Driver &driver()
{
    static Driver instance;
    return instance;
}
} // namespace

bool available() { return driver().is_available(); }
std::string client_info() { return driver().information(); }
std::int64_t connect(const std::string &host, const std::string &user,
                     const std::string &password, const std::string &database,
                     unsigned int port, const std::string &unix_socket)
{
    return driver().open(host, user, password, database, port, unix_socket);
}
bool close(std::int64_t connection) { return driver().shut(connection); }
QueryResult query(std::int64_t connection, const std::string &sql)
{
    return driver().execute(connection, sql);
}
std::string error(std::int64_t connection) { return driver().get_error(connection); }
unsigned int error_number(std::int64_t connection) { return driver().get_errno(connection); }
} // namespace katalyn_mysql
