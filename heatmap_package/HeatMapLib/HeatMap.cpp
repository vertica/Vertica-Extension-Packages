/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */
/* 
 * Description: User Defined Transform Function: HeatMap
 *
 * UDx plugin by:  Matthew Fay, Mark Fay
 *
 * Create Date: July 9, 2012
 */
#include "Vertica.h"
#include <sstream>
#include <vector>
#include <stdexcept>
#include "GaussianBlur.h"

using namespace Vertica;
using namespace std;

class HeatMap : public TransformFunction
{
    // Used For Buffering
    typedef struct {
        float x;
        float y;
        float w;
    } tuple;
    vector<tuple> tuple_buffer;
    int tuple_buffer_max;
    int tuple_buffer_count;

    // Main Histogram for HeatMap generation
    vector< vector<float> > count;

    // Mins and Maxes found from data
    float data_min_x;
    float data_min_y;
    float data_max_x;
    float data_max_y;

    // Resolution (size of bins)
    float res_x;
    float res_y;
    
    // Used during initialization
    bool have_first;
    bool hist_init;

    // Number of bins in histogram
    int histogram_width;
    int histogram_height;

    // Histogram bounds
    float histogram_min_x;
    float histogram_min_y;
    float histogram_max_x;
    float histogram_max_y;
    float histogram_weight;

    // Whether the mins/maxes are fixed by user or autocalculated
    bool fix_min_x;
    bool fix_min_y;
    bool fix_max_x;
    bool fix_max_y;
    
    // Miscuellaneous Flags
    int do_normalize;
    bool do_gaussian;
    bool do_bounding;
    bool use_weights;

    //Scales the histogram, increases the number of bins while trying to maintain overrall shape of data//
    void rescale_histogram(float min_x, float max_x, float min_y, float max_y) {
        if(fix_min_x)
          min_x = histogram_min_x;
        if(fix_max_x)
          max_x = histogram_max_x;
        if(fix_min_y)
          min_y = histogram_min_y;
        if(fix_max_y)
          max_y = histogram_max_y;
        if(min_x < histogram_min_x || min_y < histogram_min_y ||
           max_x > histogram_max_x || max_y > histogram_max_y) {

            float p_min_x = histogram_min_x;
            float p_max_x = histogram_max_x;
            float p_min_y = histogram_min_y;
            float p_max_y = histogram_max_y;
            
            float p_area = (p_max_x-p_min_x)*(p_max_y-p_min_y);
            float h_area = (max_x-min_x)*(max_y-min_y);

            float d_weight = p_area/h_area;

            float p_res_x = res_x;
            float p_res_y = res_y;
            float h_res_x = (max_x - min_x) / ((float)histogram_width);
            float h_res_y = (max_y - min_y) / ((float)histogram_height);

            vector< vector<float> > p_count;
            p_count.resize(histogram_width);
            for(int i=0; i<histogram_width; i++) {
                p_count[i].resize(histogram_height);
                for(int j=0; j<histogram_height; j++) {
                    p_count[i][j] = count[i][j]*d_weight;
                    count[i][j] = 0;
                }
            }

            for(int px=0; px<histogram_width; px++) {
                for(int py=0; py<histogram_height; py++) {
                    float plx = p_min_x + px*res_x;
                    float pty = p_min_y + py*res_y;
                    float prx = p_min_x + (px+1)*res_x;
                    float pby = p_min_y + (py+1)*res_y;
                    
                    int bin_lx = (plx-min_x)/h_res_x;
                    int bin_ty = (pty-min_y)/h_res_y;

                    int bin_rx = (prx-min_x)/h_res_x;
                    int bin_by = (pby-min_y)/h_res_y;

                    if(bin_by >= histogram_height) {
                        bin_by = bin_ty;
                        bin_by = histogram_height-1;
                    }
                    if(bin_rx >= histogram_width) {
                        bin_rx = bin_lx;
                        bin_rx = histogram_width-1;
                    }

                    if(bin_lx == bin_rx) {
                        //Same Horizontal, at worst only one split
                        if(bin_ty == bin_by) {
                            // No Split, all weight goes to one bin
                            count[bin_lx][bin_ty] += p_count[px][py];
                        } else {
                            // bin_by == bin_ty + 1
                            float bin_by_y = (bin_by * h_res_y) + min_y;
                            float factor = (bin_by_y - pty)/p_res_y;
                            count[bin_lx][bin_ty] += p_count[px][py]*factor;
                            count[bin_lx][bin_by] += p_count[px][py]*(1-factor);
                        }
                    } else {
                        //Same Horizontal, at worst only one split
                        if(bin_ty == bin_by) {
                            // bin_rx == bin_lx + 1
                            float bin_rx_x = (bin_rx * h_res_x) + min_x;
                            float factor = (bin_rx_x - plx)/p_res_x;
                            count[bin_lx][bin_ty] += p_count[px][py]*factor;
                            count[bin_rx][bin_ty] += p_count[px][py]*(1-factor);
                        } else {
                            //The Bad Case
                            float bin_rx_x = (bin_rx * h_res_x) + min_x;
                            float bin_by_y = (bin_by * h_res_y) + min_y;
                            float factor_x = (bin_rx_x - plx)/p_res_x;
                            float factor_y = (bin_by_y - pty)/p_res_y;

                            count[bin_lx][bin_ty] += p_count[px][py]*factor_x*factor_y;

                            count[bin_rx][bin_ty] += p_count[px][py]*(1-factor_x)*factor_y;
                            count[bin_lx][bin_by] += p_count[px][py]*factor_x*(1-factor_y);
                            count[bin_rx][bin_by] += p_count[px][py]*(1-factor_x)*(1-factor_y);
                        }
                    }
                }
            }

            histogram_min_x = min_x;
            histogram_max_x = max_x;
            histogram_min_y = min_y;
            histogram_max_y = max_y;
            histogram_weight = histogram_weight*d_weight;
            res_x = h_res_x;
            res_y = h_res_y;
            
        }
    }
    
    // Normalizes the histogram, 1 - divide by max, 2 - divide by max and equally proportion, 3 - divide by range
    void normalize_histogram() {
        float max_weight = 1;
        float min_weight = -1;
        float sum = 0;
        for(int i=0;i<histogram_width;i++) {
            for(int j=0;j<histogram_height;j++) {
                if(count[i][j] > max_weight) {
                    max_weight = count[i][j];
                }
                if(count[i][j] < min_weight || min_weight < 0) {
                    min_weight = count[i][j];
                }
                sum += count[i][j];
            }
        }

        if (do_normalize == 2)
          sum /= max_weight;

        for(int i=0;i<histogram_width;i++) {
            for(int j=0;j<histogram_height;j++) {
                if(do_normalize <= 2) {
                    count[i][j] = count[i][j]/max_weight;
                    if(do_normalize == 2 && sum > 0) count[i][j] /= sum;
                } else {
                    count[i][j] -= min_weight;
                    count[i][j] /= (max_weight-min_weight);
                }                  
            }
        }
    }

    // Does the row/bin-wise out put of the heatmap
    void output_counts(vector< vector<float> > counts, PartitionWriter &outputWriter) {
        for(int x=0;x<histogram_width;x++) {
            for(int y=0;y<histogram_height;y++) {
                VNumeric &bin_x = outputWriter.getNumericRef(0);
                VNumeric &bin_y = outputWriter.getNumericRef(1);
                bin_x.copy(((float)x)*res_x+histogram_min_x);
                bin_y.copy(((float)y)*res_y+histogram_min_y);
                if (do_bounding) {
                    VNumeric &bin_x_top = outputWriter.getNumericRef(2);
                    VNumeric &bin_y_top = outputWriter.getNumericRef(3);
                    bin_x_top.copy(((float)(x+1))*res_x+histogram_min_x);
                    bin_y_top.copy(((float)(y+1))*res_y+histogram_min_y);                        
                    outputWriter.setFloat(4,counts[x][y]);
                } else {
                    outputWriter.setFloat(2,counts[x][y]);
                }
                outputWriter.next();
            }
        }
    }

    // Flushes the input tuple buffer into the heat map
    void flush_buffer() {
        if(!hist_init) {
            histogram_weight = 1;
            
            if(!fix_min_x)
              histogram_min_x = data_min_x;
            if(!fix_max_x)
              histogram_max_x = data_max_x;
            if(!fix_min_y)
              histogram_min_y = data_min_y;
            if(!fix_max_y)
              histogram_max_y = data_max_y;
            
            res_x = (histogram_max_x - histogram_min_x) / ((float)histogram_width);
            res_y = (histogram_max_y - histogram_min_y) / ((float)histogram_height);
            hist_init=true;
        } else {
            rescale_histogram(data_min_x, data_max_x, data_min_y, data_max_y);
        }
        while(!tuple_buffer.empty()) {
            tuple bt = tuple_buffer.back();
            
            if (!(fix_min_x && bt.x < histogram_min_x) &&
                !(fix_min_y && bt.y < histogram_min_y) &&
                !(fix_max_x && bt.x > histogram_max_x) &&
                !(fix_max_y && bt.y > histogram_max_y)
                ) {
                
                
                int bin_x = (bt.x-histogram_min_x)/res_x;
                int bin_y = (bt.y-histogram_min_y)/res_y;
                
                if ( bin_x < 0 )
                  bin_x = 0;
                if ( bin_y < 0 )
                  bin_y = 0;
                if ( bin_x >= histogram_width )
                  bin_x = histogram_width-1;
                if ( bin_y >= histogram_height )
                  bin_y = histogram_height-1;
                if ( bin_x >= 0 && bin_x < histogram_width && bin_y >= 0 && bin_y <= histogram_height ) {
                    count[bin_x][bin_y] += histogram_weight*bt.w;                        
                }
            }
            tuple_buffer.pop_back();
        }
    }

    enum vtype { FLOAT, INT, NUMERIC };
    vtype x_type;
    vtype y_type;
    vtype w_type;
    

    // Sets up the parameters
    virtual void setup(ServerInterface & 	srvInterface,
                       const SizedColumnTypes & 	argTypes )
    {
        try {
            have_first = false;
            hist_init = false;
            tuple_buffer_count = 0;
            tuple_buffer_max = 100000;
            if (srvInterface.getParamReader().containsParameter("xbins"))
              histogram_width = (int)srvInterface.getParamReader().getIntRef("xbins");
            else
              histogram_width = 64;
            if (srvInterface.getParamReader().containsParameter("ybins"))
              histogram_height = (int)srvInterface.getParamReader().getIntRef("ybins");
            else
              histogram_height = 64;
            count = vector< vector<float> >(histogram_width, vector<float>(histogram_height, 0));

            fix_min_x = false;
            if (srvInterface.getParamReader().containsParameter("xmin")) {
                fix_min_x = true;
                histogram_min_x = (float)srvInterface.getParamReader().getFloatRef("xmin");
            }
            fix_min_y = false;
            if (srvInterface.getParamReader().containsParameter("ymin")) {
                fix_min_y = true;
                histogram_min_y = (float)srvInterface.getParamReader().getFloatRef("ymin");
            }
            fix_max_x = false;
            if (srvInterface.getParamReader().containsParameter("xmax")) {
                fix_max_x = true;
                histogram_max_x = (float)srvInterface.getParamReader().getFloatRef("xmax");
            }
            fix_max_y = false;
            if (srvInterface.getParamReader().containsParameter("ymax")) {
                fix_max_y = true;
                histogram_max_y = (float)srvInterface.getParamReader().getFloatRef("ymax");
            }

            do_normalize = 0    ;
            if (srvInterface.getParamReader().containsParameter("normalize"))
              do_normalize = srvInterface.getParamReader().getIntRef("normalize");
            if (srvInterface.getParamReader().containsParameter("bounding_box")) {
                do_bounding = srvInterface.getParamReader().getBoolRef("bounding_box");
            }
            do_gaussian = false;
            if (srvInterface.getParamReader().containsParameter("gaussian")) {
                do_gaussian=srvInterface.getParamReader().getBoolRef("gaussian");
            }
            
            use_weights = false;
            if (srvInterface.getParamReader().containsParameter("use_weights")) {
                use_weights=srvInterface.getParamReader().getBoolRef("use_weights");
            }

            if( argTypes.getColumnType(0).isFloat() ) {
                x_type = FLOAT;
            } else if( argTypes.getColumnType(0).isInt() ) {
                x_type = INT;
            } else if( argTypes.getColumnType(0).isNumeric() ) {
                x_type = NUMERIC;
            } else {
                throw runtime_error("X must be a float, integer, or numeric.");
            }

            if( argTypes.getColumnType(1).isFloat() ) {
                y_type = FLOAT;
            } else if( argTypes.getColumnType(1).isInt() ) {
                y_type = INT;
            } else if( argTypes.getColumnType(1).isNumeric() ) {
                y_type = NUMERIC;
            } else {
                throw runtime_error("Y must be a float, integer, or numeric.");
            }

            if(use_weights) {
                if( argTypes.getColumnType(2).isFloat() ) {
                    w_type = FLOAT;
                } else if( argTypes.getColumnType(2).isInt() ) {
                    w_type = INT;
                } else if( argTypes.getColumnType(2).isNumeric() ) {
                    w_type = NUMERIC;
                } else {
                    throw runtime_error("Weight must be a float, integer, or numeric.");
                }
            }


/*
                    parameterTypes.addInt("guassian");
*/
        } catch(exception& e) {
            // Standard exception. Quit.
            vt_report_error(0, "Exception while processing partition: [%s]", e.what());
        } catch(char * msg) {
            vt_report_error(0, "Error: [%s]", msg);
        }
    }

    // Does the buffering, heat map generation and then output
    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &inputReader, 
                                  PartitionWriter &outputWriter)
    {
        try {
            if ((inputReader.getNumCols() != 3 && use_weights) || (inputReader.getNumCols() != 2 && !use_weights))
              vt_report_error(0, "Function only accepts 2 or 3 arguments, but %zu provided", inputReader.getNumCols());

            do {
                float x_val = 0;
                float y_val = 0;
                float w_val = 1;
                switch(x_type) {
                  case NUMERIC:
                    x_val = inputReader.getNumericRef(0).toFloat();
                    break;
                  case INT:
                    x_val = inputReader.getIntRef(0);
                    break;
                  case FLOAT:
                    x_val = inputReader.getFloatRef(0);
                    break;
                  default:
                    break;
                }
                switch(y_type) {
                  case NUMERIC:
                    y_val = inputReader.getNumericRef(1).toFloat();
                    break;
                  case INT:
                    y_val = inputReader.getIntRef(1);
                    break;
                  case FLOAT:
                    y_val = inputReader.getFloatRef(1);
                    break;
                  default:
                    break;
                }
                if(use_weights) {
                    switch(w_type) {
                      case NUMERIC:
                        w_val = inputReader.getNumericRef(2).toFloat();
                        break;
                      case INT:
                        w_val = inputReader.getIntRef(2);
                        break;
                      case FLOAT:
                        w_val = inputReader.getFloatRef(2);
                        break;
                      default:
                        w_val = 1;
                        break;
                    }
                }

                tuple t;

                t.x = x_val;
                t.y = y_val;
                t.w = w_val;

                tuple_buffer.push_back(t);
                tuple_buffer_count++;

                if (!have_first) {
                    data_min_x = t.x;
                    data_max_x = t.x;
                    data_min_y = t.y;
                    data_max_y = t.y;

                    have_first = true;
                }
                
                if(t.x > data_max_x)
                  data_max_x = t.x;
                if(t.x < data_min_x)
                  data_min_x = t.x;
                if(t.y > data_max_y)
                  data_max_y = t.y;
                if(t.y < data_min_y)
                  data_min_y = t.y;

                if(tuple_buffer_count >= tuple_buffer_max) {
                    flush_buffer();
                    tuple_buffer_count = 0;
                }
            } while (inputReader.next());
            
            if(tuple_buffer_count >= 1) {
                flush_buffer();
                tuple_buffer_count = 0;
            }
            
            if(do_normalize>0) {
                normalize_histogram();
            }

            if(do_gaussian) {
                output_counts(GaussianBlur(count,histogram_width,histogram_height, 1.0), outputWriter);
            } else {
                output_counts(count, outputWriter);
            }

        } catch(exception& e) {
            // Standard exception. Quit.
            vt_report_error(0, "Exception while processing partition: [%s]", e.what());
        }
    }
};

class HeatMapFactory : public TransformFunctionFactory
{
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addAny();

        returnType.addNumeric();
        returnType.addNumeric();
        if (srvInterface.getParamReader().containsParameter("bounding_box")) {
              bool do_bounding = srvInterface.getParamReader().getBoolRef("bounding_box");
              if(do_bounding) {
                  returnType.addNumeric();
                  returnType.addNumeric();
              }
        }
        returnType.addFloat();
    }

    // Tell Vertica what our return string length will be, given the input
    // string length
    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &inputTypes, 
                               SizedColumnTypes &outputTypes)
    {
        if (inputTypes.getColumnCount() != 2 && inputTypes.getColumnCount() != 3)
            vt_report_error(0, "Function only accepts 2 or 3 arguments, but %zu provided", inputTypes.getColumnCount());
        outputTypes.addNumeric(10,4);
        outputTypes.addNumeric(10,4);
        if (srvInterface.getParamReader().containsParameter("bounding_box")) {
            bool do_bounding = srvInterface.getParamReader().getBoolRef("bounding_box");
            if(do_bounding) {
                outputTypes.addNumeric(10,4);
                outputTypes.addNumeric(10,4);
            }
        }
        outputTypes.addFloat();
    }

    // Defines the parameters for this UDSF. Works similarly to defining
    // arguments and return types.
    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes)
    {
        parameterTypes.addInt("xbins");
        parameterTypes.addInt("ybins");

        parameterTypes.addFloat("xmin");
        parameterTypes.addFloat("ymin");
        parameterTypes.addFloat("xmax");
        parameterTypes.addFloat("ymax");

        parameterTypes.addBool("gaussian");
        parameterTypes.addInt("normalize");

        parameterTypes.addBool("bounding_box");

        parameterTypes.addBool("use_weights");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, HeatMap); }

};

RegisterFactory(HeatMapFactory);
