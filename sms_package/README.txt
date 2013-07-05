A User-Defined Scalar Function providing SMS services (sending text messages to designated phone numbers)

-------------------------------
INTRODUCTION
-------------------------------
This package submission is to achieve the following goals:

1. Demonstrate that the functionality a Vertica UDx package can go
beyond data processing. One key value that the Vertica UDx framework
delivers is application integration, delivering value not only to SQL
and BI users, but to new groups of users on the mobile and web as
well.

2. Demonstrates that the input and output of a UDx function need not
be restricted to Vertica tables. Instead a UDx function can invoke web
services to obtain additional input, and trigger domain specific
actions.

3. Deliver a concrete service to Vertica users -- sending a text
message with customized content. Example use cases are as follows 

  a) Notification after a database job finishes: After submitting a
  potentially long running job J (e.g. loading a large amount of
  data), user Jennifer can append to J an SMS function call in the
  same Vertica session, which sends a text notification to
  herself. Since the SQL statements are executed sequentially within
  the same session, Jennifer will be notified exactly when J finishes.

  b) Collaboration: Vertica DBA Tom has a quick question to ask to his
  colleague Sandy. Instead of pulling out his mobile phone, for speed
  and simplicity Tom invokes a Vertica function to sends an SMS to
  Sandy, without having to leave the Vertica interface.

  c) Application integration: Vertica applications can leverage this
  function to deliver SMS services to their end users. For example,
  providing regular updates to the admin users on the system resource
  utilization.

-------------------------------
INSTRUCTIONS ON SETUP AND USE
-------------------------------

- Step #1: Install libcurl and openssl packages on the Vertica server
  machine. Vertica uses these libraries to make HTTP web service
  calls.

- Step #2: There are a number of SMS web services available, many of
  which can be integrated with the Vertica UDx. For illustration
  purposes, one of such services, twilio (http://www.twilio.com), is
  chosen for this Vertica UDx function. To use this function, first
  register a free twilio account, and then follow the instructions in
  examples/SMS.sql.


- Step #3: Install.
  To install a precompiled binary, run the SQL statements in
   ddl/install.sql
  or, to uninstall,
   ddl/uninstall.sql

  Note that the SQL statements assume that you have copied this package 
  to a node in your cluster and are running them from there.


To install from source, these make commands are supported:
  - make: compile the function.

  - make install: install the function into the Vertica instance on
    the local machine. Alternately, you can find the DDL that 'make
    install' uses in: src/ddl/

  - make test: test this function via VSQL. Make sure you read the
    customize examples/SMS.sql first.

  - make uninstall: uninstall this function.

-------------------------------
LICENSE
-------------------------------

Please see LICENSE.txt
