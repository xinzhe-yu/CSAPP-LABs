tommy@DESKTOP-RCDA7S1:~/CSAPP-LABs/Labs/shlab-handout/shlab-handout$ for i in $(seq -w 1 16); do   echo "=== trace$i ===";   diff <(./sdriver.pl -t trace$i.txt -s ./tsh -a "-p")        <(./sdriver.pl -t trace$i.txt -s ./tshref -a "-p"); done
=== trace01 ===
=== trace02 ===
=== trace03 ===
=== trace04 ===
5c5
< [1] (71318) ./myspin 1 &
---
> [1] (71319) ./myspin 1 &
=== trace05 ===
5c5
< [1] (71355) ./myspin 2 &
---
> [1] (71357) ./myspin 2 &
7c7
< [2] (71359) ./myspin 3 &
---
> [2] (71361) ./myspin 3 &
9,10c9,10
< [1] (71355) Running ./myspin 2 &
< [2] (71359) Running ./myspin 3 &
---
> [1] (71357) Running ./myspin 2 &
> [2] (71361) Running ./myspin 3 &
=== trace06 ===
5c5
< Job [1] (71391) terminated by signal 2
---
> Job [1] (71390) terminated by signal 2
=== trace07 ===
5c5
< [1] (71422) ./myspin 4 &
---
> [1] (71421) ./myspin 4 &
7c7
< Job [2] (71426) terminated by signal 2
---
> Job [2] (71425) terminated by signal 2
9c9
< [1] (71422) Running ./myspin 4 &
---
> [1] (71421) Running ./myspin 4 &
=== trace08 ===
5c5
< [1] (71461) ./myspin 4 &
---
> [1] (71456) ./myspin 4 &
7c7
< Job [2] (71463) stopped by signal 20
---
> Job [2] (71459) stopped by signal 20
9,10c9,10
< [1] (71461) Running ./myspin 4 &
< [2] (71463) Stopped ./myspin 5 
---
> [1] (71456) Running ./myspin 4 &
> [2] (71459) Stopped ./myspin 5 
=== trace09 ===
5c5
< [1] (71524) ./myspin 4 &
---
> [1] (71526) ./myspin 4 &
7c7
< Job [2] (71528) stopped by signal 20
---
> Job [2] (71529) stopped by signal 20
9,10c9,10
< [1] (71524) Running ./myspin 4 &
< [2] (71528) Stopped ./myspin 5 
---
> [1] (71526) Running ./myspin 4 &
> [2] (71529) Stopped ./myspin 5 
12c12
< [2] (71528) ./myspin 5 
---
> [2] (71529) ./myspin 5 
14,15c14,15
< [1] (71524) Running ./myspin 4 &
< [2] (71528) Running ./myspin 5 
---
> [1] (71526) Running ./myspin 4 &
> [2] (71529) Running ./myspin 5 
=== trace10 ===
5c5
< [1] (71595) ./myspin 4 &
---
> [1] (71594) ./myspin 4 &
7c7
< Job [1] (71595) stopped by signal 20
---
> Job [1] (71594) stopped by signal 20
9c9
< [1] (71595) Stopped ./myspin 4 &
---
> [1] (71594) Stopped ./myspin 4 &
=== trace11 ===
5c5
< Job [1] (71658) terminated by signal 2
---
> Job [1] (71657) terminated by signal 2
=== trace12 ===
5c5
< Job [1] (71679) stopped by signal 20
---
> Job [1] (71678) stopped by signal 20
7c7
< [1] (71679) Stopped ./mysplit 4 
---
> [1] (71678) Stopped ./mysplit 4 
=== trace13 ===
5c5
< Job [1] (71741) stopped by signal 20
---
> Job [1] (71740) stopped by signal 20
7c7
< [1] (71741) Stopped ./mysplit 4 
---
> [1] (71740) Stopped ./mysplit 4 
=== trace14 ===
7c7
< [1] (71796) ./myspin 4 &
---
> [1] (71800) ./myspin 4 &
23c23
< Job [1] (71796) stopped by signal 20
---
> Job [1] (71800) stopped by signal 20
27c27
< [1] (71796) ./myspin 4 &
---
> [1] (71800) ./myspin 4 &
29c29
< [1] (71796) Running ./myspin 4 &
---
> [1] (71800) Running ./myspin 4 &
=== trace15 ===
7c7
< Job [1] (71857) terminated by signal 2
---
> Job [1] (71856) terminated by signal 2
9c9
< [1] (71900) ./myspin 3 &
---
> [1] (71899) ./myspin 3 &
11c11
< [2] (71904) ./myspin 4 &
---
> [2] (71903) ./myspin 4 &
13,14c13,14
< [1] (71900) Running ./myspin 3 &
< [2] (71904) Running ./myspin 4 &
---
> [1] (71899) Running ./myspin 3 &
> [2] (71903) Running ./myspin 4 &
16c16
< Job [1] (71900) stopped by signal 20
---
> Job [1] (71899) stopped by signal 20
18,19c18,19
< [1] (71900) Stopped ./myspin 3 &
< [2] (71904) Running ./myspin 4 &
---
> [1] (71899) Stopped ./myspin 3 &
> [2] (71903) Running ./myspin 4 &
23c23
< [1] (71900) ./myspin 3 &
---
> [1] (71899) ./myspin 3 &
25,26c25,26
< [1] (71900) Running ./myspin 3 &
< [2] (71904) Running ./myspin 4 &
---
> [1] (71899) Running ./myspin 3 &
> [2] (71903) Running ./myspin 4 &
=== trace16 ===
6c6
< Job [1] (71954) stopped by signal 20
---
> Job [1] (71955) stopped by signal 20
8c8
< [1] (71954) Stopped ./mystop 2
---
> [1] (71955) Stopped ./mystop 2
10c10
< Job [2] (71974) terminated by signal 2
---
> Job [2] (71975) terminated by signal 2