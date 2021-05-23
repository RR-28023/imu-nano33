from vpython import *
from influxdb import InfluxDBClient
import time
import math

USE_GYROSCOPE_DATA = False # If False it will use accelerometer data

# Connect to InfluxDB
client = InfluxDBClient('[INFLUX SERVER]', 8086, username='[INFLUX USRNM]', password='[INFLUX PW]', database='[DATABASE NAME]')

# Draw the Nano board volume
left=box(pos=vector(-5,0,0), length=.1, width=10, height=10, color=color.white, opacity=.2)
bottom=box(pos=vector(0,-5,0), length=10, width=10, height=.1, color=color.white, opacity=.2)
back=box(pos=vector(0,0,-5), length=10, width=0.1, height=10, color=color.white, opacity=.2)
main_board=box(pos=vector(0,0,0),axis=vector(0,0,0),length=6, height=0.5, width=2)
sd=box(pos=vector(-2,0.3,0),axis=vector(0,0,0),length=1.0, height=0.1, width=1.5, color=color.red)
board = compound([main_board, sd], pos=vector(0,0,0))


def cr(x):
    # Custom rounding for accelerometer data
    prec = 0.1
    x = round(x / prec, 0) * prec
    return x

def cr2(x):
    # Custom rounding for gyroscope data
    prec = 0.05
    x = round(x / prec, 0) * prec
    return x

start = time.time()
roll, pitch, yaw = 0, 0, 0

while True:
    try:
        last_measure = list(client.query("SELECT * FROM imu ORDER BY desc LIMIT 1"))[0][0]

        if USE_GYROSCOPE_DATA:
            x_gyr, y_gyr, z_gyr = last_measure['x_gyr'], last_measure['y_gyr'], last_measure['z_gyr']
            secs_elapsed = time.time() - start
            start = time.time()
            roll = secs_elapsed * (x_gyr - 2.55)  # Observed error
            pitch = secs_elapsed * (y_gyr + 5.1)  # Observed error
            yaw = secs_elapsed * (z_gyr + 3.1)  # Observed error
            board.rotate(angle=cr2(roll * math.pi / 180.0), axis=vector(1, 0, 0))
            board.rotate(angle=cr2(pitch * math.pi / 180.0), axis=vector(0,0,-1))
            board.rotate(angle=cr2(yaw * math.pi / 180.0), axis=vector(0, 1, 0))

        else:
            x_acc, y_acc, z_acc = cr(last_measure['x_acc']), cr(last_measure['y_acc']), cr(last_measure['z_acc'])
            up = vector(-x_acc, z_acc, y_acc)
            board.up = vector(-x_acc, z_acc, y_acc)
            if x_acc == 0.0 and y_acc == 0.0 and z_acc == 1.0:
                print("back")
                board.axis = vector(1,0,0)

        time.sleep(0.1)

    except:
        continue

