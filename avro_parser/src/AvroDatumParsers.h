#ifndef avro_DatumParsers_hh__
#define avro_DatumParsers_hh__

using namespace Vertica;

/**
 * Parse an avro bool to a boolean
 */
bool
parseBool (const avro::GenericDatum& d, 
           size_t colNum,
           vbool& target,
           const VerticaType& type)
{
    if ( d.type() != AVRO_BOOL )
        return false;

    target = (d.value<bool>() ?
              vbool_true :
              vbool_false);
    return true;
}

/**
 * Parse an avro int or long to an integer
 */
bool
parseInt (const avro::GenericDatum& d, 
          size_t colNum,
          vint& target,
          const VerticaType& type)
{
   
    if ( d.type() == AVRO_INT )
        target = (long)d.value<int32_t>();
    else if ( d.type() == AVRO_LONG )
        target = (long)d.value<int64_t>();
    else
        return false;
    return true;
}

/**
 * Parse an avro float or double to a floating-point number
 */
bool
parseFloat (const avro::GenericDatum& d, 
            size_t colNum,
            vfloat& target,
            const VerticaType& type)
{
    if ( d.type() == AVRO_FLOAT )
        target = (double)d.value<float>();
    else if ( d.type() == AVRO_DOUBLE )
        target = d.value<double>();
    else
        return false;
    return true;
}

/**
 * Parse an avro string to a VARCHAR field
 */
bool
parseVarchar (const avro::GenericDatum& d, 
              size_t colNum,
              VString& target,
              const VerticaType& type)
{
    if ( d.type() != AVRO_STRING )
        return false;
    std::string val = d.value<std::string>();
    target.copy(val.c_str(), val.length());
    return true;
}

 /**
  * Parse an avro byte vector to a varbinary field
  */
bool
parseVarbinary(const avro::GenericDatum& d, 
               size_t colNum,
               VString& target,
               const VerticaType& type)
{
    // Don't attempt to parse binary.
    // Just copy it.
    if ( d.type() != AVRO_BYTES )
        return false;
    std::vector<uint8_t> val = d.value< std::vector<uint8_t> >();
    target.copy((char*)&val[0], val.size());    
    return true;
}

bool parseNumeric(const avro::GenericDatum& d, 
                  size_t colNum,
                  VNumeric& target,
                  const VerticaType& type)
{
    if ( d.type() != AVRO_STRING )
        return false;
    std::string val = d.value<std::string>();
    std::vector<char> str (val.size()+1);
    std::copy(val.begin(), val.end(), str.begin());
    str.push_back('\0');
    return VNumeric::charToNumeric(&str[0], type, target);
}

/**
 * Parse an avro string to a Vertica Date, according to the specified format.
 * `format` is a format-string as passed to the default Vertica
 * string->date parsing function.
 */
bool parseDate(const avro::GenericDatum& d, 
              size_t colNum,
              DateADT& target,
              const VerticaType& type)
{
    try {
        std::string val = d.value<std::string>();
        target = dateIn(val.c_str(), true);
        return true;
    } catch (...) {
        log("Invalid date format: %s", d.value<std::string>().c_str());
        return false;
    }
}

/**
 * Parse an avro string to a Vertica Time, according to the specified format.
 * `format` is a format-string as passed to the default Vertica
 * string->date parsing function.
 */
bool parseTime(const avro::GenericDatum& d, 
               size_t colNum,
               TimeADT& target,
               const VerticaType& type)
{
    try {
        std::string val = d.value<std::string>();
        target = timeIn(val.c_str(), type.getTypeMod(), true);
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * Parse an avro string to a Vertica Timestamp, according to the specified format.
 * `format` is a format-string as passed to the default Vertica
 * string->date parsing function.
 */
bool parseTimestamp(const avro::GenericDatum& d, 
                    size_t colNum,
                    Timestamp& target,
                    const VerticaType& type)
{
    try {
        std::string val = d.value<std::string>();
        target = timestampIn(val.c_str(), type.getTypeMod(), true);
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * Parse an avro string to a Vertica TimeTz, according to the specified format.
 * `format` is a format-string as passed to the default Vertica
 * string->date parsing function.
 */
bool parseTimeTz(const avro::GenericDatum& d, 
                 size_t colNum,
                 TimeTzADT& target,
                 const VerticaType& type)
{
    try {
        std::string val = d.value<std::string>();
        target = timetzIn(val.c_str(), type.getTypeMod(), true);
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * Parse an avro string to a Vertica TimestampTz, according to the specified format.
 * `format` is a format-string as passed to the default Vertica
 * string->date parsing function.
 */
bool parseTimestampTz(const avro::GenericDatum& d, 
                      size_t colNum, 
                      TimestampTz &target, 
                      const VerticaType &type) 
{
    try {
        std::string val = d.value<std::string>();
        target = timestamptzIn(val.c_str(), type.getTypeMod(), true);
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * Parse an interval expression (from an avro string) to a Vertica Interval
 */
bool parseInterval(const avro::GenericDatum& d, 
                   size_t colNum, 
                   Interval &target, 
                   const VerticaType &type) 
{
    try {
        std::string val = d.value<std::string>();        
        target = intervalIn(val.c_str(), type.getTypeMod(), true);
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * Parses avro datum according to the column's type
 */
bool
parseAvroFieldToType(const GenericDatum& d, 
                     size_t colNum, 
                     const VerticaType &type,
                     ::Vertica::StreamWriter *writer)
{
    bool retVal;
    
    switch(type.getTypeOid())
    {
    case BoolOID :
    {
        vbool val;
        retVal = parseBool(d, colNum, val, type);
        writer->setBool(colNum, val);
        break;
        
    }
    case Int8OID: {
        vint val;
        retVal = parseInt(d, colNum, val, type);
        writer->setInt(colNum, val);
        break;
    }
    case Float8OID: {
        vfloat val;
        retVal = parseFloat(d, colNum, val, type);
        writer->setFloat(colNum, val);
        break;
    }
    case CharOID: {
        VString val = writer->getStringRef(colNum);
        retVal = parseVarchar(d, colNum, val, type);
        break;
    }
    case VarcharOID: {
        VString val = writer->getStringRef(colNum);
        retVal = parseVarchar(d, colNum, val, type);
        break;
    }
    case NumericOID: {
        VNumeric val = writer->getNumericRef(colNum);
        retVal = parseNumeric(d, colNum, val, type);
        break;
    }
    case DateOID: {
        DateADT val;
        retVal = parseDate(d, colNum, val, type);
        writer->setDate(colNum, val);
        break;
    }
    case TimeOID: {
        TimeADT val;
        retVal = parseTime(d, colNum, val, type);
        writer->setTime(colNum, val);
        break;
    }
    case TimestampOID: {
        Timestamp val;
        retVal = parseTimestamp(d, colNum, val, type);
        writer->setTimestamp(colNum, val);
        break;
    }
    case TimestampTzOID: {
        TimestampTz val;
        retVal = parseTimestampTz(d, colNum, val, type);
        writer->setTimestampTz(colNum, val);
        break;
    }
    case BinaryOID:
    case VarbinaryOID: {
        VString val = writer->getStringRef(colNum);
        retVal = parseVarbinary(d, colNum, val, type);
        break;
    }
    case IntervalOID: {
        Interval val;
        retVal = parseInterval(d, colNum, val, type);
        writer->setInterval(colNum, val);
        break;
    }
    case TimeTzOID: {
        TimeTzADT val;
        retVal = parseTimeTz(d, colNum, val, type);
        writer->setTimeTz(colNum, val);
        break;
    }
    case IntervalYMOID:
    default:
        vt_report_error(0, "Error, unrecognized type: '%s'", type.getTypeStr());
        retVal = false;
        
    }
    return retVal;
}
#endif
