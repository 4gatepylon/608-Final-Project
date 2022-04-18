import sqlite3
import datetime
from bokeh.plotting import figure,show
import numpy as np
from bokeh.embed import components
import json

#script is meant for local development and experimentation with bokeh
new_db = "/var/jail/home/team10/database.db" 

def request_handler(request):
    now = datetime.datetime.now()
    conn = sqlite3.connect(new_db)
    c = conn.cursor()
    if request['method'] == 'POST':
        a_x = request['form']['a_x']
        a_y = request['form']['a_y']
        v_x = request['form']['v_x']
        v_y = request['form']['v_y']
        x_x = request['form']['x_x']
        x_y = request['form']['x_y']
        speed = request['form']['speed']
        direction = request['form']['direction'] 
        c.execute("""CREATE TABLE IF NOT EXISTS data (time_ timestamp, a_x real, a_y real, v_x real, v_y real, x_x real, y_y real, speed real, direction real);""")
        c.execute('''INSERT into data VALUES (?,?,?,?,?,?,?);''', (now, a_x, a_y, v_x, v_y, x_x, x_y, speed, direction))
        conn.commit()
        conn.close()
        return "done"
    if request["method"] == "GET":

        data = c.execute("""SELECT * FROM data ORDER BY time_ ASC;""").fetchall()

        a_x_vals = []
        a_y_vals = []
        v_x_vals = []
        v_y_vals = []
        x_x_vals = []
        x_y_vals = []
        speeds = []
        directions = []
        times = []

        for time_, a_x, a_y, v_x, v_y, x_x, x_y in data:
            dto = datetime.datetime.strptime(time_,"%Y-%m-%d %H:%M:%S.%f")
            times.append(dto.strftime("%m/%d/%Y, %H:%M:%S"))
            a_x_vals.append(a_x)
            a_y_vals.append(a_x)
            v_x_vals.append(a_x)
            v_y_vals.append(a_x)
            x_x_vals.append(a_x)
            x_y_vals.append(a_x)

        result_dict = {"times": times, "a_x": a_x_vals, "a_y": a_y_vals, "v_x": v_x_vals, "v_y": v_y_vals, "x_x": x_x_vals, "x_y": x_y_vals, 'speeds': speeds, 'directions': directions}
        return json.dumps(result_dict)
