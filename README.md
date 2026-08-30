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

The executable is created at `build/katalyn`.

```sh
./build/katalyn examples/disan_count.kat
./build/katalyn -a 'print("Hello from Katalyn!");'
```

Install it under `/usr/local/bin` with:

```sh
sudo make install
```

## Command line

```text
katalyn [switches] <source file> [arguments...]
```

- `-a <source>` executes source supplied directly on the command line.
- `-s` reads the program source from standard input.
- `-i` prints the compiled internal representation without executing it.
- `-n` runs without the embedded standard library.
- `-h` prints command-line help; `-v` prints the version.

Arguments following a source filename are available in the one-based `$_args`
table. A script beginning with `#!/usr/local/bin/katalyn`
can be made executable and run directly.

## Apache CGI

Katalyn includes native CGI request handling, bounded raw standard input,
environment access, URL/form decoding, JSON encoding and decoding, and safe CGI
response generation.

```katalyn
#!/usr/local/bin/katalyn

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
