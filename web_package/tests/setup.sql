-- do setup for this package (i.e., install the package and create the functions)

CREATE TABLE raw_logs (data VARCHAR(4000));
COPY raw_logs FROM STDIN;
217.226.190.13 - - [16/Oct/2003:02:59:28 -0400] "GET /scripts/nsiislog.dll" 404 307
65.124.172.131 - - [16/Oct/2003:03:50:51 -0400] "GET /scripts/nsiislog.dll" 404 307
65.194.193.201 - - [16/Oct/2003:09:17:42 -0400] "GET / HTTP/1.0" 200 14
66.92.74.252 - - [16/Oct/2003:09:43:49 -0400] "GET / HTTP/1.1" 200 14
66.92.74.252 - - [16/Oct/2003:09:43:49 -0400] "GET /favicon.ico HTTP/1.1" 404 298
65.221.182.2 - - [01/Nov/2003:22:39:51 -0500] "GET /main.css HTTP/1.1" 200 373
65.221.182.2 - - [01/Nov/2003:22:39:52 -0500] "GET /about.html HTTP/1.1" 200 532
65.221.182.2 - - [01/Nov/2003:22:39:55 -0500] "GET /web.mit.edu HTTP/1.1" 404 298
66.249.67.20 - - [02/May/2011:03:28:35 -0700] "GET /robots.txt HTTP/1.1" 404 335 "-" "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)"
66.249.67.20 - - [02/May/2011:03:28:35 -0700] "GET /classes/commit/fft-factoring.pdf HTTP/1.1" 200 69534 "-" "Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)"
123.108.250.82 - - [02/May/2011:19:59:17 -0700] "GET /classes/commit/pldi03-aal.pdf HTTP/1.1" 200 346761 "-" "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 1.1.4322)"
144.51.26.54 - - [01/Jun/2011:06:36:51 -0700] "GET /classes/commit/fft-factoring.pdf HTTP/1.1" 200 69534 "http://www.google.com/url?sa=t&source=web&cd=5&ved=0CDYQFjAE&url=http%3A%2F%2Fandrew.nerdnetworks.org%2Fclasses%2Fcommit%2Ffft-factoring.pdf&rct=j&q=dft%20factorization&ei=akDmTcHBCILw0gGgofCeCw&usg=AFQjCNEyfN4KoSidrjR4EsL5wTHbqakb7A" "Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; rv:1.9.2.17) Gecko/20110421 Red Hat/3.6-1.el5_6 Firefox/3.6.17"
144.51.26.54 - - [01/Jun/2011:06:36:51 -0700] "GET /favicon.ico HTTP/1.1" 200 790 "-" "Mozilla/5.0 (X11; U; Linux i686 (x86_64); en-US; rv:1.9.2.17) Gecko/20110421 Red Hat/3.6-1.el5_6 Firefox/3.6.17"
129.33.49.251 - - [01/Jun/2011:11:54:13 -0700] "GET /classes/mechatronics/ion-generator.pdf HTTP/1.1" 200 1064343 "http://www.google.com/search?q=ion+%22measure+airflow%22&ie=utf-8&oe=utf-8&aq=t&rls=org.mozilla:en-US:official&client=firefox-a" "Mozilla/5.0 (Windows NT 5.1; rv:2.0.1) Gecko/20100101 Firefox/4.0.1"
\.

