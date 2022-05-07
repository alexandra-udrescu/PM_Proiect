import matplotlib.pyplot as plt
import matplotlib.animation as animation

import serial

# Used code from
# https://pythonprogramming.net/live-graphs-matplotlib-tutorial/
# https://matplotlib.org/3.5.0/gallery/subplots_axes_and_figures/subplots_demo.html
# http://www.mikeburdis.com/wp/notes/plotting-serial-port-data-using-python-and-matplotlib/

fig, axs = plt.subplots(2, 2)

for ax in axs.flat:
    ax.set(xlabel='', ylabel='')
    ax.axes.xaxis.set_visible(False)

arduino = serial.Serial(port='COM5', baudrate=9600)

arduino.readline()  # ignore header

# Initialize arrays with first read
lin = ''
length = 100
x_axis = list(range(0, length))
while not lin.startswith('>'):
    lin = arduino.readline().decode()
lin = lin.replace(' ', '')
lin = lin.replace('>', '')
lin = lin.replace('\t', '')
inp = lin.split('|')
mq2 = [float(inp[0])] * length
mq135 = [float(inp[1])] * length
temp = [float(inp[2])] * length
sharp = [float(inp[4])] * length


def animate(frame):
    line = ''
    while not line.startswith('>'):
        line = arduino.readline().decode()
    line = line.replace(' ', '')
    line = line.replace('>', '')
    line = line.replace('\t', '')
    inputs = line.split('|')
    mq2.pop(0)
    mq2.append(float(inputs[0]))
    mq135.pop(0)
    mq135.append(float(inputs[1]))
    temp.pop(0)
    temp.append(float(inputs[2]))
    sharp.pop(0)
    sharp.append(float(inputs[4]))

    axs[0, 0].clear()
    axs[0, 1].clear()
    axs[1, 0].clear()
    axs[1, 1].clear()

    axs[0, 0].plot(x_axis, mq2)
    axs[0, 0].set_title('MQ2 Voltage [V]')

    axs[0, 1].plot(x_axis, mq135, 'tab:orange')
    axs[0, 1].set_title('MQ135 Voltage [V]')

    axs[1, 0].plot(x_axis, sharp, 'tab:green')
    axs[1, 0].set_title('Sharp Voltage [V]')

    axs[1, 1].plot(x_axis, temp, 'tab:red')
    axs[1, 1].set_title('Temperature [*C]')


ani = animation.FuncAnimation(fig, animate, interval=2000)
plt.show()
