SELECT EMAIL('email@vertica.com', 'This is the message subject', false, 'This is the message body');

-- assuming database w1 has column address that contains varchars 
SELECT EMAIL(w1.address, 'WEATHER ALERT', true, '/home/usr/weatherAlert.txt') FROM w1;
