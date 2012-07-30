/*****************************
 * Vertica Analytic Database
 *
 * Example SQL for User Defined Aggregate Functions
 *
 * Copyright (c) 2005 - 2012 Vertica, an HP company
 */

-- Use Functions

-- Example table used by Heat Map functions
\set datafile '\''`pwd`'/examples/data.data\''
\set cs_data '\''`pwd`'/examples/cs_data/de_dust2v2.data\''
\set cs_png '\''`pwd`'/examples/cs_data/de_dust2.png\''

--Load the tables of data used in examples
CREATE TABLE T (x NUMERIC(10,3), y NUMERIC(10,3), w float);
CREATE TABLE de_dust2 (x int, y int, z int, type char(10));
COPY T (x,y,w AS random()) FROM :datafile DELIMITER ',';
COPY de_dust2 FROM :cs_data DELIMITER ' ';

--Do a sample heatmap generation which demonstrates many of the optional parameters
SELECT heat_map(x,y USING PARAMETERS xbins=5, ybins=3, ymin='0', xmax='0', gaussian=True, bounding_box=True ) OVER () AS (x, y, x_t, y_t, c) FROM T;

--Do a sample heatmap generation which demonstrates using weighted points, the average weight is 0.5 per point so these results should be approximately half as hot as the previous example
SELECT heat_map(x,y,w USING PARAMETERS xbins=5, ybins=3, ymin='0', xmax='0', gaussian=True, bounding_box=True, use_weights=True ) OVER () AS (x, y, x_t, y_t, c) FROM T;

--Do a sample heatmap image generation
SELECT heat_map_image(x,y,c USING PARAMETERS outf='/tmp/testsizer.png') OVER () AS (img) FROM (SELECT heat_map(x,y USING PARAMETERS xbins=64, ybins=64, ymin='0', xmax='0', gaussian=False, bounding_box=True, normalize=2 ) OVER () AS (x, y, x_t, y_t, c) FROM T) AS heatmap LIMIT 50;

--Do a sample heatmap image generation with an overlay, using the previously generated image as a base
SELECT heat_map_image(x,y,c USING PARAMETERS yflip=True, outf='/tmp/test3.png', inf='/tmp/testsizer.png') OVER () AS (img) FROM (SELECT heat_map(x,y USING PARAMETERS xbins=10, ybins=5, ymin='0', xmax='0', gaussian=False, bounding_box=True, normalize=3 ) OVER () AS (x, y, x_t, y_t, c) FROM T) AS heatmap LIMIT 50;

-- This command will use an input .png of a map (de_dust2) from Counter Strike Source and death data taken from playing on the map and outputs to the /tmp/ folder an overlaid image
SELECT heat_map_image(x,y,c USING PARAMETERS inf=:cs_png,outf='/tmp/de_dust2_contour_overlay.png',overlay=True,contour=True,gaussian=True) OVER() AS (Success) FROM (SELECT heat_map(x,y USING PARAMETERS xbins=50,ybins=50,xmin=-2240,ymin=-1080,xmax=1800,ymax=3320,normalize=3,bounding_box=True,gaussian=True) OVER() AS (x,y,x_t,y_t,c) FROM de_dust2) AS heat_map;

-- Cleanup
DROP TABLE T;
DROP TABLE de_dust2;

