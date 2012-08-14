#ifndef avro_DatumParsers_hh__
#define avro_DatumParsers_hh__

/**
 * Parse an avro bool to a boolean
 */
bool
parseBool (const avro::GenericDatum& d, 
           size_t colNum,
           Vertica::vbool& target,
           const Vertica::VerticaType& type)
{
    if ( d.type() != AVRO_BOOL )
        return false;

    target = (d.value<bool>() ?
              Vertica::vbool_true :
              Vertica::vbool_false);
    return true;
}

/**
 * Parse an avro int or long to an integer
 */
bool
parseInt (const avro::GenericDatum& d, 
          size_t colNum,
          Vertica::vint& target,
          const Vertica::VerticaType& type)
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
            Vertica::vfloat& target,
            const Vertica::VerticaType& type)
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
              Vertica::VString& target,
              const Vertica::VerticaType& type)
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
               Vertica::VString& target,
               const Vertica::VerticaType& type)
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
                  Vertica::VNumeric& target,
                  const Vertica::VerticaType& type)
{
    if ( d.type() != AVRO_STRING )
        return false;
    std::string val = d.value<std::string>();
    std::vector<char> str (val.size()+1);
    std::copy(val.begin(), val.end(), str.begin());
    str.push_back('\0');
    return Vertica::VNumeric::charToNumeric(&str[0], type, target);
}

/**
 * Parse an avro string to a Vertica Date, according to the specified format.
 * `format` is a format-string as passed to the default Vertica
 * string->date parsing function.
 */
bool parseDate(const avro::GenericDatum& d, 
              size_t colNum,
              Vertica::DateADT& target,
              const Vertica::VerticaType& type)
{
    try {
        std::string val = d.value<std::string>();
        target = Vertica::dateIn(val.c_str(), true);
        return true;
    } catch (...) {
        Vertica::log("Invalid date format: %s", d.value<std::string>().c_str());
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
               Vertica::TimeADT& target,
               const Vertica::VerticaType& type)
{
    try {
        std::string val = d.value<std::string>();
        target = Vertica::timeIn(val.c_str(), type.getTypeMod(), true);
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
                    Vertica::Timestamp& target,
                    const Vertica::VerticaType& type)
{
    try {
        std::string val = d.value<std::string>();
        target = Vertica::timestampIn(val.c_str(), type.getTypeMod(), true);
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
                 Vertica::TimeTzADT& target,
                 const Vertica::VerticaType& type)
{
    try {
        std::string val = d.value<std::string>();
        target = Vertica::timetzIn(val.c_str(), type.getTypeMod(), true);
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
                      Vertica::TimestampTz &target, 
                      const Vertica::VerticaType &type) 
{
    try {
        std::string val = d.value<std::string>();
        target = Vertica::timestamptzIn(val.c_str(), type.getTypeMod(), true);
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
                   Vertica::Interval &target, 
                   const Vertica::VerticaType &type) 
{
    try {
        std::string val = d.value<std::string>();        
        target = Vertica::intervalIn(val.c_str(), type.getTypeMod(), true);
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
                     const Vertica::VerticaType &type,
                     Vertica::StreamWriter *writer)
{
    bool retVal;
    
    switch(type.getTypeOid())
    {
    case Vertica::BoolOID :
    {
        Vertica::vbool val;
        retVal = parseBool(d, colNum, val, type);
        writer->setBool(colNum, val);
        break;
        
    }
    case Vertica::Int8OID: {
        Vertica::vint val;
        retVal = parseInt(d, colNum, val, type);
        writer->setInt(colNum, val);
        break;
    }
    case Vertica::Float8OID: {
        Vertica::vfloat val;
        retVal = parseFloat(d, colNum, val, type);
        writer->setFloat(colNum, val);
        break;
    }
    case Vertica::CharOID: {
        Vertica::VString val = writer->getStringRef(colNum);
        retVal = parseVarchar(d, colNum, val, type);
        break;
    }
    case Vertica::VarcharOID: {
        Vertica::VString val = writer->getStringRef(colNum);
        retVal = parseVarchar(d, colNum, val, type);
        break;
    }
    case Vertica::NumericOID: {
        Vertica::VNumeric val = writer->getNumericRef(colNum);
        retVal = parseNumeric(d, colNum, val, type);
        break;
    }
    case Vertica::DateOID: {
        Vertica::DateADT val;
        retVal = parseDate(d, colNum, val, type);
        writer->setDate(colNum, val);
        break;
    }
    case Vertica::TimeOID: {
        Vertica::TimeADT val;
        retVal = parseTime(d, colNum, val, type);
        writer->setTime(colNum, val);
        break;
    }
    case Vertica::TimestampOID: {
        Vertica::Timestamp val;
        retVal = parseTimestamp(d, colNum, val, type);
        writer->setTimestamp(colNum, val);
        break;
    }
    case Vertica::TimestampTzOID: {
        Vertica::TimestampTz val;
        retVal = parseTimestampTz(d, colNum, val, type);
        writer->setTimestampTz(colNum, val);
        break;
    }
    case Vertica::BinaryOID:
    case Vertica::VarbinaryOID: {
        Vertica::VString val = writer->getStringRef(colNum);
        retVal = parseVarbinary(d, colNum, val, type);
        break;
    }
    case Vertica::IntervalOID: {
        Vertica::Interval val;
        retVal = parseInterval(d, colNum, val, type);
        writer->setInterval(colNum, val);
        break;
    }
    case Vertica::TimeTzOID: {
        Vertica::TimeTzADT val;
        retVal = parseTimeTz(d, colNum, val, type);
        writer->setTimeTz(colNum, val);
        break;
    }
    case Vertica::IntervalYMOID:
    default:
        vt_report_error(0, "Error, unrecognized type: '%s'", type.getTypeStr());
        retVal = false;
        
    }
    return retVal;
}
#endif
