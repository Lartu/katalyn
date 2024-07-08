(* Disan Count *)
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
exit(0);