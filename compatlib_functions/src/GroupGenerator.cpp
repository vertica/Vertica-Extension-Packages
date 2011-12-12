/* Copyright (c) 2011 Vertica, an HP company -*- C++ -*- */
/* 
 * Description: UDT to create groups for use by GroupBy to do ROLLUP/CUBE/GROUPING SETS.
 *
 * Create Date: Dec 11, 2011
 * Author: Matt Fuller
 */

#include "Vertica.h"

using namespace Vertica;
using namespace std;


/**
 * Give a single row, output N rows. Using the GroupSets to determine
 * If the output column should output the input value. Or if the output
 * column should be NULL.
 *
 * Example: We want to compute GBY A, B, C, GBY A, B, GBY A, GBY NOTHING
 *
 * Use group_generator(A, B, 95, 4)
 *
 * There will be too GroupingSets { {1, 1, 1}, {0, 1, 1}, {0, 0, 1}, {0, 0, 0} }
 * Each GroupingSet size will be in the number of input columns. 3 in this example - {A, B, C}

 * 1/TRUE indicates the set does not have this column in its GroupingSet. Therefore a NULL should be emitted.
 * 0/FALSE indicates the set include the column in its GroupingSet and we copy the input value to the output.
 *
 * 95 is the argument passed in because 95 is based 10 of binary 000001011111.
 * We should make functions ROLLUP and CUBE to make this calculation easier.
 *
 * See example SQL file on how to use GroupGenerator to do ROLLUP, CUBE,
 * GROUPING_SETS, and Multiple Distrinct Aggregates in the NEW EE.
 *
 */ 
class GroupGenerator : public TransformFunction
{
    
public:

    GroupGenerator() {}

    typedef vector<bool> GroupingSet;

    static inline void writeColumn(PartitionReader &input_reader, 
                                   PartitionWriter &output_writer,
                                   const SizedColumnTypes &types,
                                   size_t col,
                                   bool setNull) {

        const VerticaType &vt = types.getColumnType(col);
        
        // switch on type and write value
        switch (vt.getTypeOid()) 
        {

        case Int8OID:
        {
            if(!setNull) 
                output_writer.setInt(col, input_reader.getIntRef(col));
            else
                output_writer.setInt(col, vint_null);
            break;
        }

        case Float8OID: 
        {
            if(!setNull) 
                output_writer.setFloat(col, input_reader.getFloatRef(col));
            else
                output_writer.setFloat(col, vfloat_null);
            break;
        }       

        case VarcharOID:
        {
            if(!setNull)
            {
                const VString &in = input_reader.getStringRef(col);
                output_writer.getStringRef(col).copy(&in);
            }
            else
                output_writer.getStringRef(col).setNull();
            
            break;
        }

        default:
        { 
            vt_report_error(0, "Argument type not support for GroupGenerator");
            break;
        }

        }
    }

    static inline void writeRow(PartitionReader &input_reader, 
                                PartitionWriter &output_writer,
                                const GroupingSet &groupingSet) {

        SizedColumnTypes types = input_reader.getTypeMetaData();
        size_t nGroupingCols = groupingSet.size();
        uint grouping_id = 0;
        
        // write each column value
        // write null if the column should not be including in the grouping set
        for(size_t i = 0; i < nGroupingCols; ++i)
        {
            bool setNull = groupingSet[nGroupingCols-1-i];
            grouping_id <<= 1;
            if (setNull) grouping_id |= 1;
            
            writeColumn(input_reader, output_writer, types, i, setNull);
        }
        
        // write the grouping id
        output_writer.setInt(nGroupingCols, grouping_id);

    }
         
    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &input_reader, 
                                  PartitionWriter &output_writer) {
        
        uint numCols = input_reader.getNumCols();
        
        // have to assume the user passed in the correctly number of masks associated with masks
        uint64 sets = input_reader.getIntRef(numCols-2);
        uint nSets = input_reader.getIntRef(numCols-1);
        
        // "parse" the binary grouping set representation into GroupingSet vectors
        vector<GroupingSet> groupingSets;
        uint64 mask = 1;
        for(size_t i = 0; i < nSets; ++i)
        {
            GroupingSet groupingSet;
            
            for(size_t j = 0; j < numCols-2; ++j)
            {
                if (sets & mask)
                    groupingSet.push_back(true);
                else
                    groupingSet.push_back(false);

                mask <<= 1;
            }
          
            groupingSets.push_back(groupingSet);
        }

        // for each row, output a row for each grouping set
        do
        {
            for(size_t i = 0; i < groupingSets.size(); ++i)
            {
                writeRow(input_reader, output_writer, groupingSets[i]);
                output_writer.next();
            }
        } while (input_reader.next()); 

    }
        
};

class GroupGeneratorFactory : public TransformFunctionFactory
{

    void addType(ColumnTypes &types, const Oid &type) {

        switch (type)
        {
        case Int8OID:
        {
            types.addInt();
            break;
        }
        case Float8OID: 
        {
            types.addFloat();
            break;
        }       
        case VarcharOID:
        {
            types.addVarchar();
            break;
        }
        default:
        {
            vt_report_error(0, "Argument type not support for GroupGenerator");
            break;
        }
        }
    }

    virtual int getNumCols() = 0;
    virtual Oid getType(uint i) = 0;

    virtual void getPrototype(ServerInterface &srvInterface, 
                              ColumnTypes &argTypes, 
                              ColumnTypes &returnType) {

        for (int i = 0; i < getNumCols(); ++i)
        {
            addType(argTypes, getType(i+1));
            addType(returnType, getType(i+1));
        }
        
        argTypes.addInt(); // mask for grouping sets
        argTypes.addInt(); // number of grouping sets

        returnType.addInt(); // for grouping
    }

    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &input_types, 
                               SizedColumnTypes &output_types) 
        {
            for (int i = 0; i < getNumCols(); ++i)
                output_types.addArg(input_types.getColumnType(i), input_types.getColumnName(i));
                
            output_types.addInt("grouping_id");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { 
        return vt_createFuncObj(srvInterface.allocator, GroupGenerator); 
    }

};

#define I Int8OID
#define F Float8OID
#define V VarcharOID
#define O 0

#define GroupGeneratorFactoryM4(type1, type2, type3, type4)     \
    class GroupGeneratorFactory##type1##type2##type3##type4 : public GroupGeneratorFactory { \
        int getNumCols() { return 4; }                                  \
    Oid getType(uint i) {                                               \
        if (i == 1) { return type1; }                                   \
        if (i == 2) { return type2; }                                   \
        if (i == 3) { return type3; }                                   \
        if (i == 4) { return type4; }                                   \
        return 0;                                                       \
    }                                                                   \
    };                                                                  \
    RegisterFactory(GroupGeneratorFactory##type1##type2##type3##type4); \

#define GroupGeneratorFactoryM3(type1, type2, type3)                    \
    class GroupGeneratorFactory##type1##type2##type3 : public GroupGeneratorFactory { \
        int getNumCols() { return 3; }                                  \
    Oid getType(uint i) {                                               \
        if (i == 1) { return type1; }                                   \
        if (i == 2) { return type2; }                                   \
        if (i == 3) { return type3; }                                   \
        return 0;                                                       \
    }                                                                   \
    };                                                                  \
    RegisterFactory(GroupGeneratorFactory##type1##type2##type3); \

#define GroupGeneratorFactoryM2(type1, type2)                           \
    class GroupGeneratorFactory##type1##type2 : public GroupGeneratorFactory { \
        int getNumCols() { return 2; }                                  \
    Oid getType(uint i) {                                               \
        if (i == 1) { return type1; }                                   \
        if (i == 2) { return type2; }                                   \
        return 0;                                                       \
    }                                                                   \
    };                                                                  \
    RegisterFactory(GroupGeneratorFactory##type1##type2); \

#define GroupGeneratorFactoryM1(type1)                                  \
    class GroupGeneratorFactory##type1 : public GroupGeneratorFactory { \
        int getNumCols() { return 1; }                                  \
    Oid getType(uint i) {                                               \
        if (i == 1) { return type1; }                                   \
        return 0;                                                       \
    }                                                                   \
    };                                                                  \
    RegisterFactory(GroupGeneratorFactory##type1); \

// make multiple factories with different types
GroupGeneratorFactoryM4(V,V,V,V);
GroupGeneratorFactoryM4(V,V,V,F);
GroupGeneratorFactoryM4(V,V,F,V);
GroupGeneratorFactoryM4(V,V,F,F);
GroupGeneratorFactoryM4(V,F,V,V);
GroupGeneratorFactoryM4(V,F,V,F);
GroupGeneratorFactoryM4(V,F,F,V);
GroupGeneratorFactoryM4(V,F,F,F);
GroupGeneratorFactoryM4(F,V,V,V);
GroupGeneratorFactoryM4(F,V,V,F);
GroupGeneratorFactoryM4(F,V,F,V);
GroupGeneratorFactoryM4(F,V,F,F);
GroupGeneratorFactoryM4(F,F,V,V);
GroupGeneratorFactoryM4(F,F,V,F);
GroupGeneratorFactoryM4(F,F,F,V);
GroupGeneratorFactoryM4(F,F,F,F);

GroupGeneratorFactoryM3(V,V,V);
GroupGeneratorFactoryM3(V,V,F);
GroupGeneratorFactoryM3(V,F,V);
GroupGeneratorFactoryM3(V,F,F);
GroupGeneratorFactoryM3(F,V,V);
GroupGeneratorFactoryM3(F,V,F);
GroupGeneratorFactoryM3(F,F,V);
GroupGeneratorFactoryM3(F,F,F);

GroupGeneratorFactoryM2(V,V);
GroupGeneratorFactoryM2(V,F);
GroupGeneratorFactoryM2(F,V);
GroupGeneratorFactoryM2(F,F);

GroupGeneratorFactoryM1(V);
GroupGeneratorFactoryM1(F);
