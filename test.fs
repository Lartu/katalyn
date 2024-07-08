in $a: 99;
add_scope;
    local $a: $a - 1;
    print($a);
del_scope;

in $t: #;
in $t{100}: "Roberto";
del($t, "100");
unset($a);

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