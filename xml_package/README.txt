
Note:

The reason these are in a singular package is because they all rely on one another.




Submissions:

- Static properties class
  This is an incredibly simple class that allows flexible (node-specific) parameters to be set. This
  really should be a session on global object. See enhancements.
  
- XPATH function
  This function comes in 2 forms, xpath_find and xpath_find all. The first extracts only the first
  match of an xpath, the second matches all (with one per line).
  
- XSLT function
  The XSLT function also comes in 2 forms. The first form simply applies the transform to the XML data.
  The second allows you to put the resulting data into records/rows by providing delimiters.

- HTTP Get
  The http_get will return the test from a URL. This requires static access because of CURL.

- Google API

  This API is meant to be a framework for any and all of Google's API's. Google API's run on XML responses,
  so its simply a matter of applying the right XSL and transforming. In this example I am pulling data
  from the Analytics API, which is the website tracking information Google collects.
  
  API Functions
  -- Authorize
  gapi_analytics_authorize will fetch a token to use for all subsequent requests.
  
  Analytics API
  -- Get Tables
  Lists all the tables available (multiple websites).
  
  -- Get Analytics
  Lists the hits and longitude/latitude for a given date range
  


Bugs:
- The representation of getDate from the input_reader is inconsistent(?) with the output_writer.



Enhancement Requests:
- Create a UDF type that can return multiple rows but does not require the "over ()" clause. 
- Allow session and cluster-wide static variables to be set.
- Allow UDFs to have access to existing environment information such as:
  - The user name of the person executing the function
  - The host that the function is executing on
  - Epoch information
  - Table Meta Data (RO)
- Allow where clauses on UDTs that have named columns.
- Add the "JulianDate" class to the UDF offering.

