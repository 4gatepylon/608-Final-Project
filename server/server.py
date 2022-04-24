import sqlite3
from datetime import datetime
import os
import json

class Crud(object):
    DB_FILE = "/var/jail/home/team10/database_new.db" 
    def __init__(self):
        pass

    # Unclear why you can't @staticmethod these
    # (I don't know what it does technically)
    # Check out
    # https://stackoverflow.com/questions/41921255/staticmethod-object-is-not-callable
    # (don't hae the time and it's not that important)

    def withConnCursor(func):
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
    def handle_db_api_get(c, conn, request):
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
    def handle_db_api_post(c, conn, request):
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
    def handle_webpage_get(request):
        if os.path.exists(Webpage.INDEX_FILE):
            with open(Webpage.INDEX_FILE, "r") as html_file:
                return html_file.read()
        else:
            return "Index not found"

def request_handler(request):
    if request['method'] == 'POST':
        return Crud.handle_db_api_get(request)
    if request["method"] == "GET":
        has_value = "values" in request and len(request["values"]) > 0
        if has_value:
            return Webpage.handle_webpage_get(request)
        else:
            return Crud.handle_db_api_get(request)
