/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */
// vim:ru:sm:ts=4:et:tw=0

#include "Vertica.h"
#include "StringParsers.h"
#include <sql.h>
#include <time.h>
#include <sqlext.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <pcrecpp.h>

// To deal with TimeTz and TimestampTz.
// No standard native-SQL representation for these,
// so we ask for them as strings and re-parse them.
// (Ew...)
#include "StringParsers.h"

// To support Vertica SDK before 9.3:
#ifndef SDK_BUILD_ASSERTIONS_H // conveniently doesn't exist before 9.3
#define parseTimeTz(a,b,c,d,e,f) parseTimeTz(a,b,c,d,e)
#define parseTimestampTz(a,b,c,d,e,f) parseTimestampTz(a,b,c,d,e)
#define parseNumeric(a,b,c,d,e,f) parseNumeric(a,b,c,d,e)
#endif

#define MIN_ROWSET  1       // Min rowset value
#define MAX_ROWSET  10000   // Max rowset value
#define DEF_ROWSET  100     // Default rowset
#define MAX_PRELEN  2048    // Max predicate length
#define MAX_PRENUM  10      // Max predicate number
#define REG_CASTRM  R"(::\w+(\(.*?\))*)"
#define REG_ANYMTC  R"(\s*=\s*ANY\s*\(ARRAY\[(.*)\])"
#define REG_ANYREP  R"( IN(\1)"
#define REG_TILDEM  R"(\s*~~\s*)"
#define REG_TILDER  R"( LIKE )"
#define REG_ENDSCO  R"(\s*;\s*$)"
#define REG_QUERYP  R"(^\s*\(*\s*override_query\s*<\s*'\s*(.*)\s*'.*$)"

using namespace Vertica;

static inline TimeADT getTimeFromHMS(uint32 hour, uint8 min, uint8 sec) {
    return getTimeFromUnixTime(sec + min*60 + hour*3600);
}

class ODBCLoader : public UDParser {
public:
    ODBCLoader() : quirks(NoQuirks) {}

    // Maximum length of diagnostic-message text
    // that we can receive from the ODBC driver.
    // Currently must fit on the stack.
    // Diagnostics messages are expected to be short,
    // but the spec does not define a max length.
    static const uint32 MAX_DIAG_MSG_TEXT_LENGTH = 1024;

    // Periodically, we need to break out of our
    // loop fetching data from the remote server and
    // let Vertica do some accounting.  (Mostly check
    // to see if this query has been cancelled.)
    // This knob sets how many times we should try to
    // read another row before doing so.
    // Each such break incurs the cost of a C++
    // virtual function call; this number should be
    // big enough to effectively amortize that cost.
    static const uint32 ROWS_PER_BREAK = 10000;

private:
    // Keep a copy of the information about each column.
    // Note that Vertica doesn't let us safely keep a reference to
    // the internal copy of this data structure that it shows us.
    // But keeping a copy is fine.
    SizedColumnTypes colInfo;

    // ODBC connection/query state
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLSMALLINT numcols;
    SQLULEN nfrows;		// Number of fetched rows
    size_t rowset;

    enum PerDBQuirks {
        NoQuirks = 0,
        Oracle
    };

    PerDBQuirks quirks;

    // MF keeping this to re-use the code in the Fetch loop...
    struct Buf {
        SQLLEN len;
        SQLPOINTER buf;
    };

    //std::vector<Buf> col_data_bufs;
    // MF we're going to use rowset in "column binding format" so we need
    //    for each retrieved column two arrays:
    //    one containing "rowset" results
    //    one containing "rowset" length indicators
    //    resp and len are the pointers to the pointers array.
    SQLPOINTER *resp ;     // result array pointers pointer
    SQLLEN **lenp ;        // length array pointers pointer

    // MF we want to determine Vertica/ODBC types & sizes once and for all...
    BaseDataOID *vtype ;  // Vertica types pointer
    uint32      *stype ;  // Vertica data type size

    StringParsers parser;

    // Gets the Vertica type of the specified column
    VerticaType getVerticaTypeOfCol(SQLSMALLINT colnum) {
        return colInfo.getColumnType(colnum);
    }

    // Gets the ODBC type of the specified column
    SQLSMALLINT getODBCTypeOfCol(SQLSMALLINT colnum) {
        VerticaType type = getVerticaTypeOfCol(colnum);
        switch (type.getTypeOid()) {
        case BoolOID:           return SQL_BIT;
        case Int8OID:           return SQL_BIGINT;
        case Float8OID:         return SQL_DOUBLE;
        case CharOID:           return SQL_CHAR;
        case VarcharOID:        return SQL_LONGVARCHAR;
        case DateOID:           return SQL_DATE;
        case TimeOID:           return SQL_TIME;
        case TimestampOID:      return SQL_TIMESTAMP;
        case TimestampTzOID:    return SQL_VARCHAR;  // Don't know how to deal with timezones in ODBC; just get them as a string and parse it
        case IntervalOID:       return SQL_INTERVAL_DAY_TO_SECOND;
        case IntervalYMOID:     return SQL_INTERVAL_YEAR_TO_MONTH;
        case TimeTzOID:         return SQL_VARCHAR;  // Don't know how to deal with timezones in ODBC; just get them as a string and parse it
        case NumericOID:        return SQL_NUMERIC;
        case BinaryOID:         return SQL_BINARY;
        case VarbinaryOID:      return SQL_LONGVARBINARY;

#ifndef NO_LONG_OIDS
	case LongVarbinaryOID:  return SQL_LONGVARBINARY;
	case LongVarcharOID:    return SQL_LONGVARCHAR;
#endif // NO_LONG_OIDS

        default:                vt_report_error(0, "Unrecognized Vertica type: %s (OID %llu)", type.getTypeStr(), type.getTypeOid()); return SQL_UNKNOWN_TYPE;  // Should never get here; vt_report_error() shouldn't return
        }
    }

    // Gets the ODBC C data-type identifier for the specified column
    SQLSMALLINT getCTypeOfCol(SQLSMALLINT colnum) {
        VerticaType type = getVerticaTypeOfCol(colnum);
        switch (type.getTypeOid()) {
        case BoolOID:           return SQL_C_BIT;
        case Int8OID:           return (quirks != Oracle ? SQL_C_SBIGINT : SQL_C_CHAR);
        case Float8OID:         return SQL_C_DOUBLE;
        case CharOID:           return SQL_C_CHAR;
        case VarcharOID:        return SQL_C_CHAR;
        case DateOID:           return SQL_C_DATE;
        case TimeOID:           return SQL_C_TIME;
        case TimestampOID:      return SQL_C_TIMESTAMP;
        case TimestampTzOID:    return SQL_C_CHAR;  // Don't know how to deal with timezones in ODBC; just get them as a string and parse it
        case IntervalOID:       return SQL_C_INTERVAL_DAY_TO_SECOND;
        case IntervalYMOID:     return SQL_C_INTERVAL_YEAR_TO_MONTH;
        case TimeTzOID:         return SQL_C_CHAR;  // Don't know how to deal with timezones in ODBC; just get them as a string and parse it
        case NumericOID:        return SQL_C_CHAR;
        case BinaryOID:         return SQL_C_BINARY;
        case VarbinaryOID:      return SQL_C_BINARY;

#ifndef NO_LONG_OIDS
	case LongVarbinaryOID:  return SQL_C_BINARY;
	case LongVarcharOID:    return SQL_C_CHAR;
#endif // NO_LONG_OIDS

        default:                vt_report_error(0, "Unrecognized Vertica type %s (OID: %llu)", type.getTypeStr(), type.getTypeOid()); return SQL_UNKNOWN_TYPE;  // Should never get here; vt_report_error() shouldn't return
        }
    }

    // Return the size of the memory allocation needed to store ODBC data for column 'colnum'
    uint32 getFieldSizeForCol(SQLSMALLINT colnum) {
        VerticaType type = getVerticaTypeOfCol(colnum);
        switch (type.getTypeOid()) {
        // Everything fixed-length is the same size in Vertica as ODBC
        case BoolOID: case Int8OID: case Float8OID:
            return type.getMaxSize();

        // Everything string-based is the same size too.
        // Except ODBC may decide that we want a trailing null terminator.
        case CharOID: case VarcharOID: case BinaryOID: case VarbinaryOID:
	
#ifndef NO_LONG_OIDS
	case LongVarbinaryOID: case LongVarcharOID:
#endif // NO_LONG_OIDS

            return type.getMaxSize() + 1;

        // Numeric is a special beast
        // Needs to be size of their header plus our(/their) data
        // Let's be lazy for now and just do their header plus our total size (includes our header)
        // EDIT: Just use strings for Numeric's as well; some DB's seem to have trouble scaling them.
        case NumericOID:
            return 128;

        // Things represented as char's because there's no good native type
        // could be just about any length.
        // So just make something up; hope it's long enough.
        case TimestampTzOID: case TimeTzOID:
            return 80;
            
        // Everything struct-based needs to be the size of that struct
        case DateOID: return sizeof(DATE_STRUCT);
        case TimeOID: return sizeof(TIME_STRUCT);
        case TimestampOID: return sizeof(TIMESTAMP_STRUCT);
        case IntervalOID: case IntervalYMOID: return sizeof(SQL_INTERVAL_STRUCT);
        
        // Otherwise it's a type we don't know about
        default: vt_report_error(0, "Unrecognized Vertica type: %s (OID: %llu)", type.getTypeStr(), type.getTypeOid()); return (uint32)-1;  // Should never get here; vt_report_error() shouldn't return
        }
    }

    void handleReturnCode(ServerInterface &srvInterface, int r, SQLSMALLINT handle_type, SQLHANDLE handle, const char *fn_name) {
        // Check for error codes; retrieve error messages if any
        bool error = false;
        switch (r) {
        case SQL_SUCCESS: return;
            
        case SQL_ERROR: error = true;  // Fall through
        case SQL_SUCCESS_WITH_INFO: {
            SQLCHAR state_rec[6];
            SQLINTEGER native_code;
            SQLCHAR message_text[MAX_DIAG_MSG_TEXT_LENGTH];
            SQLSMALLINT msg_length;
            SQLRETURN r_diag = SQLGetDiagRec(handle_type, handle, 1, &state_rec[0], &native_code,
                                             &message_text[0], MAX_DIAG_MSG_TEXT_LENGTH, &msg_length);

            // No infinite loops!
            // Throw out secondary 'info' messages;
            // if our process for fetching info messages generates info messages,
            // we'll be at it for a while...
            if (r_diag != SQL_SUCCESS && r_diag != SQL_SUCCESS_WITH_INFO) {
                if (error) {
                    vt_report_error(0, "ODBC Error:  Error reported attempting to get the error message for another error!  Unable to display the error message.  Original error was in function %s.", fn_name);
                } else {
                    srvInterface.log("ODBC Warning:  Error reported attempting to get the warning message for another operation!  Unable to display the warning message.  Original warning was in function %s.", fn_name);
                }
            }
            
            const char *truncated = (msg_length > (SQLSMALLINT)MAX_DIAG_MSG_TEXT_LENGTH ? "... (message truncated)" : "");
            
            if (error) {
                vt_report_error(0, "ODBC Error: %s failed with error code %s, native code %d [%s%s]",
                                fn_name, &state_rec[0], (int)native_code, &message_text[0], truncated);
            } else {
                srvInterface.log("ODBC Warning: %s emitted a warning with error code %s, native code %d [%s%s]",
                                 fn_name, &state_rec[0], (int)native_code, &message_text[0], truncated);
            }

            break;
        }
            
        case SQL_INVALID_HANDLE: vt_report_error(0, "ODBC Error: %s failed with internal error SQL_INVALID_HANDLE", fn_name); break;
            
        case SQL_STILL_EXECUTING: vt_report_error(0, "ODBC Error: Synchronous function %s returned SQL_STILL_EXECUTING", fn_name); break;

        case SQL_NO_DATA: vt_report_error(0, "ODBC Error: %s returned SQL_NO_DATA.  Were we cancelled remotely?", fn_name); break;

        case SQL_NEED_DATA: vt_report_error(0, "ODBC Error: %s eturned SQL_NEED_DATA.  Are we calling a stored procedure?  We do not provide parameter values to remote databases; arguments must be hardcoded.", fn_name); break;

// TODO: Apparently this isn't defined but is a valid return code sometimes?
//        case SQL_PARAM_DATA_AVAILABLE: vt_report_error(0, "ODBC Error: Returned SQL_PARAM_DATA_AVAILABLE.  Remote server wants us to handle ODBC Parameters that we didn't set.");

        default: vt_report_error(0,
                                 "ODBC Error: Invalid return code from %s: %d.  " \
                                 "Expected values are %d (SQL_SUCCESS), %d (SQL_SUCCESS_WITH_INFO), %d (SQL_ERROR), " \
                                 "%d (SQL_INVALID_HANDLE), %d (SQL_STILL_EXECUTING), %d (SQL_NO_DATA), or %d (SQL_NEED_DATA).",
                                 fn_name, r, SQL_SUCCESS, SQL_SUCCESS_WITH_INFO, SQL_ERROR,
                                 SQL_INVALID_HANDLE, SQL_STILL_EXECUTING, SQL_NO_DATA, SQL_NEED_DATA);
        }
    }

public:

    virtual StreamState process(ServerInterface &srvInterface, DataBuffer &input, InputState input_state) {
        // Every so many iterations we want to
        // break out and check for Vertica cancel messages
        uint32 iter_counter = 0;

        SQLRETURN fetchRet;
        while (SQL_SUCCEEDED(fetchRet = SQLFetch(stmt))) {
#if LOADER_DEBUG
  srvInterface.log("DEBUG Number of fetched rows/columns = %lu/%d", nfrows, numcols);
#endif
          for (uint32 j = 0; j < (uint32)nfrows; j++) {         // for each fetched row...
            for (SQLUSMALLINT i = 0; i < numcols; i++) {        // for each column...
#if LOADER_DEBUG
  srvInterface.log("DEBUG nfrows=%u j=%u i=%d lenp[%d][%d]=%ld", (uint32)nfrows, j, i, i, j, lenp[i][j]);
#endif

                // MF allocate & set Buf struct so we can re-use the original code in the Fetch loop...
                Buf data ;

                // MF SQLPOINTER is a (void *) so it would generate an arithmetic warning if not casted
                data.buf = (SQLPOINTER)( (uint8_t *)resp[i] + stype[i] * j ) ;
                data.len = lenp[i][j] ;

                std::string rejectReason = "unrecognized syntax from remote database";
                
                // Null's are easy
                // except when they're not due to typecast mismatch fun
                if ((int)data.len == (int)SQL_NULL_DATA) { writer->setNull(i); }
                else switch (vtype[i]) {
                        
                        // Simple fixed-length types
                        // Let C++ figure out how to convert from, ie., SQLBIGINT to vint.
                        // (Both are native C++ types with appropriate meanings, so hopefully this will DTRT.)
                        // (In most implementations they are probably the same type so this is a no-op.)
                    case BoolOID:           writer->setBool(i, (*(SQLCHAR*)data.buf == SQL_TRUE ? VTrue : VFalse)); break;
                    case Int8OID:
                        if (quirks != Oracle) {
                            writer->setInt(i, *(SQLBIGINT*)data.buf);
                        } else {
                            // Oracle doesn't support int64 as a type.
                            // So we get the data as a string and parse it to an int64.
                            if (data.len == SQL_NTS) { writer->setInt(i, vint_null); }
                            else { writer->setInt(i, (vint)atoll((char*)data.buf)); }
                        } 
                        break;
                    case Float8OID:         writer->setFloat(i, *(SQLDOUBLE*)data.buf); break;

                        // Strings (all the same representation)
                    case CharOID: case BinaryOID:
                    case VarcharOID: case VarbinaryOID:

#ifndef NO_LONG_OIDS
                    case LongVarcharOID: case LongVarbinaryOID:
#endif // NO_LONG_OIDS

                        if (data.len == SQL_NTS) { data.len = strnlen((char*)data.buf, getFieldSizeForCol(i)); }
                        writer->getStringRef(i).copy((char*)data.buf, data.len);
                        break;

                        // Date/Time functions that work in reasonably direct ways
                    case DateOID: {
                        SQL_DATE_STRUCT &s = *(SQL_DATE_STRUCT*)data.buf;
                        struct tm d = {0,0,0,s.day,s.month-1,s.year-1900,0,0,-1};
                        time_t unixtime = mktime(&d);
                        writer->setDate(i, getDateFromUnixTime(unixtime + d.tm_gmtoff));
                        break;
                    }
                    case TimeOID: {
                        SQL_TIME_STRUCT &s = *(SQL_TIME_STRUCT*)data.buf;
                        writer->setTime(i, getTimeFromHMS(s.hour, s.minute, s.second));
                        break;
                    }
                    case TimestampOID: {
                        SQL_TIMESTAMP_STRUCT &s = *(SQL_TIMESTAMP_STRUCT*)data.buf;
                        struct tm d = {s.second,s.minute,s.hour,s.day,s.month-1,s.year-1900,0,0,-1};
                        time_t unixtime = mktime(&d);
                        // s.fraction is in nanoseconds; Vertica only does microsecond resolution
                        // setTimestamp() wants time since epoch localtime.
                        writer->setTimestamp(i, getTimestampFromUnixTime(unixtime + d.tm_gmtoff) + s.fraction/1000);
                        break;
                    }
                        
                        // Date/Time functions that require string-parsing
                    case TimeTzOID: {
                        // Hacky workaround:  Some databases (ie., us) send the empty string instead of NULL here
                        if (((char*)data.buf)[0] == '\0') { writer->setNull(i); break; }
                        TimeADT t = 0;
                        
                        if (!parser.parseTimeTz((char*)data.buf, (size_t)data.len, i, t, getVerticaTypeOfCol(i), rejectReason)) {
                            vt_report_error(0, "Error parsing TimeTz: '%s' (%s)", (char*)data.buf, rejectReason.c_str());  // No rejected-rows for us!  Die on failure.
                        }
                        writer->setTimeTz(i,t);
                        break;
                    }
                        
                    case TimestampTzOID: {
                        // Hacky workaround:  Some databases (ie., us) send the empty string instead of NULL here
                        if (((char*)data.buf)[0] == '\0') { writer->setNull(i); break; }
                        TimestampTz t = 0;
                        if (!parser.parseTimestampTz((char*)data.buf, (size_t)data.len, i, t, getVerticaTypeOfCol(i), rejectReason)) {
                            vt_report_error(0, "Error parsing TimestampTz: '%s' (%s)", (char*)data.buf, rejectReason.c_str());  // No rejected-rows for us!  Die on failure.
                        }
                        writer->setTimestampTz(i,t);
                        break;
                    }
                        
                    case IntervalOID: {
                        SQL_INTERVAL_STRUCT &intv = *(SQL_INTERVAL_STRUCT*)data.buf;
                        
                        // Make sure we know what we're talking about
                        if (intv.interval_type != SQL_IS_DAY_TO_SECOND) {
                            vt_report_error(0, "Error parsing Interval:  Is type %d; expecting type 10 (SQL_IS_HOUR_TO_SECOND)", (int)intv.interval_type);
                        }

                        // Vertica Intervals are stored as durations in microseconds
                        Interval ret = ((intv.intval.day_second.day*usPerDay)
                                        + (intv.intval.day_second.hour*usPerHour)
                                        + (intv.intval.day_second.minute*usPerMinute)
                                        + (intv.intval.day_second.second*usPerSecond)
                                        + (intv.intval.day_second.fraction/1000)) // Fractions are in nanoseconds; we do microseconds
                            * (intv.interval_sign == SQL_TRUE ? -1 : 1); // Apply the sign bit
                        
                        writer->setInterval(i, ret);
                        break;   
                    }

                    case IntervalYMOID: {
                        SQL_INTERVAL_STRUCT &intv = *(SQL_INTERVAL_STRUCT*)data.buf;
                        
                        // Make sure we know what we're talking about
                        if (intv.interval_type != SQL_IS_YEAR_TO_MONTH) {
                            vt_report_error(0, "Error parsing Interval:  Is type %d; expecting type 7 (SQL_IS_YEAR_TO_MONTH)", (int)intv.interval_type);
                        }

                        // Vertica Intervals are stored as durations in months
                        Interval ret = ((intv.intval.year_month.year*MONTHS_PER_YEAR)
                                        + (intv.intval.year_month.month))
                            * (intv.interval_sign == SQL_TRUE ? -1 : 1); // Apply the sign bit
                        
                        writer->setInterval(i, ret);
                        break;   
                    }
                        
                        // TODO:  Sort out the binary ODBC Numeric format
                        // and the abilities of various DB's to cast to/from it on demand;
                        // make this use the native binary format and cast/convert as needed.
                    case NumericOID: {
                        // Hacky workaround:  Some databases may send the empty string instead of NULL here
                        if (((char*)data.buf)[0] == '\0') { writer->setNull(i); break; }
                        if (!parser.parseNumeric((char*)data.buf, (size_t)data.len, i, writer->getNumericRef(i), getVerticaTypeOfCol(i), rejectReason)) {
                            vt_report_error(0, "Error parsing Numeric: '%s' (%s)", (char*)data.buf, rejectReason.c_str());  // No rejected-rows for us!  Die on failure.
                        }
                        break;
                    }

                    default: vt_report_error(0, "Unrecognized Vertica type %s (OID %llu)", getVerticaTypeOfCol(i).getTypeStr(), getVerticaTypeOfCol(i).getTypeOid());
                }
            }

            writer->next();

            if (++iter_counter == ROWS_PER_BREAK) {
                // Periodically yield and let upstream do its thing
                return KEEP_GOING;
            }
          }
        }

        // If SQLFetch() failed for some reason, report it
        // But, SQLFetch() is allowed to return SQL_NO_DATA from time to time.
        // TODO:  Maybe be smarter if we're getting SQL_NO_DATA forever / apparently stuck?
        if (fetchRet != SQL_NO_DATA) {
            handleReturnCode(srvInterface, fetchRet, SQL_HANDLE_STMT, stmt, "SQLFetch()");
        }

        return DONE;
    }

    void setQuirksMode(ServerInterface &srvInterface, SQLHDBC &dbc) {
        // Set the quirks mode based on the DB name
        SQLSMALLINT len;
        char buf[32];
        memset(&buf[0], 0, 32);

        SQLGetInfo(dbc, SQL_SERVER_NAME, buf,
                   sizeof(buf) - 1 /* leave a byte for null-termination */,
                   &len);
        srvInterface.log("ODBC Loader: Connecting to server of type '%s'", buf);

        std::string db_type(buf, len);
        if (db_type == "ORCL") {
            quirks = Oracle;
        }
    }

    virtual void setup(ServerInterface &srvInterface, SizedColumnTypes &returnType) {
        // Capture our column types
        colInfo = returnType;
		bool src_rfilter = true ;       // Rows filtering flag
        bool src_cfilter = true ;       // Column filtering flag
        bool oq_flag = false ;          // Query Ovverride flag
        std::string connect = "" ;      // Connect string
        std::string query = "" ;        // Remote system query string
        std::string predicates = "" ;   // Predicates

        // Read User defined Session parameters 
        if (srvInterface.getUDSessionParamReader("library").containsParameter("src_rfilter")) {
            src_rfilter = ( srvInterface.getUDSessionParamReader("library").getStringRef("src_rfilter").str() == "f" ) ? false : true ;
        } else if (srvInterface.getParamReader().containsParameter("src_rfilter")) {
            src_rfilter = srvInterface.getParamReader().getBoolRef("src_rfilter") ;
        }
        if (srvInterface.getUDSessionParamReader("library").containsParameter("override_query")) {
            query = srvInterface.getUDSessionParamReader("library").getStringRef("override_query").str() ;
        } else {
            query = srvInterface.getParamReader().getStringRef("query").str();
        }
        if (srvInterface.getUDSessionParamReader("library").containsParameter("src_cfilter")) {
            src_cfilter = ( srvInterface.getUDSessionParamReader("library").getStringRef("src_cfilter").str() == "f" ) ? false : true ;
        } else if (srvInterface.getParamReader().containsParameter("src_cfilter")) {
            src_cfilter = srvInterface.getParamReader().getBoolRef("src_cfilter") ;
        }
        connect = srvInterface.getParamReader().getStringRef("connect").str();
#if LOADER_DEBUG
  srvInterface.log("DEBUG Initial connect=<%s>", connect.c_str());
  srvInterface.log("DEBUG Initial query=<%s>", query.c_str());
  srvInterface.log("DEBUG SETUP src_rfilter is %s", src_rfilter ? "true" : "false" );
  srvInterface.log("DEBUG SETUP src_cfilter is %s", src_cfilter ? "true" : "false" );
#endif

        // Check Connection string, Query and Rowset "public" parameters

        // Check "rowset" parameter
        if (srvInterface.getParamReader().containsParameter("rowset")) {
            vint rowset_param = srvInterface.getParamReader().getIntRef("rowset") ;
            if ( rowset_param < MIN_ROWSET || rowset_param > MAX_ROWSET ) 
                vt_report_error(0, "Error:  Invalid rowset=%zd. Permitted values between %d and %d", rowset_param, MIN_ROWSET, MAX_ROWSET);
            else
                rowset = (size_t) rowset_param ;
        } else {
                rowset = DEF_ROWSET ;	// use default if not set
        }
  
        // Check "hidden" parameters __pred_#__ to filter out rows
        char pred[16] ;
        for ( unsigned int k = 0, l = 0 ; k < MAX_PRENUM ; k++ ) {
            snprintf(pred, sizeof(pred), "__pred_%u__", k ) ;
            if (srvInterface.getParamReader().containsParameter(pred)) {
                std::string mpred = srvInterface.getParamReader().getStringRef(pred).str() ;
#if LOADER_DEBUG
  srvInterface.log("DEBUG predicate [%s] length=%zu, string=<%s>", pred, strlen(mpred.c_str()), mpred.c_str());
#endif
                if ( pcrecpp::RE(REG_QUERYP, pcrecpp::RE_Options(PCRE_DOTALL)).FullMatch(mpred) ) {
                    pcrecpp::RE(REG_QUERYP, pcrecpp::RE_Options(PCRE_DOTALL)).GlobalReplace("\\1", &mpred) ;
                    query = mpred ;
                    oq_flag = true ;
#if LOADER_DEBUG
  srvInterface.log("DEBUG new query length=%zu, new query string=<%s>",query.length(),  query.c_str());
#endif
                } else if ( src_rfilter ) {
                        pcrecpp::RE(REG_ANYMTC).GlobalReplace(REG_ANYREP, &mpred) ;     // to replace ANY(ARRAY()) with IN()
                        pcrecpp::RE(REG_TILDEM).GlobalReplace(REG_TILDER, &mpred) ;     // to replace ~~ with LIKE
                        if ( l++ ) 
                            predicates += " AND " + mpred ;
                        else
                            predicates += " WHERE " + mpred ;
                }
            } else {
                break ;
            }
        }

        // Remove ending semicolon from "query" (if any)
        pcrecpp::RE(REG_ENDSCO).GlobalReplace("", &query) ;

        // Check "hidden" parameters __query_col_name__ and __query_col_idx__ to filter out columns
        if ( src_cfilter ) {
           if (srvInterface.getParamReader().containsParameter("__query_col_name__")) {
               if (srvInterface.getParamReader().containsParameter("__query_col_idx__")) {
                   int ncols = (int)colInfo.getColumnCount() ;
                   std::stringstream ss_cols(srvInterface.getParamReader().getStringRef("__query_col_name__").str());
                   std::stringstream ss_idx(srvInterface.getParamReader().getStringRef("__query_col_idx__").str());
                   std::string tk_col, tk_idx, slist="" ;
                   int k = 0;
       
                   while (std::getline(ss_idx, tk_idx, ',')) {
                       std::getline(ss_cols, tk_col, ',');
                       for (  ; k < atoi(tk_idx.c_str()) ; k++ )
                           slist += k ? ", NULL" : "NULL" ;
                       if ( k )
                           slist += "," ;
                       slist += ( tk_col == "override_query" ) ? "'.' AS override_query" : tk_col ;
                       k++ ;
                   }
                   // MF to remove Vertica casts (::<data_type>)
#if LOADER_DEBUG
  srvInterface.log("DEBUG BEFORE GlobalReplace slist=<%s>", slist.c_str());
#endif
                   pcrecpp::RE(REG_CASTRM).GlobalReplace("", &slist) ;
#if LOADER_DEBUG
  srvInterface.log("DEBUG AFTER GlobalReplace slist=<%s> (length=%zu)", slist.c_str(), slist.length());
#endif

                   // Add NULLs for the remaining columns (if select list is not empty)
                   if ( slist.length() ) {
                       for (  ; k < ncols ; k++ ) {
                           slist += k ? ", NULL" : "NULL" ;
                       }
                   } else {
                       slist = "*" ;
                   }
                   query = "SELECT " + slist + " FROM ( " + query + " ) sq" ;
#if LOADER_DEBUG
  srvInterface.log("DEBUG FINAL slist=%s", slist.c_str());
#endif
               } else {
                   query = "SELECT " +
                           srvInterface.getParamReader().getStringRef("__query_col_name__").str() +
                           " FROM ( " +
                           query  +
                           " ) sq" ;
               }
            }
        } else {
            query = oq_flag ? "SELECT '.' AS override_query, sq.* FROM ( " + query + " ) sq" : "SELECT  * FROM ( " + query + " ) sq" ;
        }

        // Append predicates to outer SELECT
        if ( src_rfilter )
            query += predicates ;
  
        SQLRETURN r;
#if LOADER_DEBUG
  srvInterface.log("DEBUG query=%s", query.c_str());
  srvInterface.log("DEBUG rowset=%zu", rowset);
#endif

        // Establish an ODBC connection
        r = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
        handleReturnCode(srvInterface, r, SQL_HANDLE_ENV, env, "SQLAllocHandle()");

        r = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
        handleReturnCode(srvInterface, r, SQL_HANDLE_ENV, env, "SQLSetEnvAttr()");

        r = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
        handleReturnCode(srvInterface, r, SQL_HANDLE_DBC, dbc, "SQLAllocHandle(SQL_HANDLE_DBC)");

        r = SQLDriverConnect(dbc, NULL, (SQLCHAR*)connect.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
        handleReturnCode(srvInterface, r, SQL_HANDLE_DBC, dbc, "SQLDriverConnect()");

        // We have a connection; now we know enough to figure out
        // which DB we have to customize to
        setQuirksMode(srvInterface, dbc);

        r = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
        handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLAllocHandle(SQL_HANDLE_STMT)");

        // Set bind by column statement attribute:
        r = SQLSetStmtAttr(stmt, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)SQL_BIND_BY_COLUMN, 0) ;
        handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLSetStmtAttr(SQL_ATTR_ROW_BIND_TYPE)");

        // Set ROW_ARRAY_SIZE statement attribute:
        r = SQLSetStmtAttr(stmt, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)rowset, 0) ;
        handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLSetStmtAttr(SQL_ATTR_ROW_ARRAY_SIZE)");

        // Set ROWS_FETCHED_PTR statement attribute:
        r = SQLSetStmtAttr(stmt, SQL_ATTR_ROWS_FETCHED_PTR, &nfrows, 0) ;
        handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLSetStmtAttr(SQL_ATTR_ROWS_FETCHED_PTR)");

        r = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLExecDirect()");

        r = SQLNumResultCols(stmt, &numcols);
        handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLNumResultCols()");

        if ((ssize_t)numcols != (ssize_t)colInfo.getColumnCount()) {
            vt_report_error(0, "Expected %d columns; got %d from the remote database", (int)colInfo.getColumnCount(), (int)numcols);
        }

        // Allocate space for result & length array pointers
        resp = (SQLPOINTER *)srvInterface.allocator->alloc(numcols * sizeof(SQLPOINTER)) ;
        lenp = (SQLLEN **)srvInterface.allocator->alloc(numcols * sizeof(SQLLEN *)) ;

        // Allocate space for Vertica data types OID and size
        vtype = (BaseDataOID *)srvInterface.allocator->alloc(numcols * sizeof(BaseDataOID)) ;
        stype = (uint32 *)srvInterface.allocator->alloc(numcols * sizeof(uint32)) ;

        // Set up column-data buffers
        // Bind to the columns in question
        for (SQLSMALLINT i = 0; i < numcols; i++) {
            vtype[i] = getVerticaTypeOfCol(i).getTypeOid();
            stype[i] = getFieldSizeForCol(i) ;
#if LOADER_DEBUG
  srvInterface.log("DEBUG i=%d rowset=%zu stype[i]=%d", i, rowset, stype[i]);
#endif
            resp[i] = (SQLPOINTER)srvInterface.allocator->alloc(stype[i] * rowset);
            lenp[i] = (SQLLEN *)srvInterface.allocator->alloc(sizeof(SQLLEN) * rowset);

            r = SQLBindCol(stmt, i+1, getCTypeOfCol(i), resp[i], stype[i], lenp[i]);
            handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLBindCol()");
        }
    }

    virtual void destroy(ServerInterface &srvInterface, SizedColumnTypes &returnType) {
        // Try to free even on error, to minimize the risk of memory leaks.
        // But do check for errors in the end.
        SQLRETURN r_disconnect = SQLDisconnect(dbc);
        SQLRETURN r_free_dbc = SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLRETURN r_free_env = SQLFreeHandle(SQL_HANDLE_ENV, env);

        handleReturnCode(srvInterface, r_disconnect, SQL_HANDLE_DBC, dbc, "SQLDisconnect()");
        handleReturnCode(srvInterface, r_free_dbc, SQL_HANDLE_DBC, dbc, "SQLFreeHandle(SQL_HANDLE_DBC)");
        handleReturnCode(srvInterface, r_free_env, SQL_HANDLE_ENV, env, "SQLFreeHandle(SQL_HANDLE_ENV)");
    }
};

class ODBCLoaderFactory : public ParserFactory {
public:
    virtual void plan(ServerInterface &srvInterface,
            PerColumnParamReader &perColumnParamReader,
            PlanContext &planCtxt) {
        if (!srvInterface.getParamReader().containsParameter("connect")) {
            vt_report_error(0, "Error:  ODBCConnect requires a 'connect' string containing ODBC connect information (at minimum, 'DSN=myDSN' for some myDSN in odbc.ini)");
        }
        if (!srvInterface.getParamReader().containsParameter("query")) {
            vt_report_error(0, "Error:  ODBCConnect requires a 'query' string, the query to execute on the remote system");
        }
    }

    virtual UDParser* prepare(ServerInterface &srvInterface,
            PerColumnParamReader &perColumnParamReader,
            PlanContext &planCtxt,
            const SizedColumnTypes &returnType)
    {
        return vt_createFuncObj(srvInterface.allocator, ODBCLoader);
    }

    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes) {
        parameterTypes.addVarchar(65000, "connect");
        parameterTypes.addVarchar(65000, "query");
        parameterTypes.addVarchar(65000, "__query_col_name__");
        parameterTypes.addVarchar(65000, "__query_col_idx__");
        char pred[16] ;
	    for ( unsigned int k = 0 ; k < MAX_PRENUM ; k++ ) {
                snprintf(pred, sizeof(pred), "__pred_%u__", k ) ;
                parameterTypes.addVarchar(MAX_PRELEN, pred);
        }
        parameterTypes.addInt("rowset");
        parameterTypes.addBool("src_rfilter");
        parameterTypes.addBool("src_cfilter");
    }
};

RegisterFactory(ODBCLoaderFactory);


// ODBCLoader does all the real work.
// This is basically a stub that tells Vertica to run the query on the current node only.
class ODBCSource : public UDSource {
public:
    virtual StreamState process(ServerInterface &srvInterface, DataBuffer &output) {
        if (output.size < 1) return OUTPUT_NEEDED;
        output.offset = 1;
        return DONE;
    }
};

class ODBCSourceFactory : public SourceFactory {
public:

    virtual void plan(ServerInterface &srvInterface,
            NodeSpecifyingPlanContext &planCtxt) {
        // Make the query only run on the current node.
        std::vector<std::string> nodes;
        nodes.push_back(srvInterface.getCurrentNodeName());
        planCtxt.setTargetNodes(nodes);
    }


    virtual std::vector<UDSource*> prepareUDSources(ServerInterface &srvInterface,
            NodeSpecifyingPlanContext &planCtxt) {
        std::vector<UDSource*> retVal;
        retVal.push_back(vt_createFuncObj(srvInterface.allocator, ODBCSource));
        return retVal;
    }
};
RegisterFactory(ODBCSourceFactory);

// Library Metadata
RegisterLibrary (
    "Vertica Team",
    __DATE__,
    "0.10.6",
    "v11.x.x",
    "TBD",
    "With this loader Vertica can COPY and SELECT from any ODBC data source",
    "", 
    ""  
);      
