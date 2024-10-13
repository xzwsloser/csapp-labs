#! /bin/bash
mytsh_result="./tshmy.txt"   
tshref_result="./tshref.txt"
truncate $mytsh_result --size 0 
truncate $tshref_result --size 0 
echo -e "testing...\n"

for i in {1..16}
do
    t=""
    if [ $i -ge 10 ]; then t="$i"
    else t="0$i"
    fi
    echo -e "./sdriver.pl  -t trace$t.txt -s ./tsh -a \"-p\"" >> result
    ./sdriver.pl  -t trace$t.txt -s ./tsh -a "-p" >> "$mytsh_result"
    ./sdriver.pl  -t trace$t.txt -s ./tshref -a "-p" >> $tshref_result
done

diff $tshref_result $mytsh_result   > ./tshdiff