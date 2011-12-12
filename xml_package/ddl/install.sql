--------------------------------------------------------
-- install.sql
--
-- Description: This file will install the library into Vertica and will create functions
-- for the respective areas that have been developed.
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


select version();

\set libname 'XMLLib'
\set libfile '\''`pwd`'/lib/XMLPackage.so\''

CREATE LIBRARY :libname AS :libfile;

-- Vertica Properties Functions
CREATE FUNCTION vertica_get_property AS LANGUAGE 'C++' NAME 'VerticaPropGetFactory' LIBRARY :libname;
CREATE FUNCTION vertica_set_property AS LANGUAGE 'C++' NAME 'VerticaPropSetFactory' LIBRARY :libname;
CREATE TRANSFORM FUNCTION vertica_list_properties AS LANGUAGE 'C++' NAME 'VerticaListPropFactory' LIBRARY :libname;

-- XML Functions
CREATE FUNCTION xpath_find AS LANGUAGE 'C++' NAME 'XPathNumFactory' LIBRARY :libname;
CREATE TRANSFORM FUNCTION xpath_find_all AS LANGUAGE 'C++' NAME 'XPathFindFactory' LIBRARY :libname;
CREATE FUNCTION xslt_apply AS LANGUAGE 'C++' NAME 'XSLTApplyFactory' LIBRARY :libname;
CREATE TRANSFORM FUNCTION xslt_apply_split AS LANGUAGE 'C++' NAME 'XSLTApplySplitFactory' LIBRARY :libname;

-- Http Functions
CREATE FUNCTION http_get AS LANGUAGE 'C++' NAME 'HttpGetFactory' LIBRARY :libname;

-- Google API Functions
CREATE FUNCTION gapi_analytics_authorize AS LANGUAGE 'C++' NAME 'GAPIAnalyticsAuthorizationFactory' LIBRARY :libname;
CREATE TRANSFORM FUNCTION gapi_analytics_get_analytics AS LANGUAGE 'C++' NAME 'GAPIAnalyticsGetAnalyticsFactory' LIBRARY :libname;
CREATE TRANSFORM FUNCTION gapi_analytics_get_tables AS LANGUAGE 'C++' NAME 'GAPIAnalyticsGetTablesFactory' LIBRARY :libname;
