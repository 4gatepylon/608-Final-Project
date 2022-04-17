import sqlite3
import datetime

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

        return f'''<!DOCTYPE html>
        <html> 
        </html>
        '''

        