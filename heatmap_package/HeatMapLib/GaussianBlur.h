///////////////////////
//// Gaussian Blur ////
////    Mark Fay   ////
///////////////////////

#include <vector>
using namespace std;

/*
 * RGBA struct
 */
typedef struct {
	vector< vector<float> > red;
	vector< vector<float> > green;
	vector< vector<float> > blue;
	vector< vector<float> > alpha;
} vec_rgba;

/*
 * Gaussian Expression (for two dimensions)
 * returns -1 if sigma is 0
 */
float Gaussian2D(float x, float y, float sigma);

/*
 * Takes in a 2D vector, the width and height of the vector,
 * and the desired sigma value for the blurring
 * The blur will create a new vector with the blurred effect
 * and return it.
 */
vector< vector<float> > GaussianBlur(vector< vector<float> > a, int w, int h, float sigma);

/*
 * Scaling function - given a 2d vector of wxh, return 2d vector of w_new x h_new
 * This is used for RGBA pixels - so unsigned chars are being used
 */
vec_rgba ScaleTo(vec_rgba in, int w, int h, int w_new, int h_new);

