# Katalyn

Katalyn is a compact interpreted scripting language designed to combine Lua-like
simplicity with Perl-like practicality. The current interpreter is implemented
in C++17 and compiles source into instructions for its in-process Nambly VM.

Katalyn is experimental software. It is suitable for learning, personal tools,
automation, text processing, and experimental Apache CGI programs, but has not
yet received a production security audit.

## Build

```sh
make
make test
```

The executable is created at `build/kat`.

```sh
./build/kat examples/disan_count.kat
./build/kat -a 'print("Hello from Katalyn!");'
```

Install it under `/usr/local/bin` with:

```sh
sudo make install
```

## Command line

```text
kat [switches] <source file> [arguments...]
```

- `-a <source>` executes source supplied directly on the command line.
- `-s` reads the program source from standard input.
- `-i` prints the compiled internal representation without executing it.
- `-n` runs without the embedded standard library.
- `-h` prints command-line help; `-v` prints the version and build timestamp.

Arguments following a source filename are available in the one-based `$_args`
table. A script beginning with `#!/usr/local/bin/kat`
can be made executable and run directly.

At startup, `$_scriptpath` contains the script's absolute filename,
`$_scriptdir` contains its absolute parent directory, and `$_wdir` contains the
absolute working directory. For source supplied with `-a` or `-s`, both script
location variables are nil because no script file exists.

## Language facilities

Katalyn includes Unicode text, associative tables, immutable byte sequences,
structured `try`/`catch`/`finally` errors, actor-style workers and message
passing, JSON and CGI helpers, binary I/O, and portable filesystem/path
operations. For example:

```katalyn
try;
    $payload: read_bytes(path_join($_scriptdir, "message.txt"));
    print(utf8_decode($payload));
catch $error;
    print($error{kind}, ": ", $error{message});
finally;
    print("Finished.");
ok;
```

`finally` is guaranteed to run when its protected block completes normally,
raises an error, returns from a function, or leaves a loop with `break` or
`continue`. See the complete manual for the byte encoding, binary file, and
filesystem APIs.

## Concurrency

Workers have isolated globals and FIFO inboxes. Messages are copied when sent,
so workers do not share mutable tables. The runtime schedules work
automatically; Katalyn programs do not need a `yield` function.

```katalyn
def greet;
    $message: receive();
    send($message{from}, "Hello, " & $message{message} & "!");
    return "finished";
ok;

$worker: spawn(greet);
send($worker, "Katalyn");
$reply: receive(1);
print($reply{message});
$result: wait($worker);
```

The worker API consists of `spawn`, `self`, `send`, `receive`, `receive_now`,
`worker_alive`, and `wait`. Uncaught worker errors stay inside that worker and
are reported by `wait`. See `examples/concurrency.kat` and the concurrency
chapter in the complete manual for lifecycle, timeout, copying, and error
semantics.

## Apache CGI

Katalyn includes native CGI request handling, bounded raw standard input,
environment access, URL/form decoding, JSON encoding and decoding, and safe CGI
response generation.

```katalyn
#!/usr/local/bin/kat

$request: cgi_request();
$name: "world";
if is($request{query}{name}) && len($request{query}{name});
    $name: $request{query}{name};
ok;
$response: table;
$response["message"]: "Hello, " & $name & "!";
cgi_response(200, "application/json; charset=utf-8", json_encode($response));
```

See `examples/apache_cgi.kat` and `docs-complete/KATALYN-CGI.txt` for Apache
configuration, the complete request-table layout, security notes, and the CGI
API reference.

## Documentation

- `docs-complete/index.html` — complete single-page HTML manual
- `docs-complete/KATALYN-MANUAL.txt` — complete plain-text language manual
- `docs-complete/KATALYN-CGI.txt` — Apache CGI and JSON guide

Katalyn is released under the Apache License 2.0. See `LICENSE`.
