///////////////////////
//// Gaussian Blur ////
////    Mark Fay   ////
///////////////////////

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "GaussianBlur.h"
#define _USE_MATH_DEFINES

using namespace std;

/*
 * Gaussian Expression (for two dimensions)
 * returns -1 if sigma is 0
 */
 float Gaussian2D(float x, float y, float sigma) {
    float coef = 1/sqrt(2*M_PI*pow(sigma,2.));
    float xy = 0. - (pow(x,2.) + pow(y,2.));
    float denom = 2 * pow(sigma,2.);
    return (denom != 0) ? coef*exp(xy/denom) : -1.;
 }

/*
 * Takes in a 2D vector, the width and height of the vector,
 * and the desired sigma value for the blurring
 * The blur will create a new vector with the blurred effect
 * and return it.
 */
vector< vector<float> > GaussianBlur(vector< vector<float> > a, int w, int h, float sigma) {
    int min = (w > h) ? h : w;
    int bound = (int)(.01 * (float)min+1.0);
    
    vector< vector<float> > blurredimg;
    blurredimg.resize(w);
    for(int i = 0; i < w; ++i) {
        blurredimg[i].resize(h);
    }
    
    vector< vector<float> > GWM; // Gaussian Weight Matrix - optimization
    int dblbnd = 2*bound + 1;
    GWM.resize(dblbnd);
    for(int i = 0; i < dblbnd; ++i) {
        GWM[i].resize(dblbnd);
        for (int j = 0; j < dblbnd; ++j) {
            GWM[i][j] = -1.;
        }
    }
    
    // for each pixel/bucket:
    for(int px = 0; px < w; ++px) {
        for(int py = 0; py < h; ++py) {
            float finalVal = 0;
            float G = 0;
            // for each nearby pixel/bucket
            // 10% of min side-length
            int gx = px - bound;
            float count = 0.;
            while (gx <= px + bound) {
                int gy = py - bound;
                while (gy <= py + bound) {
                    if(gx >= 0 && gy >= 0 && gx < w && gy < h) {
                        float gauss;
                        if(GWM[gx+bound-px][gy+bound-py] != -1.) {
                            gauss = Gaussian2D(gx-px,gy-py,sigma);
                            GWM[gx+bound-px][gy+bound-py] = gauss;
                            // possible optimization: add value to cells
                            // with the same value: i.e. mirror cells
                        }
                        else {
                            gauss = GWM[gx+bound-px][gy+bound-py];
                        }
                        count += gauss;
                        G += a[gx][gy]*gauss;
                    }
                    gy += 1;
                }
                gx += 1;
            }
            // sum Gs
            finalVal += G/count;
            blurredimg[px][py] = finalVal;
        }
    }
    
    return blurredimg;
}

/*
 * Scaling function - given a 2d vector of wxh, return 2d vector of w_new x h_new
 * This is used for RGBA pixels - so unsigned chars are being used
 */
vec_rgba ScaleTo(vec_rgba in, int w, int h, int w_new, int h_new) {
    vec_rgba out;
    
    int p_size_x = (int)((float)w_new/(float)w + 1.); // new pixel sizes
    int p_size_y = (int)((float)h_new/(float)h + 1.);
    float sc_x = (float)w_new/(float)w;
    float sc_y = (float)h_new/(float)h;
    
    out.red.resize(w_new);
    out.blue.resize(w_new);
    out.green.resize(w_new);
    out.alpha.resize(w_new);
    for(int i = 0; i < w_new; ++i) {
        (out.red)[i].resize(h_new);
        (out.green)[i].resize(h_new);
        (out.blue)[i].resize(h_new);
        (out.alpha)[i].resize(h_new);
        for(int j = 0; j < h_new; ++j) {
            (out.red)[i][j] = 255;
            (out.green)[i][j] = 255;
            (out.blue)[i][j] = 255;
            (out.alpha)[i][j] = 0;
        }
    }
    
    for(int i = 0; i < w; ++i) {
        for(int j = 0; j < h; ++j) {
            int x = (int)(sc_x*(float)i);
            int y = (int)(sc_y*(float)j);
            if(x<0 || x>=w_new || y<0 || y>=h_new)
                return out;
            (out.red)[x][y] = (in.red)[i][j];
            (out.green)[x][y] = (in.green)[i][j];
            (out.blue)[x][y] = (in.blue)[i][j];
            (out.alpha)[x][y] = (in.alpha)[i][j];
            for(int a = 0; a < p_size_x; ++a) {
                for(int b = 0; b < p_size_y; ++b) {
                    if(x+a<0 || x+a>=w_new || y+b<0 || y+b>=h_new)
                        continue;   // if out of bounds, continue on instead
                                    // of causing an index exception
                    (out.red)[x+a][y+b] = (out.red)[x][y];
                    (out.green)[x+a][y+b] = (out.green)[x][y];
                    (out.blue)[x+a][y+b] = (out.blue)[x][y];
                    (out.alpha)[x+a][y+b] = (out.alpha)[x][y];
                }
            }
        }
    }
    
    return out;
 }


