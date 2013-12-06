-------------------------------
INTRODUCTION
-------------------------------

This extension package contains a function that sends emails to specified 
addresses. You can include a subject line and provide an inline message as the 
body, or a text file whose content is printed into the body. 

-------------------------------
INSTALLING / UNINSTALLING
-------------------------------

You can install by copying `build/myudx.so' to your Vertica cluster, then 
running the following SQL statements:

CREATE LIBRARY EMAILLib as 'build/myudx.so';"
CREATE FUNCTION EMAIL as language 'C++' name 'EMAILFactory' library EMAILLib;

To uninstall, you can run:

DROP LIBRARY EMAILLib CASCADE;

(The 'CASCADE' will cause the EMAIL function to be dropped as well.)

Alternatively, assuming vsql is in your path, just do:

$ make install
$ make uninstall

Or, if vsql not in your standard path, edit the makefile.

-------------------------------
BUILDING
-------------------------------

To build from source:

$ make


-------------------------------
USAGE
-------------------------------

This function calls mail. If the email doesn't send, check mail's 
configurations.

SELECT EMAIL('address', 'subject', 'boolean', 'message/filename');

Arguments: 
address - 	   The email address, or location of emails.
subject - 	   The subject of the email.
boolean - 	   A varchar indicating if the fourth argument is a file name, 
	  	   or an inline message. 'True' or '1' indicates a text file 
	  	   name. 'False' or '0' is false, indicating an inline message.
message/filename - The inline message or the name of the file to be included in
		   the body of the email. 

See the examples file for samples of usage.

-------------------------------
LICENSE
-------------------------------

See LICENSE.txt
