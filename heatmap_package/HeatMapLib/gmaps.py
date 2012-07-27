#This Is Python
import sys;
import re;

html_top = """

<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="initial-scale=1.0, user-scalable=no" />
<meta http-equiv="content-type" content="text/html; charset=UTF-8"/>
<title>Google Maps JavaScript API v3 Example: Polygon Auto Close</title>
<style>
html, body {
  height: 100%;
  margin: 0;
  padding: 0;
}

#map_canvas {
  height: 100%;
}

@media print {
  html, body {
    height: auto;
  }

  #map_canvas {
    height: 650px;
  }
}
</style>
<script type="text/javascript" src="http://maps.googleapis.com/maps/api/js?key=AIzaSyB0GFtDWM8Zng_EjU3USnXepgKnV8fyz7k&sensor=false"></script>
<script type="text/javascript">

  function initialize() {
    var myLatLng = new google.maps.LatLng(24.886436490787712, -70.2685546875);
    var myOptions = {
      zoom: 5,
      center: myLatLng,
      mapTypeId: google.maps.MapTypeId.ROADMAP
    };

    var map = new google.maps.Map(document.getElementById("map_canvas"),
        myOptions);
"""

def ctc(w):
    cint = str(hex(int(w*255)))
    cint = cint[2:]
    if len(cint) == 1:
        cint = "0"+cint
    if len(cint) == 0:
        cint = "00"
    return str(cint+"0000")
    

def create_rect(name, x,y,x_t,y_t,c ):
    name = str(name)
    x = str(x)
    y = str(y)
    x_t = str(x_t)
    y_t = str(y_t)
    rect = "var "+name+"Rect\n\n"
    rect += "var "+name+"Coords = [\n"
    rect += "new google.maps.LatLng("+x+", "+y+"),\n"
    rect += "new google.maps.LatLng("+x_t+", "+y+"),\n"
    rect += "new google.maps.LatLng("+x_t+", "+y_t+"),\n"
    rect += "new google.maps.LatLng("+x+", "+y_t+"),\n"
    rect += "new google.maps.LatLng("+x+", "+y+")\n"
    rect += "];\n\n"
    rect += name+"Rect = new google.maps.Polygon({\n"
    rect += "paths: "+name+"Coords,\n"
    rect += "strokeColor: \"#"+ctc(c)+"\",\n"
    rect += "strokeOpacity: 0.9,\n"
    rect += "strokeWeight: 0,\n"
    rect += "fillColor: \"#"+ctc(c)+"\",\n"
    rect += "fillOpacity: 0.6\n"
    rect += "});\n\n"
    rect += name+"Rect.setMap(map);\n"
    return rect


bermuda = """
var bermudaTriangle;

var triangleCoords = [
        new google.maps.LatLng(25.774252, -80.190262),
        new google.maps.LatLng(18.466465, -66.118292),
        new google.maps.LatLng(32.321384, -64.75737)
    ];

    bermudaTriangle = new google.maps.Polygon({
      paths: triangleCoords,
      strokeColor: "#FF0000",
      strokeOpacity: 0.99,
      strokeWeight: 1,
      fillColor: "#FF0000",
      fillOpacity: 0.99
    });

   bermudaTriangle.setMap(map);
"""

html_bot = """
}
</script>
</head>
<body onload="initialize()">
  <div id="map_canvas"></div>
</body>
</html>
"""

print html_top;
#print bermuda;
count = 0
for line in sys.stdin:
    args=line.split("|")
    if len(args) > 4:
        x = float(args[0])
        y = float(args[1])
        x_t=float(args[2])
        y_t=float(args[3])
        c = float(args[4])
        count = count + 1
        name=str(count)
        print create_rect("r"+name, x,y,x_t,y_t,c)
print html_bot;
