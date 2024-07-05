(* Katalyn Disan Count *)
in $max: accept();
in $i: $max;
in $even_numbers: #;
in $even_numbers{max value}: $max;
in $even_count: 0;
while $i >= 0;
    if $i % 2 = 0;
        print($i & " is even!");
        (* Save all even numbers *)
        in $even_numbers[$even_count]: $i;
        in $even_count: $even_count + 1;
    ok;
    in $i: $i + 1;
ok;
print("Done! I've found " & $even_count & " even numbers.");