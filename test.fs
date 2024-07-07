(1-2) * $b[10 + 20] + sum(1, 2);

(*((($b * 2 - 3) + -3) ^ max(2, 3, 7.2) > 0 + 3);

(* Katalyn Disan Count
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
    in $i: $i + - 1;
ok;
print("Done! I've found " & $even_count & " even numbers.");


(* Bottles of beer example *)
for range(99, 1);
    print($k & " bottle" & $s & " of beer on the wall,");
    print($k & " bottle" & $s & " of beer.");
    print("Take one down, pass it around,");
    print(($k - 1) & " bottle" & $s & " of beer on the wall.");
ok;
print("No more bottles of beer on the wall,\n\
       no more bottles of beer,\n\
       Go to the store and buy some more,\n\
       99 bottles of beer on the wall...");