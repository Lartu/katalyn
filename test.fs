in $file: open_rw("aoc1_input.txt");
while read_line($file);
    local $line: trim($_r);
    if "one" :: $line;
        print("The line ", $line," contains 'one'");
    ok;
ok;

(*in $file: open_rw("aoc1_input.txt");
in $total: 0;
while read_line($file);
    local $line: trim($_r); (* $_r es el resultado del while y es local *)
    print($line); (* $_r es el resultado del if y es local *)
    local $first_digit: 0;
    local $last_digit: 0;

    (* Find first digit *)
    local $i: 1;
    while slice($line, $i, 1);
        (* Lo razonable acá sería un for $line con $_k y $_v*)
        local $char: $_r;
        if $char :: "0123456789";
            in $first_digit: $char;
            print("Found first digit: ", $first_digit);
            goto SECOND_CHAR;
        ok;
        in $i: $i + 1;
    ok;
    label SECOND_CHAR;

    (* Find list digit *)
    local $i: len($line); (* Por casos como este es que Katalyn indexa desde 1 *)
    while slice($line, $i, 1);
        local $char: $_r;
        if $char :: "0123456789";
            in $last_digit: $char;
            print("Found last digit: ", $last_digit);
            goto CONTINUE;
        ok;
        in $i: $i - 1;
    ok;
    label CONTINUE;

    in $total: $total + ($first_digit & $last_digit);
ok;
label EXIT;
print("Total: ", $total);

(*in $a: 99;
add_scope;
    local $a: $a - 1;
    print($a);
del_scope;

in $t: #;
in $t{100}: "Roberto";
in $t2: $t;
(*del($t, "100");
unset($a);
unset($t);*)

(*
Katalyn Documentation:
• funciones y comandos
• operadores
• variables y accesos
• strings, ints, floats y tables
• $_r
• comentarios, unclosed y nested
*)

print($a);

(*(* Fibonacci calculator!! *)
in $a: 1; in $b: 1;
in $n: 9000000; in $max: $n;
if $n <= 0;
    print("Error, enter a number greater than or equal to 1.");
    exit(1);
ok;
if $n <= 2;
    print(1);
ok;
if $n > 2;
    in $n: $n - 2;
    in $iterations: 0;
    in $total_iterations: 0;
    while $n > 0;
        if $iterations = 100000;
            in $total_iterations: $total_iterations + $iterations;
            in $iterations: 0;
            print("Me faltan ", $max - $total_iterations, " números");
        ok;
        in $iterations: $iterations + 1;
        in $c: $a + $b; in $a: $b; in $b: $c;
        in $n: $n - 1;
    ok;
    if $c % 3 = 0;
        print("It's divisible by 3!");
    ok;
    if $c % 3 <> 0;
        print("It's not divisible by 3 :(");
    ok;
    print($c);
ok;

(*(* Disan Count *)
in $max: accept("Enter the max value: ");
in $even_numbers: #;
in $even_numbers{size}: 0;
if $max < 0;
    print("The number must be greater than 0.");
    exit(1);
ok;
while $max >= 0;
    if $max % 2 = 0;
        print($max, " is even!");
        in $even_numbers[$even_numbers{size}]: $max;
        in $even_numbers{size}: $even_numbers{size} + 1;
    ok;
    in $max: $max - 1;
ok;

(* Print all even numbers found, again, in reverse order. *)
print("I've found " & $even_numbers{size} & " even numbers. They are:");
in $i: 0;
while is($even_numbers, $i);
    print("• " & $even_numbers[$i]);
    in $i: $i + 1;
ok;
exit(0);*)