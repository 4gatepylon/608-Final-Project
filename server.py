import sqlite3
import datetime
from bokeh.plotting import figure,show
import numpy as np
from bokeh.embed import components

#script is meant for local development and experimentation with bokeh
new_db = '/var/jail/home/team10/database.db' 

def request_handler(request):
    now = datetime.datetime.now()
    conn = sqlite3.connect(new_db)
    c = conn.cursor()
    if request['method'] == 'POST':
        x = request['x']
        y = request['y']
        c.execute("""CREATE TABLE IF NOT EXISTS accel_data (time_ timestamp, x real, y real);""")
        c.execute('''INSERT into sensor_data VALUES (?,?,?, ?);''', ( datetime.datetime.now(), x, y))
        conn.commit()
        conn.close()
        return x, y
    if request['method'] == 'GET':

        data = c.execute('''SELECT * FROM accel_data ORDER BY time_ ASC;''').fetchall()

        x = []
        y = []
        times = []

        for time_, x_val, y_val in data:
            dto = datetime.datetime.strptime(time_,'%Y-%m-%d %H:%M:%S.%f')
            times.append(dto)
            x.append(x_val)
            y.append(y_val)

        plot = figure( x_axis_type="datetime") #create a figure called p
        plot.line(times, x, legend_label="x", line_color="orange") #add a line plot of x vs. y arrays
        plot.line(times, y, legend_label="y", line_color="green") #add a line plot of x vs. y arrays
        
        script, div = components(plot)

        return f'''<!DOCTYPE html>
        <html> <script src="https://cdn.bokeh.org/bokeh/release/bokeh-2.4.0.min.js"></script>
            <body>
                {div}
            </body>
            {script}
        </html>
        '''