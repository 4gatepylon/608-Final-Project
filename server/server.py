import sqlite3
from datetime import datetime
import os
import json

from enum import Enum

from typing import Callable, Dict, List, Tuple, Any

from black import Timestamp

# Utility
def unzip(lst: List[Tuple]) -> Tuple:
    return zip(*lst)

class MITBuilding(Enum):
    Student_Center = 0
    Dorm_Row = 1
    Simmons_Briggs = 2
    Boston_FSILG_West = 3
    Boston_FSILG_East = 4
    Stata_North_Court = 5
    East_Campus = 6
    Vassar_Academic_Buildings = 7
    Infinite_Corridor_Killian = 8
    Kendall_Square = 9
    Sloan_Media_Lab = 10
    North_Campus = 11
    Technology_Square = 12

    def name(self) -> str:
        # We have to keep this here to avoid it becoming an enum value
        return {
            0: "Student Center",
            1: "Dorm Row",
            2: "Simmons/Briggs",
            3: "Boston FSILG (West)",
            4: "Boston FSILG (East)",
            5: "Stata/North Court",
            6: "East Campus",
            7: "Vassar Academic Buildings",
            8: "Infinite Corridor/Killian",
            9: "Kendall Square",
            10: "Sloan/Media Lab",
            11: "North Campus",
            12: "Technology Square",
        }[self.value]

    def asInt(self) -> int:
        return self.value
    
    def __hash__(self) -> Any:
        return hash(self.value)

    def __str__(self) -> str:
        return self.name()

class GeoFencer(object):
    MIT_BUILDINGS = {
        MITBuilding.Student_Center: [
            (-71.095863,42.357307),
            (-71.097730,42.359075),
            (-71.095102,42.360295),
            (-71.093900,42.359340),
            (-71.093289,42.358306),
        ],
        MITBuilding.Dorm_Row: [
            (-71.093117,42.358147),
            (-71.092559,42.357069),
            (-71.102987,42.353866),
            (-71.106292,42.353517),
        ],
        MITBuilding.Simmons_Briggs: [
            (-71.097859,42.359035),
            (-71.095928,42.357243),
            (-71.106356,42.353580),
            (-71.108159,42.354468),
        ],
        MITBuilding.Boston_FSILG_West: [
            (-71.124664,42.353342),
            (-71.125737,42.344906),
            (-71.092478,42.348014),
            (-71.092607,42.350266),
        ],
        MITBuilding.Boston_FSILG_East: [
            (-71.092409,42.351392),
            (-71.090842,42.343589),
            (-71.080478,42.350900),
            (-71.081766,42.353771),
        ],
        MITBuilding.Stata_North_Court: [
            (-71.091636,42.361802),
            (-71.090950,42.360811),
            (-71.088353,42.361112),
            (-71.088267,42.362476),
            (-71.089769,42.362618),
        ],
        MITBuilding.East_Campus: [
            (-71.089426,42.358306),
            (-71.090885,42.360716),
            (-71.088310,42.361017),
            (-71.087130,42.359162),
        ],
        MITBuilding.Vassar_Academic_Buildings: [
            (-71.094973,42.360359),
            (-71.091776,42.361770),
            (-71.090928,42.360636),
            (-71.094040,42.359574),
        ],
        MITBuilding.Infinite_Corridor_Killian: [
            (-71.093932,42.359542),
            (-71.092259,42.357180),
            (-71.089619,42.358274),
            (-71.090928,42.360541),
        ],
        MITBuilding.Kendall_Square: [
            (-71.088117,42.364188),
            (-71.088225,42.361112),
            (-71.082774,42.362032),
        ],
        MITBuilding.Sloan_Media_Lab: [
            (-71.088203,42.361017),
            (-71.087044,42.359178),
            (-71.080071,42.361619),
            (-71.082796,42.361905),
        ],
        MITBuilding.North_Campus: [
            (-71.11022,42.355325),
            (-71.101280,42.363934),
            (-71.089950,42.362666),
            (-71.108361,42.354484),],
        MITBuilding.Technology_Square: [
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
        for loc, poly in GeoFencer.MIT_BUILDINGS.items():
            if GeoFencer.within_area(point_coord, poly) % 2 == 1:
                return loc.name()
        return "Off Campus"

class Crud(object):
    DB_FILE = "/var/jail/home/team10/database_new.db" 
    DB_FORMAT = [
        # Tells us (form key, database key, type in SQL, multiple name)
        # It is a list of tuples to enforce order
        # for insertions and reads from the database
        (None, "time_", "timestamp", "times"),
        ("a_x", "tilt_x", "real", "x_tilts")
        ("a_y", "tilt_y", "real", "y_tilts")
        ("lon", "longitude", "real", "latitudes"),
        ("lat", "latitude", "real", "longitudes"),
        ("speed", "speed", "real", "speeds"),
        ("dir", "direction", "real", "directions"),
        (None, "building", "int", "buildings"),
    ]

    def __init__(self):
        pass
        
    # Helper so that we can use exclusively DB Format
    def read_form(form):
        search_for = filter(lambda fmt: not fmt[0] is None, Crud.DB_FORMAT)
        generate_for = filter(lambda fmt: fmt[0] is None, Crud.DB_FORMAT)
        gotten = {}
        # Get from the form all the values we need
        for form_key, db_key, db_type, _ in search_for:
            if not form_key in form:
                raise Exception(f"Did not find {form_key} in form")
            value = form[form_key]
            if db_type == "timestamp":
                value = datetime.datetime.strptime(value, "%Y-%m-%d %H:%M:%S")
            gotten[db_key] = value
        # We have to run these second to have already gotten the lat/lon
        for _, db_key, _ in generate_for:
            if db_key == "time_":
                gotten[db_key] = datetime.now()
            elif db_key == "building":
                gotten[db_key] = GeoFencer().get_area((gotten["longitude"], gotten["latitude"]))
            else:
                raise Exception(f"Found unknown db key {db_key}")
        
        insert_tuple = [gotten[key] for _, key, _ in Crud.DB_FORMAT]
        return insert_tuple

    # Here are various SQL utility methods
    def create_table_command() -> str:
        defs = [f"{name} {type_}" for _, name, type_ in Crud.DB_FORMAT]
        defs_str = ", ".join(defs)
        return f"CREATE TABLE IF NOT EXISTS full_data ({defs_str});"
    
    def insert_command() -> str:
        q_str = ",".join(["?" for _ in range(len(Crud.DB_FORMAT))])
        return f"INSERT into full_data VALUES ({q_str});"
    
    def select_star_time_asc_command() -> str:
        return "SELECT * FROM full_data ORDER BY time_ ASC;"
    

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
        data = c.execute(Crud.select_star_time_asc_command()).fetchall()
        lists = unzip(data)
        dictionary = {multi_name : lst for (_, _, _, multi_name), lst in zip(Crud.DB_FORMAT, lists)}

        # Change the formats to be interpretable by the client
        for (_, db_name, _type, multi_name), (name, value) in zip(Crud.DB_FORMAT, dictionary.items()):
            if _type == "timestamp":
                dictionary[multi_name] = list(map(
                    lambda t: datetime.strptime(t,"%Y-%m-%d %H:%M:%S.%f").strftime("%m/%d/%Y, %H:%M:%S"), 
                    value,
                ))
            elif db_name == "building":
                dictionary[multi_name] = list(map(
                    lambda b: b.name(),
                    value,
                ))
            
        return json.dumps(dictionary)
    
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
        # TODO read the table and then tell you where you've been all this time
        return "Not Implemented"

    @withConnCursor
    def handle_db_api_post(c: sqlite3.Cursor, conn: sqlite3.Connection, request: Any) -> str:
        c.execute(Crud.create_table_command())
        c.execute(Crud.insert_command(), Crud.read_form(request["values"]))
        return "done"

class Webpage(object):
    INDEX_FILE = "/var/jail/home/team10/index.html"
    def __init__(self):
        pass

    def handle_locations_page(request: Any) -> str:
        # TODO
        return "Not Yet Implemented"

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
