CREATE TABLE public.weather (address varchar(255));
insert into public.weather values ('aleblang@vertica.com');
insert into public.weather values ('aleblang@vertica.com');
SELECT EMAIL(weather.address, 'WEATHER ALERT', false, 'THERE IS A WEATHER WARNING') FROM weather;
