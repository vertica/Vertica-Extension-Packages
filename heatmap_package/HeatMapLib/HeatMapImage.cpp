/* Copyright (c) 2005 - 2012 Vertica, an HP company -*- C++ -*- */
/* 
 * Description: User Defined Transform Function: HeatMap
 *
 * UDx plugin by: Mark Fay, Matt Fay
 * 
 * Create Date: July 9, 2012
 */
#include "Vertica.h"
#include "lodepng.h"
#include "GaussianBlur.h"
#include <vector>
#include <sstream>
#include <iostream>

using namespace Vertica;
using namespace std;
using namespace lodepng;

class HeatMapImage : public TransformFunction
{
    // used for reading in row data
    typedef struct {
        float x;
        float y;
        float c;
    } rowInfo;
    vector<rowInfo> rows_info;
    int rows_count;
    int rows_max_count;
    
    // data mins and maxes
    float data_min_x;
    float data_max_x;
    float data_min_y;
    float data_max_y;
    float data_min_c;
    float data_max_c;
    
    // miscellaneous row processing
    bool is_first_row;
    int bins_x,bins_y;
    float check_x,check_y;
    
    // miscellaneous params (optional)
    bool do_gaussian;
    bool do_contour;
    bool do_overlay;
    bool output_rows;
    int inputted_width;
    int inputted_height;
    const static int MAX_INPUT_WH = 2560; // maximum allowed height, width
    bool flip_x;
    bool flip_y;
    
    // File I/O (optional, recommended)
    bool in_exists;
    string file_in_name;
    string file_out_name;
    
    /*
     * This function takes a vec-rgba containing 2d arrays of r,g,b, and a channels
     * and returns a 1d vector of unsigned chars that is equivalent
     */
    vector<unsigned char> pixels_2d_to_1d(vec_rgba * pixels_2d, unsigned width, unsigned height) {
        unsigned total_pixels = width*height;
        vector<unsigned char> pixels = vector<unsigned char>(total_pixels*4);
        
        for(unsigned i = 0; i < 4*total_pixels; ++i) {
            int tmp = i/4;
            int t_x = tmp%width;
            int t_y = tmp/width;
            if (i%4 == 3) pixels[i] = (unsigned char)pixels_2d->alpha[t_x][t_y];
            else if (i%4 == 2) pixels[i] = (unsigned char)pixels_2d->blue[t_x][t_y];
            else if (i%4 == 1) pixels[i] = (unsigned char)pixels_2d->green[t_x][t_y];
            else if (i%4 == 0) pixels[i] = (unsigned char)pixels_2d->red[t_x][t_y];
        }
        return pixels;
    }
    
    /*
     * This function is the inverse of the above function
     */
    vec_rgba pixels_1d_to_2d(vector<unsigned char> p, unsigned width, unsigned height) {
        unsigned total_pixels = width*height;
        vec_rgba ret;
        ret.red.resize(width);
        ret.green.resize(width);
        ret.blue.resize(width);
        ret.alpha.resize(width);
        for(unsigned i = 0; i < width; ++i) {
            ret.red[i].resize(height);
            ret.green[i].resize(height);
            ret.blue[i].resize(height);
            ret.alpha[i].resize(height);
        }
        
        for(unsigned i = 0; i < 4*total_pixels; ++i) {
            int tmp = i/4;
            int t_x = tmp%width;
            int t_y = tmp/width;
            if(i%4 == 0) ret.red[t_x][t_y] = (float)p[i];
            else if(i%4 == 1) ret.green[t_x][t_y] = (float)p[i];
            else if(i%4 == 2) ret.blue[t_x][t_y] = (float)p[i];
            else if(i%4 == 3) ret.alpha[t_x][t_y] = (float)p[i];
        }
        return ret;
    }
    
    /*
     * This function takes a 2d array of pixels and flips them over the y axis (name may change)
     */
    vec_rgba * pixels_2d_flip_x(vec_rgba * pixels_2d, unsigned width, unsigned height) {
        unsigned x=0;
        unsigned x_t=width-1;
        while(x < x_t) {
            for(unsigned y=0;y<height;y++) {
                unsigned char tmp;
                tmp = pixels_2d->red[x][y];
                pixels_2d->red[x][y] = pixels_2d->red[x_t][y];
                pixels_2d->red[x_t][y] = tmp;
                tmp = pixels_2d->green[x][y];
                pixels_2d->green[x][y] = pixels_2d->green[x_t][y];
                pixels_2d->green[x_t][y] = tmp;
                tmp = pixels_2d->blue[x][y];
                pixels_2d->blue[x][y] = pixels_2d->blue[x_t][y];
                pixels_2d->blue[x_t][y] = tmp;
                tmp = pixels_2d->alpha[x][y];
                pixels_2d->alpha[x][y] = pixels_2d->alpha[x_t][y];
                pixels_2d->alpha[x_t][y] = tmp;
            }
            x++;
            x_t--;
        }

        return pixels_2d;
    }
    
    /*
     * This function is the same as above except flips over the x-axis (again, name may change)
     */
    vec_rgba * pixels_2d_flip_y(vec_rgba * pixels_2d, unsigned width, unsigned height) {
        unsigned y=0;
        unsigned y_t=height-1;
        while(y < y_t) {
            for(unsigned x=0;x<width;x++) {
                unsigned char tmp;
                tmp = pixels_2d->red[x][y];
                pixels_2d->red[x][y] = pixels_2d->red[x][y_t];
                pixels_2d->red[x][y_t] = tmp;
                tmp = pixels_2d->green[x][y];
                pixels_2d->green[x][y] = pixels_2d->green[x][y_t];
                pixels_2d->green[x][y_t] = tmp;
                tmp = pixels_2d->blue[x][y];
                pixels_2d->blue[x][y] = pixels_2d->blue[x][y_t];
                pixels_2d->blue[x][y_t] = tmp;
                tmp = pixels_2d->alpha[x][y];
                pixels_2d->alpha[x][y] = pixels_2d->alpha[x][y_t];
                pixels_2d->alpha[x][y_t] = tmp;
            }
            y++;
            y_t--;
        }

        return pixels_2d;
    }
    
    /*
     * This function overlays the created heat map image over the inputted image
     * This should only be used if the two images are the same size (i.e. use
     * the scale functionality provided in GaussianBlur.h first)
     */
    vector<unsigned char> overlay(vector<unsigned char> source, vector<unsigned char> dest) {
        vector<unsigned char> out = vector<unsigned char>(dest.size());
        for(unsigned int i=0; i<dest.size(); i+=4) {
            int ri = i;
            int gi = i+1;
            int bi = i+2;
            int ai = i+3;
            float s_alpha = (float)source[ai]/(float)255;
            out[ri] = ((float)dest[ri])*(1.0-(float)s_alpha) + ((float)source[ri])*((float)s_alpha);
            out[gi] = ((float)dest[gi])*(1.0-(float)s_alpha) + ((float)source[gi])*((float)s_alpha);
            out[bi] = ((float)dest[bi])*(1.0-(float)s_alpha) + ((float)source[bi])*((float)s_alpha);
            out[ai] = dest[ai];
        }
        return out;
    }
    
    // Ran upon object creation
    virtual void setup(ServerInterface &    srvInterface,
                       const SizedColumnTypes &     argTypes )
    {
        try {
            is_first_row = true;
            bins_x = 0;
            bins_y = 0;
            rows_max_count = 512;
            rows_info.resize(rows_max_count);
            rows_count = 0;
            
            // optional parameters
            if (srvInterface.getParamReader().containsParameter("inf")) {
                file_in_name = ((VString)srvInterface.getParamReader().getStringRef("inf")).str();
                in_exists = true;
            }
            else in_exists = false;
            
            if (srvInterface.getParamReader().containsParameter("outf"))
                file_out_name = ((VString)srvInterface.getParamReader().getStringRef("outf")).str();
            else file_out_name = "/tmp/out.png"; // default output file
            
            if (srvInterface.getParamReader().containsParameter("contour"))
                do_contour = srvInterface.getParamReader().getBoolRef("contour");
            else do_contour = false;
            
            if (srvInterface.getParamReader().containsParameter("overlay"))
                do_overlay = srvInterface.getParamReader().getBoolRef("overlay");
            else do_overlay = false;
            
            if (srvInterface.getParamReader().containsParameter("output_rows"))
                output_rows = srvInterface.getParamReader().getBoolRef("output_rows");
            else output_rows = false;
            
            if (srvInterface.getParamReader().containsParameter("gaussian"))
                do_gaussian = srvInterface.getParamReader().getBoolRef("gaussian");
            else do_gaussian = false;
            
            if (srvInterface.getParamReader().containsParameter("width"))
                inputted_width = srvInterface.getParamReader().getIntRef("width");
            else inputted_width = 0;
            if (srvInterface.getParamReader().containsParameter("height"))
                inputted_height = srvInterface.getParamReader().getIntRef("height");
            else inputted_height = 0;

            flip_x = false;
            if (srvInterface.getParamReader().containsParameter("xflip"))
                flip_x = srvInterface.getParamReader().getBoolRef("xflip");
            flip_y = false;
            if (srvInterface.getParamReader().containsParameter("yflip"))
                flip_y = srvInterface.getParamReader().getBoolRef("yflip");
            
            
            if (inputted_width < 0) inputted_width = 0;
            else if (inputted_width > MAX_INPUT_WH) inputted_width = MAX_INPUT_WH;
            if (inputted_height < 0) inputted_height = 0;
            else if (inputted_height > MAX_INPUT_WH) inputted_height = MAX_INPUT_WH;
            
        } catch(exception& e) {
            // Standard exception. Quit.
            vt_report_error(0, "Exception while processing partition: [%s]", e.what());
        }

    }

    virtual void processPartition(ServerInterface &srvInterface, 
                                  PartitionReader &inputReader, 
                                  PartitionWriter &outputWriter)
    {
        try {
            if (inputReader.getNumCols() != 3)
                vt_report_error(0, "Function only accepts 3 arguments, but %zu provided", inputReader.getNumCols());
            
            
            do {
                const VNumeric &x_numeric = inputReader.getNumericRef(0);
                const VNumeric &y_numeric = inputReader.getNumericRef(1);
                const float &count_fl = inputReader.getFloatRef(2);
                
                float x = x_numeric.toFloat();
                float y = y_numeric.toFloat();
                float c = count_fl;
                
                // Processing inputs (used for normalization, bounding)
                if (is_first_row) {
                    data_min_c = c;
                    data_max_c = c;
                    data_min_x = x;
                    data_max_x = x;
                    data_min_y = y;
                    data_max_y = y;
                    check_x = y;
                    check_y = x;
                    bins_x++;
                    bins_y++;
                    is_first_row = false;
                } else {
                    if (c < data_min_c) data_min_c = c;
                    if (c > data_max_c) data_max_c = c;
                    if (x < data_min_x) data_min_x = x;
                    if (x > data_max_x) data_max_x = x;
                    if (y < data_min_y) data_min_y = y;
                    if (y > data_max_y) data_max_y = y;
                    if (y == check_x) bins_x++;
                    if (x == check_y) bins_y++;
                }
                
                // resizing of row info struct if necessary
                if (rows_count >= rows_max_count) {
                    rows_max_count *= 2;
                    rows_info.resize(rows_max_count);
                }
                rows_info[rows_count].x = x;
                rows_info[rows_count].y = y;
                rows_info[rows_count++].c = c;
                
            } while (inputReader.next());
        
        } catch(exception& e) {
            // Standard exception. Quit.
            vt_report_error(0, "Exception while processing partition: [%s]", e.what());
        }
        
        //////////////////////
        // Create/Write PNG //
        //////////////////////
        
        float scalar = 255./(data_max_c - data_min_c); // normalization
        vector<unsigned char> pixels; // pixels that get saved
        vector<unsigned char> buffer; // input image data
        vector<unsigned char> image; // input image pixels
        unsigned in_w, in_h; // eventual output width and height
        vec_rgba v;
        vec_rgba v_scaled;
        vec_rgba * v_ptr;
        
        // initialize values
        v.red.resize(bins_x);
        v.green.resize(bins_x);
        v.blue.resize(bins_x);
        v.alpha.resize(bins_x);
        for (int i = 0; i < bins_x; ++i) {
            (v.red)[i].resize(bins_y);
            (v.green)[i].resize(bins_y);
            (v.blue)[i].resize(bins_y);
            (v.alpha)[i].resize(bins_y);
        }
        
        // color pixels appropriately (red or contoured)
        for(int i = 0; i < bins_x*bins_y; ++i) {
            int t_x = i%bins_x;
            int t_y = i/bins_x;
            if(do_contour) {
                float val = rows_info[i].c * scalar;
                v.red[t_x][t_y] = 0.;
                v.green[t_x][t_y] = 0.;
                v.blue[t_x][t_y] = 0.;
                v.alpha[t_x][t_y] = 255. * .7;
                if(val > 200.)
                    v.red[t_x][t_y] = 255.;
                else if(val > 125) {
                    v.red[t_x][t_y] = 255;
                    v.green[t_x][t_y] = 255;
                }
                else if(val > 50.) v.green[t_x][t_y] = 255;
                else if(val > 1.) v.blue[t_x][t_y] = 155;
                else v.alpha[t_x][t_y] = 0.;
                    
            }
            else {
                v.red[t_x][t_y] = 255.;
                v.green[t_x][t_y] = 0.;
                v.blue[t_x][t_y] = 0.;
                v.alpha[t_x][t_y] = rows_info[i].c * scalar * .8;
            }
        }
        
        // If an input file exists, then load it and rescale to it
        if(in_exists) {
            load_file(buffer, file_in_name);
            unsigned error = decode(image,in_w,in_h,buffer);
            if(error) {
                vt_report_error(error, "ERROR DECODING GIVEN IMAGE");
            }
            buffer.clear(); // we don't need the input-image data
                            // (besides the pixel data)
            v_scaled = ScaleTo(v,bins_x,bins_y,(int)in_w,(int)in_h);
            v_ptr = &v_scaled;
        }
        else {
            in_w = bins_x;
            in_h = bins_y;
            v_ptr = &v;
        }
        
        // Gaussian Smooth/Blur
        if (do_gaussian) {
            if (do_contour) {
                v_ptr->alpha = GaussianBlur(v_ptr->alpha,in_w,in_h,1.0);
                v_ptr->red = GaussianBlur(v_ptr->red,in_w,in_h,1.0);
                v_ptr->green = GaussianBlur(v_ptr->green,in_w,in_h,1.0);
                v_ptr->blue = GaussianBlur(v_ptr->blue,in_w,in_h,1.0);
            }
            else v_ptr->alpha = GaussianBlur(v_ptr->alpha,in_w,in_h,1.0);
        }
        
        // Flipping and Overlaying
        if(flip_x) pixels_2d_flip_x(v_ptr,in_w,in_h);
        if(flip_y) pixels_2d_flip_y(v_ptr,in_w,in_h);
        pixels = pixels_2d_to_1d(v_ptr,in_w,in_h);
        if(in_exists && do_overlay) pixels = overlay(pixels,image);
        
        // Rescaling to inputted values
        if (inputted_height > 0 && inputted_width > 0) {
            vec_rgba v_ptr_temp = pixels_1d_to_2d(pixels,in_w,in_h);
            vec_rgba v_temp = ScaleTo(v_ptr_temp,(int)in_w,(int)in_h,inputted_width,inputted_height);
            
            in_w = (unsigned)inputted_width;
            in_h = (unsigned)inputted_height;
            
            v_ptr = &v_temp;
            pixels = pixels_2d_to_1d(v_ptr,in_w,in_h);
        }
        
        // Writing .png to file
        unsigned error = lodepng::encode(file_out_name,pixels,in_w,in_h);
        if(error) vt_report_error(error, "ERROR CREATING IMAGE");
        
        // Writing output rows
        if (!output_rows) {
            outputWriter.setInt(0,1);
            outputWriter.next();
        }
        else {
            for(unsigned i = 0; i < in_w; ++i) {
                for(unsigned j = 0; j < in_h; ++j) {
                    int ij_to_pixel = 4*(in_w*i + j);
                    outputWriter.setInt(0,(int)i);
                    outputWriter.setInt(1,(int)j);
                    outputWriter.setInt(2,(int)pixels[ij_to_pixel + 0]);
                    outputWriter.setInt(3,(int)pixels[ij_to_pixel + 1]);
                    outputWriter.setInt(4,(int)pixels[ij_to_pixel + 2]);
                    outputWriter.setInt(5,(int)pixels[ij_to_pixel + 3]);
                    outputWriter.next();
                }
            }
        }
    }
};



class HeatMapImageFactory : public TransformFunctionFactory
{
    virtual void getPrototype(ServerInterface &srvInterface, ColumnTypes &argTypes, ColumnTypes &returnType)
    {
        argTypes.addNumeric();
        argTypes.addNumeric();
        argTypes.addFloat();
        returnType.addInt();
        returnType.addInt();
        returnType.addInt();
        returnType.addInt();
        returnType.addInt();
        returnType.addInt();
    }

    virtual void getReturnType(ServerInterface &srvInterface, 
                               const SizedColumnTypes &inputTypes, 
                               SizedColumnTypes &outputTypes)
    {
        // Error out if we're called with anything but 3 arguments
        if (inputTypes.getColumnCount() != 3)
            vt_report_error(0, "Function only accepts 3 arguments, but %zu provided", inputTypes.getColumnCount());
        
        outputTypes.addInt();
        if (srvInterface.getParamReader().containsParameter("output_rows") && 
                srvInterface.getParamReader().getBoolRef("output_rows") == true) {
            outputTypes.addInt();
            outputTypes.addInt();
            outputTypes.addInt();
            outputTypes.addInt();
            outputTypes.addInt();
        }
    }
    
    virtual void getParameterType(ServerInterface &srvInterface,
                                  SizedColumnTypes &parameterTypes)
    {
        // optional parameters
        parameterTypes.addVarchar(128,"inf");
        parameterTypes.addVarchar(128,"outf");
        parameterTypes.addBool("contour");
        parameterTypes.addBool("overlay");
        parameterTypes.addBool("output_rows");
        parameterTypes.addInt("width");
        parameterTypes.addInt("height");
        parameterTypes.addBool("xflip");
        parameterTypes.addBool("yflip");
        parameterTypes.addBool("gaussian");
    }

    virtual TransformFunction *createTransformFunction(ServerInterface &srvInterface)
    { return vt_createFuncObj(srvInterface.allocator, HeatMapImage); }

};

RegisterFactory(HeatMapImageFactory);
