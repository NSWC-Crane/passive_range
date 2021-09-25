
import platform
import os
import math


import numpy as np
import cv2 as cv
import bokeh
from bokeh.io import curdoc
from bokeh.models import ColumnDataSource
from bokeh.plotting import figure, show
from bokeh.layouts import column, row, Spacer


spin_width = 110

wind_speed = Spinner(title="wind speed (m/s)", low=0.0, high=10.0, step=0.1, value=1.0, width=spin_width)

freq_plot = figure(plot_height=550, plot_width=1300, title="Frequency Surface")



## setup the range of variables
wavelength = 0.525e-6
k2 = ((2*np.pi)/wavelength)**2

# distances
L = np.linspace(600, 1000, num=41)

# range of Cn2 values
Cn2 = np.linspace(1e-16, 1e-13, num=1000)

# create the mesh
l, cn = np.meshgrid(L, Cn2)

## calculate the r0 and tg values

V = 0.1  # m/s

r0 = (0.423*k2*cn*L)**(-3/5);

tg = 0.32 * r0/V




