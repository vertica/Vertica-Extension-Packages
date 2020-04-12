/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */

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

// To deal with TimeTz and TimestampTz.
// No standard native-SQL representation for these,
// so we ask for them as strings and re-parse them.
// (Ew...)
#include "StringParsers.h"

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

    enum PerDBQuirks {
        NoQuirks = 0,
        Oracle
    };

    PerDBQuirks quirks;

    struct Buf {
        SQLLEN len;
        SQLPOINTER buf;
    };

    std::vector<Buf> col_data_bufs;

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
            for (SQLUSMALLINT i = 0; i < numcols; i++) {

                Buf data = col_data_bufs[i];
                std::string rejectReason;
                
                // Null's are easy
                // except when they're not due to typecast mismatch fun
                if ((int)data.len == (int)SQL_NULL_DATA) { writer->setNull(i); }
                else switch (getVerticaTypeOfCol(i).getTypeOid()) {
                        
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
                        struct tm d = {0,0,0,s.day,s.month-1,s.year-1900};
                        writer->setDate(i, getDateFromUnixTime(mktime(&d)));
                        break;
                    }
                    case TimeOID: {
                        SQL_TIME_STRUCT &s = *(SQL_TIME_STRUCT*)data.buf;
                        writer->setTime(i, getTimeFromHMS(s.second, s.minute, s.hour));
                        break;
                    }
                    case TimestampOID: {
                        SQL_TIMESTAMP_STRUCT &s = *(SQL_TIMESTAMP_STRUCT*)data.buf;
                        struct tm d = {s.second,s.minute,s.hour,s.day,s.month-1,s.year-1900};
                        // s.fraction is in nanoseconds; Vertica only does microsecond resolution
                        writer->setTimestamp(i, getTimestampFromUnixTime(mktime(&d)) + s.fraction/1000);
                        break;
                    }
                        
                        // Date/Time functions that require string-parsing
                    case TimeTzOID: {
                        // Hacky workaround:  Some databases (ie., us) send the empty string instead of NULL here
                        if (((char*)data.buf)[0] == '\0') { writer->setNull(i); break; }
                        TimeADT t = 0;
                        
                        if (!parser.parseTimeTz((char*)data.buf, (size_t)data.len, i, t, getVerticaTypeOfCol(i), rejectReason)) {
                            vt_report_error(0, "Error parsing TimeTz: '%s' (unrecognized syntax from remote database)", (char*)data.buf);  // No rejected-rows for us!  Die on failure.
                        }
                        writer->setTimeTz(i,t);
                        break;
                    }
                        
                    case TimestampTzOID: {
                        // Hacky workaround:  Some databases (ie., us) send the empty string instead of NULL here
                        if (((char*)data.buf)[0] == '\0') { writer->setNull(i); break; }
                        TimestampTz t = 0;
                        if (!parser.parseTimestampTz((char*)data.buf, (size_t)data.len, i, t, getVerticaTypeOfCol(i), rejectReason)) {
                            vt_report_error(0, "Error parsing TimestampTz: '%s' (unrecognized syntax from remote database)", (char*)data.buf);  // No rejected-rows for us!  Die on failure.
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
                            vt_report_error(0, "Error parsing Numeric: '%s' (unrecognized syntax from remote database)", (char*)data.buf);  // No rejected-rows for us!  Die on failure.
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

        // Connection string, passed in as an argument
        std::string connect = srvInterface.getParamReader().getStringRef("connect").str();
        std::string query = srvInterface.getParamReader().getStringRef("query").str();

        SQLRETURN r;

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

        r = SQLExecDirect(stmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLExecDirect()");

        r = SQLNumResultCols(stmt, &numcols);
        handleReturnCode(srvInterface, r, SQL_HANDLE_STMT, stmt, "SQLNumResultCols()");

        if ((ssize_t)numcols != (ssize_t)colInfo.getColumnCount()) {
            vt_report_error(0, "Expected %d columns; got %d from the remote database", (int)colInfo.getColumnCount(), (int)numcols);
        }

        // Set up column-data buffers
        // Bind to the columns in question
        col_data_bufs.reserve(numcols);  // So that push_back() doesn't move things around
        for (SQLSMALLINT i = 0; i < numcols; i++) {
            uint32 col_size = getFieldSizeForCol(i);
            SQLPOINTER buf = srvInterface.allocator->alloc(col_size);

            // This allocator may already guarantee zero'ed pages, but we really care; let's guarantee it
            memset(buf, '\0', col_size);

            col_data_bufs.push_back((Buf){0, buf});
            r = SQLBindCol(stmt, i+1, getCTypeOfCol(i),
                           buf, col_size-1, &col_data_bufs.back().len);

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

