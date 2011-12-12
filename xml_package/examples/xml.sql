--------------------------------------------------------
-- xml.sql
--
-- Description: Examples of xstl and xpath.
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

select xslt_apply('<?xml version="1.0" encoding="UTF-8"?><doc><person><name>James</name></person></doc>', 
   '<?xml version="1.0" encoding="UTF-8"?>
               <xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
                   <xsl:output method="text" version="1.0" encoding="iso-8859-1" indent="no"/>
                   <xsl:template match="/">
                       <xsl:for-each select="*[name() = ''doc'']/*[name() = ''person'']">
                               <xsl:value-of select="*"/>
                           <xsl:text>n</xsl:text>
                       </xsl:for-each>
                   </xsl:template>
               </xsl:stylesheet>');



select xslt_apply_split('<?xml version="1.0" encoding="UTF-8"?>	<doc>
    <person>
        <first_name>James</first_name>
        <last_name>Smith</last_name>
    </person>
    <person>
        <first_name>Ann</first_name>
        <last_name>Hardy</last_name>
    </person>
</doc>', 
   '<?xml version="1.0" encoding="UTF-8"?>
               <xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
				    <xsl:output method="text" version="1.0" encoding="iso-8859-1" indent="no"/>
				    <xsl:template match="/">
				        
				        <xsl:for-each select="doc/person">
				            <xsl:value-of select="first_name"/>|<xsl:value-of select="last_name"/>
				            <xsl:text>
</xsl:text>
				        </xsl:for-each>
				        
				    </xsl:template>

               </xsl:stylesheet>', '|', chr(10))
    over ();

select xpath_find('<?xml version="1.0" encoding="UTF-8"?><doc><person><name>James</name><name>Smith</name></person></doc>', 
	'/doc/person/name', 1);

select xpath_find('<?xml version="1.0" encoding="UTF-8"?><doc><person><name>James</name><name>Smith</name></person></doc>', 
	'/doc/person/name', 2);

select xpath_find('<?xml version="1.0" encoding="UTF-8"?><doc><person><name>James</name><name>Smith</name></person></doc>', 
	'/doc/person/name', 3);

select xpath_find_all('<?xml version="1.0" encoding="UTF-8"?><doc><person><name>James</name><name>Smith</name></person></doc>', 
	'/doc/person/name') over ();
