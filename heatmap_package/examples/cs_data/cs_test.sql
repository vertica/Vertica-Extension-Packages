/***************************
 * Image Testing SQL Commands
 * To be used on database
 * set up with cs_data
 */

DROP LIBRARY TestHeatMapFunctions CASCADE;

-- Step 1: Create LIBRARY 
\set libfile '\''`pwd`'/lib/HeatMapLib.so\'';
CREATE LIBRARY TestHeatMapFunctions AS :libfile;

-- Step 2: Create Functions
CREATE TRANSFORM FUNCTION heat_map AS LANGUAGE 'C++'
NAME 'HeatMapFactory' LIBRARY TestHeatMapFunctions;

CREATE TRANSFORM FUNCTION heat_map_image AS LANGUAGE 'C++'
NAME 'HeatMapImageFactory' LIBRARY TestHeatMapFunctions;

\set datafile '\''`pwd`'/data.data\''

-- SQL COMMANDS
--SELECT heat_map(x,y USING PARAMETERS xbins=40,ybins=40,xmin=-1820,ymin=-2200,xmax=2350,ymax=1300,gaussian=True,normalize=3,bounding_box=True) OVER (ORDER BY 1) AS (x, y, x_t, y_t, c) FROM cs_office;
SELECT heat_map_image(x,y,c USING PARAMETERS inf='/home/mfay/HeatMap/examples/cs_data/de_train.png',outf='/home/mfay/HeatMap/examples/cs_data/de_train_contour_overlay.png',output_rows=True,overlay=True,contour=True,gaussian=True) OVER (ORDER BY 1) AS (x,y,r,g,b,a) FROM (SELECT heat_map(x,y USING PARAMETERS xbins=50,ybins=50,xmin=-2000,ymin=-1800,xmax=2050,ymax=1900,normalize=3,bounding_box=True,gaussian=True) OVER (ORDER BY 1) AS (x, y, x_t, y_t, c) FROM cs_train) AS heat_map LIMIT 50;

-- Step 4: Cleanup
DROP LIBRARY TestHeatMapFunctions CASCADE;
