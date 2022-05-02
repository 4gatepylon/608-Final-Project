from email.mime import image
import sqlite3
from datetime import datetime
import os
import json
import base64

from typing import Callable, List, Tuple, Any

directory = '/var/jail/home/'
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
	<h2>Highscores</h2>
	<table>
		<tr>
			<th>Location</th>
            <th>Time</th>
		</tr>
"""

HTML_FOOTER: str = "</table></body></html>"

def LOCATIONS_HTML(timestamped_locations):
    # Have to reverse because we add from top to bottom and want newest first
    entries = "".join(reversed([f"<tr><td>{name}</td><td>{time}</td></tr>" for time, name in timestamped_locations]))
    return HTML_HEADER + entries + HTML_FOOTER

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
    DB_FILE = "/var/jail/home/team10/information.db" 
    CAM_FILE = "/var/jail/home/team10/cam.db" 

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
    
    @withConnCursor
    def handle_db_api_get(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        data = c.execute("""SELECT * FROM full_data ORDER BY time_ ASC;""").fetchall()

        a_x_vals = []
        a_y_vals = []
        v_x_vals = []
        v_y_vals = []
        x_x_vals = []
        x_y_vals = []
        speeds = []
        directions = []
        times = []
        buildings = []

        for time_, a_x, a_y, v_x, v_y, x_x, x_y, speed, direction, building in data:
            dto = datetime.strptime(time_,"%Y-%m-%d %H:%M:%S.%f")
            times.append(dto.strftime("%m/%d/%Y, %H:%M:%S"))
            # acceleration is tilt
            a_x_vals.append(a_x)
            a_y_vals.append(a_y)
            # velocity is unused
            v_x_vals.append(v_x)
            v_y_vals.append(v_y)
            # x/y is now the lat/lon
            x_x_vals.append(x_x)
            x_y_vals.append(x_y)
            speeds.append(speed)
            directions.append(direction)
            buildings.append(building)

        result_dict = {"times": times, "a_x": a_x_vals, "a_y": a_y_vals, "v_x": v_x_vals, "v_y": v_y_vals, "x_x": x_x_vals, "x_y": x_y_vals, 'speeds': speeds, 'directions': directions, 'buildings': buildings}
        return json.dumps(result_dict)
    
    @withConnCursor
    def handle_whereami(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        if not "x" in request["values"] or not "y" in request["values"]:
            return "Error: please provide x and y"
        x_str: str = request["values"]["x"]
        y_str: str = request["values"]["y"]
        try:
            x: float = float(x_str)
            y: float = float(y_str)
            loc: str = GeoFencer.get_area((x, y))
            return loc
        except Exception as e:
            return f"Error: please provide x and y as floats, had error: {e}"
    
    @withConnCursor
    def handle_wherehaveibeen(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        data = c.execute("""SELECT * FROM full_data ORDER BY time_ ASC;""").fetchall()
        tlocs_ = [(time_, (float(x_x), float(x_y))) for (time_, _, _, _, _, x_x, x_y, _, _, build) in data]
        # In theory the building is necessary, but it is what it is
        tlocs = [(time_, GeoFencer.get_area(loc)) for (time_, loc) in tlocs_]
        return LOCATIONS_HTML(tlocs)

    @withConnCursor
    def handle_db_api_post(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        now = datetime.now()
        a_x = float(request['form']['a_x'])
        a_y = float(request['form']['a_y'])
        v_x = float(request['form']['v_x'])
        v_y = float(request['form']['v_y'])
        x_x = float(request['form']['x_x'])
        x_y = float(request['form']['x_y'])
        speed = request['form']['speed']
        direction = request['form']['dir']
        build = GeoFencer.get_area((float(x_x), float(x_y)))
        c.execute("""CREATE TABLE IF NOT EXISTS full_data (time_ timestamp, a_x real, a_y real, v_x real, v_y real, x_x real, y_y real, speed real, direction real, building text);""")
        c.execute('''INSERT into full_data VALUES (?,?,?,?,?,?,?,?,?,?);''', (now, a_x, a_y, v_x, v_y, x_x, x_y, speed, direction, build))
        return "done" 
    
    @withConnCamCursor
    def handle_camera_post(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        now = datetime.now() 
        json_camera = json.loads(request['data']) 
        image_decoded= base64.b64decode(json_camera['fullimg']) 
        c.execute("""CREATE TABLE IF NOT EXISTS cam_data (time_ timestamp, image text);""")
        c.execute('''INSERT into cam_data VALUES (?,?);''', (now, image_decoded))
        filename = '/var/jail/home/team10/build/camera.jpg'  # I assume you have a way of picking unique filenames
        with open(filename, 'wb') as f: 
            f.write(image_decoded) # gets upddated everytime 
        return image_decoded 
    
    # get the image from the cam_data database. Send back the most recent image
    @withConnCamCursor
    def handle_camera_get(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        c.execute("""SELECT * FROM cam_data ORDER BY time_ ASC;""")
        data = c.fetchone()
        
        # NOTE image decoded is already decoded
        # image_decoded = data[1]
        # image_decoded = base64.b64encode(image_decoded)
        # image_decoded = image_decoded.decode('utf-8')
        # cut_prefix = "dataimage/gifbase64"
        # cut_length = len(cut_prefix)
        # image_decoded = image_decoded[cut_length:]
        # add_prefix = "data:image/gif;base64,"
        return """
        <!DOCTYPE html>
        <html>
        <head>
        </head>
        <body>
            <img src="data:image/gif;base64,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UBcbimsKAExTSKQj/9k=2Q==">
        </body>
        </html>
        """
        

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
        if has_value:
            if "camera" in request["values"]:
                return Crud.handle_camera_post(request)
        else:
            return Crud.handle_db_api_post(request)
    if request["method"] == "GET":
        has_value = "values" in request and len(request["values"]) > 0
        camera = "camera" in request["values"]
        if camera:
            return Crud.handle_camera_get(request)
        
        if has_value:
            if "whereami" in request["values"]:
                return Crud.handle_whereami(request)
            elif "wherehaveibeen" in request["values"]:
                return Crud.handle_wherehaveibeen(request)
            elif "monalisa" in request["values"]:
                return Webpage.handle_mona_lisa(request)
            return Webpage.handle_webpage_get(request)
        else:
            return Crud.handle_db_api_get(request)
