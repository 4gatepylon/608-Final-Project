import sqlite3
from datetime import datetime
import os
import json

from typing import Callable, Dict, List, Tuple, Any

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
    
    def get_area(point_coord: Tuple[int, int]) -> str:
        for name, poly in GeoFencer.locations.items():
            if GeoFencer.within_area(point_coord, poly) % 2 == 1:
                return name
        return "Off Campus"

class Crud(object):
    DB_FILE = "/var/jail/home/team10/database_new.db" 
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

        for time_, a_x, a_y, v_x, v_y, x_x, x_y, speed, direction in data:
            dto = datetime.strptime(time_,"%Y-%m-%d %H:%M:%S.%f")
            times.append(dto.strftime("%m/%d/%Y, %H:%M:%S"))
            a_x_vals.append(a_x)
            a_y_vals.append(a_y)
            v_x_vals.append(v_x)
            v_y_vals.append(v_y)
            x_x_vals.append(x_x)
            x_y_vals.append(x_y)
            speeds.append(speed)
            directions.append(direction)

        result_dict = {"times": times, "a_x": a_x_vals, "a_y": a_y_vals, "v_x": v_x_vals, "v_y": v_y_vals, "x_x": x_x_vals, "x_y": x_y_vals, 'speeds': speeds, 'directions': directions}
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
    def handle_db_api_post(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        now = datetime.now()
        a_x = request['form']['a_x']
        a_y = request['form']['a_y']
        v_x = request['form']['v_x']
        v_y = request['form']['v_y']
        x_x = request['form']['x_x']
        x_y = request['form']['x_y']
        speed = request['form']['speed']
        direction = request['form']['dir'] 
        c.execute("""CREATE TABLE IF NOT EXISTS full_data (time_ timestamp, a_x real, a_y real, v_x real, v_y real, x_x real, y_y real, speed real, direction real);""")
        c.execute('''INSERT into full_data VALUES (?,?,?,?,?,?,?,?,?);''', (now, a_x, a_y, v_x, v_y, x_x, x_y, speed, direction))
        return "done"

class Webpage(object):
    INDEX_FILE = "/var/jail/home/team10/index.html"
    def __init__(self):
        pass
    def handle_webpage_get(request: Any):
        if os.path.exists(Webpage.INDEX_FILE):
            with open(Webpage.INDEX_FILE, "r") as html_file:
                return html_file.read()
        else:
            return "Index not found"

def request_handler(request: Any):
    if request['method'] == 'POST':
        return Crud.handle_db_api_get(request)
    if request["method"] == "GET":
        has_value = "values" in request and len(request["values"]) > 0
        if has_value:
            if "whereami" in request["values"]:
                return Crud.handle_whereami(request)
            return Webpage.handle_webpage_get(request)
        else:
            return Crud.handle_db_api_get(request)
