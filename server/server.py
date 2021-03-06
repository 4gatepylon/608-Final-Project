from email.mime import image
import sqlite3
from datetime import datetime, timedelta
import os
import json
import base64
import requests
import os

from typing import Callable, List, Tuple, Any

directory = '/var/jail/home/'

API_KEY = "AIzaSyBllku4kCJWbkSc0LuhKZxskgsG_g1YcGU"

#API_KEY = json.dumps(API_KEY)

HTML_HEADER: str = """
<!DOCTYPE html>
<html>
<head>
	<style>
		table {
			font-family: arial, sans-serif;
			border-collapse: collapse;
			width: 100%;
		}
		td,
		th {
			border: 1px solid #dddddd;
			text-align: left;
			padding: 8px;
		}
		h2 {
			font-family: arial, sans-serif;
		}
		body {
			padding: 40px;
		}
		tr:nth-child(even) {
			background-color: #dddddd;
		}
	</style>
</head>
<body>
	<h2>Locations</h2>
	<table>
		<tr>
			<th>Location</th>
            <th>Time</th>
            <th>Lat</th>
            <th>Lon</th>
		</tr>
"""

HTML_FOOTER: str = "</table></body></html>"




def LOCATIONS_HTML(timestamped_locations):
    # Have to reverse because we add from top to bottom and want newest first
    entries = "".join(reversed([f"<tr><td>{name}</td><td>{time}</td><td>{lat}</td><td>{lon}</td></tr>" for time, name, lat, lon in timestamped_locations]))
    return HTML_HEADER + entries + HTML_FOOTER

'''
def detect_labels(path):
    """Detects labels in the file."""

    
    client = vision.ImageAnnotatorClient()

    with io.open(path, 'rb') as image_file:
        content = image_file.read()

    image = vision.Image(content=content)

    response = client.label_detection(image=image)

    objects = client.object_localization(
        image=image).localized_object_annotations
    objects.keys 

    print('Number of objects found: {}'.format(len(objects)))
    for object_ in objects:
        print('\n{} (confidence: {})'.format(object_.name, object_.score))
        print('Normalized bounding polygon vertices: ')
        for vertex in object_.bounding_poly.normalized_vertices:
            print(' - ({}, {})'.format(vertex.x, vertex.y))
    # see if object right in front 

    labels = response.label_annotations

    return labels

    if response.error.message:
        raise Exception(
            '{}\nFor more info on error messages, check: '
            'https://cloud.google.com/apis/design/errors'.format(
                response.error.message))

    # provides labels of images 
'''

class GeoFencer(object):
    MIT_LOCATIONS = {
        "Student Center": [
            (-71.095863,42.357307),
            (-71.097730,42.359075),
            (-71.095102,42.360295),
            (-71.093900,42.359340),
            (-71.093289,42.358306),
        ],
        "Dorm Row": [
            (-71.093117,42.358147),
            (-71.092559,42.357069),
            (-71.102987,42.353866),
            (-71.106292,42.353517),
        ],
        "Simmons/Briggs": [
            (-71.097859,42.359035),
            (-71.095928,42.357243),
            (-71.106356,42.353580),
            (-71.108159,42.354468),
        ],
        "Boston FSILG (West)": [
            (-71.124664,42.353342),
            (-71.125737,42.344906),
            (-71.092478,42.348014),
            (-71.092607,42.350266),
        ],
        "Boston FSILG (East)": [
            (-71.092409,42.351392),
            (-71.090842,42.343589),
            (-71.080478,42.350900),
            (-71.081766,42.353771),
        ],
        "Stata/North Court": [
            (-71.091636,42.361802),
            (-71.090950,42.360811),
            (-71.088353,42.361112),
            (-71.088267,42.362476),
            (-71.089769,42.362618),
        ],
        "East Campus": [
            (-71.089426,42.358306),
            (-71.090885,42.360716),
            (-71.088310,42.361017),
            (-71.087130,42.359162),
        ],
        "Vassar Academic Buildings": [
            (-71.094973,42.360359),
            (-71.091776,42.361770),
            (-71.090928,42.360636),
            (-71.094040,42.359574),
        ],
        "Infinite Corridor/Killian": [
            (-71.093932,42.359542),
            (-71.092259,42.357180),
            (-71.089619,42.358274),
            (-71.090928,42.360541),
        ],
        "Kendall Square": [
            (-71.088117,42.364188),
            (-71.088225,42.361112),
            (-71.082774,42.362032),
        ],
        "Sloan/Media Lab": [
            (-71.088203,42.361017),
            (-71.087044,42.359178),
            (-71.080071,42.361619),
            (-71.082796,42.361905),
        ],
        "North Campus": [
            (-71.11022,42.355325),
            (-71.101280,42.363934),
            (-71.089950,42.362666),
            (-71.108361,42.354484),],
        "Technology Square": [
            (-71.093610,42.363157),
            (-71.092130,42.365837),
            (-71.088182,42.364188),
            (-71.088267,42.362650),
        ],
    }

    """ From Lab 5A (`geofencing`) ... tells you when you are in some given location """
    def __init__(self):
        pass

    # NOTE: this is not used yet
    def bounding_box(point_coord: Tuple[int, int], box: Tuple[int, int]) -> bool:
        x, y = point_coord

        left = min(box, key=lambda tp: tp[0])[0]
        right = max(box, key=lambda tp: tp[0])[0]
        top = min(box, key=lambda tp: tp[1])[1]
        bot = max(box, key=lambda tp: tp[1])[1]
        
        return left <= x and x <= right and top <= y and y <= bot
    
    def within_area(point_coord: Tuple[int, int], poly: List[Tuple[int, int]]) -> bool:
        x0, y0 = point_coord

        # Zero out the polygon to use the algorithm described in the lab (5A, linked here
        # https://iesc.io/608/S22/labs/lab05a)
        poly = [(xp-x0, yp-y0) for (xp, yp) in poly]

        # The idea is to see whether you are inside the polygon by seeing if
        # a horizontal line coming out of it (to the right) from your point
        # (normalized to 0, 0) would intersect an edge an odd number of times
        # (even means every time you went in you came out, so if you came out
        # you had to go in, so you could not have been inside, but further to the left).
        count = 0
        for i in range(len(poly)):
            # Look at every edge (in this representation it's the consecutive pairs of points)
            # and add to the counter if we cross it. This will tell us how many times we intersect
            # the polygon.
            xp0, yp0 = poly[i]
            xp1, yp1 = (poly[i+1] if i+1 < len(poly) else poly[0])
            if yp0 * yp1 > 0:
                # Case 1: the edge does not cross the x-axis, so we can ignore it
                # (are below or above it)
                continue
            elif xp0 < 0 and xp1 < 0:
                # Case 2: the edge is entirely to the left of the point, so we can ignore it
                continue
            elif xp0 > 0 and xp1 > 0:
                # Case 3: The edge cross the x-axis and is to the right of the point entirely
                # so it must be counted.
                count += 1
            else:
                # Case 4: The edge is both to the left and to the right of the y-axis as well
                # as crossing the x-axis, so we need to check whether it is in front of the point
                # or not (i.e. it might not be if it goes up from the left to right).
                dy = yp1 - yp0
                revdot = -xp1 * yp0 + xp0 * yp1
                p = revdot / dy
                if p > 0:
                    count += 1
        return count % 2 == 1
    
    def get_area(point_coord: Tuple[int, int]):
        for name, poly in GeoFencer.MIT_LOCATIONS.items():
            if GeoFencer.within_area(point_coord, poly) % 2 == 1:
                return name
        return "Off Campus"

class Crud(object):
    DB_FILE = "/var/jail/home/team10/robot.db" 
    CAM_FILE = "/var/jail/home/team10/camera.db" 
    LOC_FILE = "/var/jail/home/team10/loc.db" 
    PATHS_FILE = "/var/jail/home/team10/paths.db"

    PATHS_TABLE = "paths"

    def __init__(self):
        pass

    # Unclear why you can't @staticmethod these
    # (I don't know what it does technically)
    # Check out
    # https://stackoverflow.com/questions/41921255/staticmethod-object-is-not-callable
    # (don't hae the time and it's not that important)

    def withConnCursor(func: Callable[[sqlite3.Cursor, sqlite3.Connection, Any], str]) -> str:
        """ Wrap your functions in this when you want them to have access to the database"""
        def wrapper(*args, **kwargs):
            conn = sqlite3.connect(Crud.DB_FILE)
            c = conn.cursor()
            result = func(c, conn, *args, **kwargs)
            conn.commit()
            conn.close()
            return result
        return wrapper

    def withConnCamCursor(func: Callable[[sqlite3.Cursor, sqlite3.Connection, Any], str]) -> str:
        """ Wrap your functions in this when you want them to have access to the database"""
        def wrapper(*args, **kwargs):
            conn = sqlite3.connect(Crud.CAM_FILE)
            c = conn.cursor()
            result = func(c, conn, *args, **kwargs)
            conn.commit()
            conn.close()
            return result
        return wrapper
    
    def withConnLocCursor(func: Callable[[sqlite3.Cursor, sqlite3.Connection, Any], str]) -> str:
        """ Wrap your functions in this when you want them to have access to the database"""
        def wrapper(*args, **kwargs):
            conn = sqlite3.connect(Crud.LOC_FILE)
            c = conn.cursor()
            result = func(c, conn, *args, **kwargs)
            conn.commit()
            conn.close()
            return result
        return wrapper
    
    def withConnPathCursor(func: Callable[[sqlite3.Cursor, sqlite3.Connection, Any], str]) -> str:
        """ Wrap your functions in this when you want them to have access to the database"""
        def wrapper(*args, **kwargs):
            conn = sqlite3.connect(Crud.PATHS_FILE)
            c = conn.cursor()
            result = func(c, conn, *args, **kwargs)
            conn.commit()
            conn.close()
            return result
        return wrapper

    @withConnCursor
    def handle_db_api_get(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        data = c.execute("""SELECT * FROM full_data ORDER BY time_ ASC;""").fetchall()

        a_x_vals = []
        a_y_vals = []
        v_x_vals = []
        v_y_vals = []
        #x_x_vals = []
        #x_y_vals = []
        speeds = []
        directions = []
        times = []
        #buildings = []

        for time_, a_x, a_y, v_x, v_y, speed, direction, in data:
            try:
                dto = datetime.strptime(time_,"%Y-%m-%d %H:%M:%S.%f")
            except:
                dto = datetime.strptime(time_,"%Y-%m-%d %H:%M:%S")
            times.append(dto.strftime("%m/%d/%Y, %H:%M:%S"))
            # acceleration is tilt
            a_x_vals.append(a_x)
            a_y_vals.append(a_y)
            # velocity is unused
            v_x_vals.append(v_x)
            v_y_vals.append(v_y)
            # x/y is now the lat/lon
            #x_x_vals.append(x_x)
            #x_y_vals.append(x_y)
            speeds.append(speed)
            directions.append(direction)
            #buildings.append(building)

        result_dict = {"times": times, "a_x": a_x_vals, "a_y": a_y_vals, "v_x": v_x_vals, "v_y": v_y_vals, 'speeds': speeds, 'directions': directions}
        return json.dumps(result_dict)
    
    @withConnLocCursor 
    def handle_whereami(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        if not "x" in request["values"] or not "y" in request["values"]:
            return "Error: please provide x and y"
        x_str: str = request["values"]["x"]
        y_str: str = request["values"]["y"]
        
        try:
            x: float = float(x_str)
            y: float = float(y_str)
            loc: str = GeoFencer.get_area((x, y))
            now = datetime.now()
            # post to database 
            c.execute("""CREATE TABLE IF NOT EXISTS loc_data (time_ timestamp, x_x real, y_y real, build text);""")
            c.execute('''INSERT into loc_data VALUES (?,?,?,?);''', (now, x,y,loc))
            return loc 
        except Exception as e:
            return f"Error: please provide x and y as floats, had error: {e}"
    
    @withConnLocCursor 
    def handle_wherehaveibeen(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        data = c.execute("""SELECT * FROM loc_data ORDER BY time_ ASC LIMIT 20;""").fetchall() 
        #tlocs_ = [(time_, (float(x_x), float(x_y))) for (time_, x_x, x_y) in data]
        # In theory the building is necessary, but it is what it is
        #tlocs = [(time_, GeoFencer.get_area(loc)) for (time_, loc) in tlocs_] # time and location every time 
        alldata = []
        for time, lat, lon, building in data:
            x = { }
            x["time"] = time
            x["x_x"] = lat
            x["x_y"]=lon
            x["building"] = building 
            alldata.append(x)
        return json.dumps({"data" : alldata})
        #return LOCATIONS_HTML(tlocs) 

    @withConnCursor
    def handle_db_api_post(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        now = datetime.now()
        a_x = float(request['form']['a_x'])
        a_y = float(request['form']['a_y'])
        v_x = float(request['form']['v_x'])
        v_y = float(request['form']['v_y'])
        speed = request['form']['speed']
        direction = request['form']['dir']
        c.execute("""CREATE TABLE IF NOT EXISTS full_data (time_ timestamp, a_x real, a_y real, v_x real, v_y real, speed real, direction real);""")
        c.execute('''INSERT into full_data VALUES (?,?,?,?,?,?,?);''', (now, a_x, a_y, v_x, v_y,speed, direction))
        return "done" 
    
    @withConnCamCursor
    def handle_camera_post(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        now = datetime.now() 
        json_camera = json.loads(request['data']) 
        
        coded_string =  json_camera['fullimg'].split("=")[0].split("data:image/gif;base64,")[1] #+ "=="
        #byte_string = coded_string.encode()
        #filename = '/var/jail/home/team10/cam.jpeg'  # I assume you have a way of picking unique filenames
        #with open(filename, 'wb') as f: 
            #f.write(base64.decodebytes(byte_string)) # gets upddated everytime 
            #f.close()
        os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = "var/jail/home/team10/key.json" 
        request_dict = {
            "requests":[
                {
                "image":{
                    "content":coded_string
                },
                "features":[
                    {
                    "type":"OBJECT_LOCALIZATION",
                    "maxResults":3
                    }
                ]
                }
            ]
            } 
        # parse response 
        response = requests.post(
            url='https://vision.googleapis.com/v1/images:annotate?key={}'.format(API_KEY),
            data=json.dumps(request_dict), 
            headers={'Content-Type': 'application/json'}
        ).json()['responses'][0]['localizedObjectAnnotations'][0]['name']
        # send this to database 
        # send to sender tft? 
        # sent to tft 
        c.execute("""CREATE TABLE IF NOT EXISTS cam_data (time_ timestamp, image text, response text);""")
        #c.execute('''DELETE FROM cam_data''')
        c.execute('''INSERT into cam_data VALUES (?,?,?);''', (now, json_camera['fullimg'],response))
        return response 
    
    # get the image from the cam_data database. Send back the most recent image
    @withConnCamCursor
    def handle_camera_get(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        c.execute("""SELECT * FROM cam_data ORDER BY time_ DESC;""") 
        data = c.fetchone()
        
        # NOTE image decoded is already decoded
        image_decoded = data[1]
        object_detected = data[2] 
        image_decoded = image_decoded.split("=")[0] 
       
        if request['values']['camera']=='1':
             return json.dumps({"image_decoded": image_decoded, "object": object_detected})
        else: # if camera is the other one send 
            return object_detected

        
        # send through object detection and result 

    def handle_camera_get_all():

        cam_conn = sqlite3.connect(Crud.CAM_FILE)
        cam_c = cam_conn.cursor()
        cam_c.execute("""SELECT * FROM cam_data ORDER BY time_ DESC LIMIT 10;""") 
        cam_data = cam_c.fetchall() 

        loc_conn = sqlite3.connect(Crud.LOC_FILE)
        loc_c = loc_conn.cursor()

        loc_c.execute(f"""SELECT * from loc_data ORDER BY time_ DESC LIMIT 1;""")
        default_loc = loc_c.fetchall()

        img_array = []
        for cam_t, img, object in cam_data:
            cam_t_obj = datetime.strptime(cam_t, "%Y-%m-%d %H:%M:%S.%f")
            end_t_obj = cam_t_obj + timedelta(minutes=0.1)
            start_t_obj = cam_t_obj - timedelta(minutes=0.1)
            end_t = end_t_obj.strftime("%Y-%m-%d %H:%M:%S.%f")
            start_t = start_t_obj.strftime("%Y-%m-%d %H:%M:%S.%f")
            loc_c.execute(f"""SELECT * from loc_data WHERE time_ BETWEEN '{start_t}' AND '{end_t}';""")
            close_loc = loc_c.fetchall()
            
            if len(close_loc) != 0:
                img_array.append([img.split("=")[0], object, cam_t, close_loc[0]])
            else:
                img_array.append([img.split("=")[0], object, cam_t, default_loc[0]])
        
        return json.dumps({"result": img_array})

        #c.execute("""SELECT * FROM loc_data ORDER BY time_ DESC;""") 
        #loc_data = c.fetchall()
        # for every time 
        # NOTE image decoded is already decoded
        #image_decoded = data[1]
        #object_detected = data[2] 
        # remove the last //9k=UBcbimsKAExTSKQj/9k=2Q==
        #image_decoded = image_decoded.split("=")[0] 
        # send through object detection and result 
        # get location 
    
    # Uses a slightly different syntax so it's easier to type
    @withConnPathCursor
    def handle_path_create(c: sqlite3.Cursor, conn: sqlite3.Connection, request):
        assert(request["method"] == 'POST')
        assert("values" in request)
        values = request["values"]
        assert("pathcreate" in values)
        if not "name" in values:
            return "No name provided"
        name = values['name']
        # You are able to specify to the granularity of days (this is also meant to work on urls)
        if not "start" in values:
            return "No start date provided, format is YYYYMMDD"
        if not "end" in values:
            return "No end date provided, format is YYYYMMDD"
        start = datetime.strptime(values['start'], "%Y%m%d")
        end = datetime.strptime(values['end'], "%Y%m%d")
        if (end < start):
            return "End date is before start date"

        c.execute(f"""CREATE TABLE IF NOT EXISTS {Crud.PATHS_TABLE} (name text, start timestamp, end timestamp);""")
        data = c.execute(f"""SELECT * FROM {Crud.PATHS_TABLE};""").fetchall()
        if len(data) > 0:
            return f"Bad name {name} was already in use"
        c.execute(f'''INSERT into {Crud.PATHS_TABLE} VALUES (?,?,?);''', (name, start, end))
        return "OK"
    
    @withConnCursor
    def get_all_path_between(c: sqlite3.Cursor, conn: sqlite3.Connection, start, end):
        assert(type(start) == datetime)
        assert(type(end) == datetime)

        # NOTE this is copied from `handle_data_api_get`
        data = c.execute("""SELECT * FROM loc_data WHERE (? < time_ AND time_ < ?) ORDER BY time_ ASC LIMIT 20;""", (start, end)).fetchall() 
        #tlocs_ = [(time_, (float(x_x), float(x_y))) for (time_, x_x, x_y) in data]
        # In theory the building is necessary, but it is what it is
        #tlocs = [(time_, GeoFencer.get_area(loc)) for (time_, loc) in tlocs_] # time and location every time 
        alldata = []
        for time, lat, lon, building in data:
            time = time if type(time) == str else \
                time.strftime("%Y-%m-%d %H:%M:%S.%f") if type(time) == datetime else \
                    str(time)
            alldata.append((time, building, str(lat), str(lon)))
        return LOCATIONS_HTML(alldata)
    
    @withConnPathCursor
    def handle_path_get(c: sqlite3.Cursor, conn: sqlite3.Connection, request):
        assert(request["method"] == 'GET')
        assert("values" in request)
        values = request["values"]
        assert("pathget" in values)
        name = values['name']

        c.execute(f"""CREATE TABLE IF NOT EXISTS {Crud.PATHS_TABLE} (name text, start timestamp, end timestamp);""")
        data = c.execute(f"""SELECT * FROM {Crud.PATHS_TABLE} WHERE name = ?;""", (name,)).fetchone()
        assert(len(data) == 3)
        realname, start, end = data
        assert(name == realname)
        start = datetime.strptime(start, "%Y-%m-%d %H:%M:%S") if type(start) == str else None
        end = datetime.strptime(end, "%Y-%m-%d %H:%M:%S") if type(end) == str else None
        if start is None or end is None:
            return "Error: had no start or end for an existing entry in the database (or was not datetime parsable string)"
        
        # Returns an HTML page
        return Crud.get_all_path_between(start, end)

        

class Webpage(object):
    # The build folder in build/ is generated by react and you can copy it like this:
    # `scp -r build team10@608dev-2.net:/home/team10/` after doing `npm run build` in
    # the webapp dirctory
    INDEX_FILE = "/var/jail/home/team10/build/index.html"
    def __init__(self):
        pass
    def handle_webpage_get(request: Any):
        if os.path.exists(Webpage.INDEX_FILE):
            with open(Webpage.INDEX_FILE, "r") as html_file:
                return html_file.read()
        else:
            return "Index not found"
    
    def handle_mona_lisa(request: Any):
        png_bytes = None
        with open('/var/jail/home/team10/mona_lisa.png', 'rb') as f:
            png_bytes = f.read()
        if png_bytes is None or len(png_bytes) == 0:
            return "Error: no image"
        base64bytes = base64.b64encode(png_bytes)
        base64bytes = base64bytes.decode('utf-8')
        return f"""
        <!DOCTYPE html>
        <html>
        <head>
        </head>
        <body>
            <img src="data:image/png;base64,{base64bytes}">
        </body>
        </html>
        """

def request_handler(request: Any):
    if request['method'] == 'POST':
        has_value = "values" in request and len(request["values"]) > 0
        pathcreate ="values" in request and  "pathcreate" in request["values"]
        if pathcreate:
            return Crud.handle_path_create(request)
        if has_value: 
            if "camera" in request["values"]:
                return Crud.handle_camera_post(request)
            elif "whereami" in request["values"]: 
                return Crud.handle_whereami(request)
        else:
            return Crud.handle_db_api_post(request)
    if request["method"] == "GET":
        has_value = "values" in request and len(request["values"]) > 0
        camera = "camera" in request["values"]
        camera1 = "allcamera" in request["values"]
        if camera1:
            return Crud.handle_camera_get_all()
        pathget = "values" in request and "pathget" in request["values"]
        if pathget:
            return Crud.handle_path_get(request)
        if camera:
            return Crud.handle_camera_get(request)
        
        if has_value:
            if "wherehaveibeen" in request["values"]:
                return Crud.handle_wherehaveibeen(request)
            elif "monalisa" in request["values"]:
                return Webpage.handle_mona_lisa(request)
            # values=1 for db api request
            return Crud.handle_db_api_get(request)
        else:
            return Webpage.handle_webpage_get(request)
