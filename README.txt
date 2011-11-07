Vertica Extension Packages repository. 

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
how to use your function.


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

template_package: 
  Description: template for new packages

hive_functions: 
  Description: Partial implementation of UDTs that are shipped with apache Hiv
  Examples: get_json_object, xpathint, etc.
  https://cwiki.apache.org/confluence/display/Hive/LanguageManual+UDF
  Status: significant usefulness

tools:
  scripts to create and install packages (in process)
  Example: ./install_package -p <pkg_name>

