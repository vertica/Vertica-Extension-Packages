--------------------------------------------------------
-- google_analytics.sql
--
-- Description: Examples of executing the analytics functions.
--
--
-- Vertica Analytic Database
--
-- Submitted by Patrick Toole
--
-- Copyright 2011 Vertica Systems, an HP Company
--
--
-- Portions of this software Copyright (c) 2011 by Vertica, an HP
-- Company.  All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions are
-- met:
--
--   - Redistributions of source code must retain the above copyright
--     notice, this list of conditions and the following disclaimer.
--
--   - Redistributions in binary form must reproduce the above copyright
--     notice, this list of conditions and the following disclaimer in the
--     documentation and/or other materials provided with the distribution.
--
--
--   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
--   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
--   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
--   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
--   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
--   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
--   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
--   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
--   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
--   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
--   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--
--
--------------------------------------------------------



--
-- NOTE: This function will not work in demo mode. It must be configured to 
-- accept a user name and password for a Google Analytics account.
--
-- Description:
-- This function will log into Google, then return a session key that can be
-- passed to the next set of functions.
--
-- Input:
-- varchar username
-- varchar password
--
-- Return: 
-- varchar authorization token or error message
--
select gapi_analytics_authorize('xxx@gmail.com','xxxxx');
--
-- Example:
--                     gapi_analytics_authorize                              
-----------------------------------------------------------------------------
-- DEAAALsAAADzr7ARhKHRCGBGpHfA3wWEcLgV0E-_VhrjS131qPDW4vuxItf2kxxnPyhil4TU-H
-- DSmQ9U72vwiKDejQ-VJK_5r-uA1mz8kRrQRxP-eLxJy9f0MuOCdvZTMEna506t4IiskWhSIBQb
-- r5oN-A1CEb521qX0iU8RMZskYXEDZnyv4Gml2etvum65Jr3D3TNSFmW3MjYFKqUyJCPDejBfZi
-- 6-K9qPAvU5PP8PrDHuDrMzMDuHctirYaE6PvC-MSHy5XE





-- 
-- Description:
-- This function will list all tables that the user account has access to. 
--
-- Input:
-- varchar authorization token or NULL (to read from the previous authorization call)
--
-- Return: 
-- varchar account_url or error message
-- varchar title
-- int     account_id
-- varchar account_name
-- int     profile_id
-- varchar web_property_id
-- varchar currency
-- varchar timezone
-- varchar table_id
--
select gapi_analytics_get_tables(null) over();
-- Example:
--                        account_url                         | update_date |        title        | account_id |    account_name     | profile_id | web_property_id | currency |     timezone     |  table_id   
--------------------------------------------------------------+-------------+---------------------+------------+---------------------+------------+-----------------+----------+------------------+-------------
-- http://www.google.com/analytics/feeds/accounts/ga:391048   | 2000-01-02  | www.example.com     | 270237     | www.example.com     | 391048     | UA-270237-1     | USD      | America/New_York | ga:391048
-- http://www.google.com/analytics/feeds/accounts/ga:1188977  | 2000-01-02  | www.example.com     | 867628     | Example Site        | 1188977    | UA-867628-1     | USD      | America/New_York | ga:1188977
-- http://www.google.com/analytics/feeds/accounts/ga:1912627  | 2000-01-02  | Another API         | 867628     | Example Site        | 1912627    | UA-867628-1     | USD      | America/New_York | ga:1912627
-- http://www.google.com/analytics/feeds/accounts/ga:53107801 | 2000-01-02  | www.example.com     | 27310575   | www.example.com     | 53107801   | UA-27310575-1   | USD      | America/New_York | ga:53107801



--
-- Description:
-- This function pulls analytical data from the google API. Note that Google only allows for 10k records at a time,
-- so you must plan appropriately to insert data into your local tables.
select gapi_analytics_get_analytics(null, '390048', '2011-10-11'::Date, '2011-10-12'::Date) over();




