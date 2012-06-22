Vertica Extension Packages
Copyright 2011 Vertica, an HP Company

This repository contains extension code (UDx) code to extend the
processing capabilities of the Vertica Analytic Database. 

You need the Vertica SDK to compile these programs and you need the
Vertica Analytic Database to run them.

All code is provided under the license found in LICENSE.txt

IMPORTANT: If you wish to contribute anything to this repository, in
order for us to accept your pull request you MUST sign and send a copy
of the appropriate Contributor License Agreement to Vertica
(contribs@vertica.com):

license/PersonalCLA.pdf: If you are contributing for yourself
license/CorporateCLA.pdf: If you are contributing on behalf of your company

*******************
Submission guidelines:
*******************

Any submission to this repository should contain:

1) A few sentences describing what your function does, written in
English.

2) The complete source code and build commands to build your
function. Ideally you would add your function to one of the existing
packages (listed below), or you can create your own package using the
contents of template_package as a guide.

3) REQUIRED: SQL example/tests showing your function in action. This
script should have sample input, and sample queries that demonstrate
how to use your function. The examples should be placed in
package_dir/examples


*******************
Current packages
*******************

string_functions: 
  Description: string manipulation functions
  Examples: EditDistance, StringTokenizer

web_functions
  Description: web log analysis functions
  Examples: ApacheParser, URIExtractor

compatlib_functions: 
  Description: Oracle compatibility functions (such as CONNECT_BY, TRANSPOSE)

encryption_package: 
  Description: Field level encryption functions

shell_load_package:
  Description: Have COPY fetch data by running commands rather than opening files

template_package: 
  Description: template for new packages

tools:
  scripts to create and install packages (in process)
  Example: ./install_package -p <pkg_name>

*******************
Future packages that need their build system cleaned up
*******************

hive_functions: 
  Description: Partial implementation of UDTs that are shipped with apache Hiv
  Examples: get_json_object, xpathint, etc.
  https://cwiki.apache.org/confluence/display/Hive/LanguageManual+UDF
  Status: significant usefulness
