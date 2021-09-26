
import os
import math

import numpy as np
import bokeh
from bokeh.models import ColumnDataSource, Spinner, Range1d, Slider, Legend, CustomJS, HoverTool
from bokeh.plotting import figure, show
from bokeh.layouts import column, row, Spacer

spin_width = 100

Cn2_min = 1e-16
Cn2_max = 1e-13
Cn2_step = 1e-16

range_min = 600
range_max = 1000
range_step = 10

tooltips = [("x", "$x"), ("y", "$y"), ("time", "@tg{0.0000}")]
wind_speed = Spinner(title="Wind Speed (m/s)", low=0.0, high=20.0, step=0.01, value=0.01, width=spin_width)
min_x_spin = Spinner(title="Min Range", low=range_min, high=range_max, step=5, value=range_min, width=spin_width)
max_x_spin = Spinner(title="Max Range", low=range_min, high=range_max, step=5, value=range_max, width=spin_width)
min_y_spin = Spinner(title="Min Cn2", low=Cn2_min, high=Cn2_max, step=Cn2_step, value=Cn2_min, width=spin_width)
max_y_spin = Spinner(title="Max Cn2", low=Cn2_min, high=Cn2_max, step=Cn2_step, value=Cn2_max, width=spin_width)

# -----------------------------------------------------------------------------
freq_plot = figure(plot_height=800, plot_width=1200, title="Greenwood Time Constant Surface", tooltips=tooltips)

freq_plot.x_range.range_padding = 0
freq_plot.y_range.range_padding = 0
freq_plot.x_range = Range1d(start=range_min, end=range_max)
freq_plot.y_range = Range1d(start=Cn2_min, end=Cn2_max)

freq_plot.title.align = "center"
freq_plot.title.text_font_size = "16px"
freq_plot.title.text_font_style = "bold"

freq_plot.xaxis.axis_label = "Range (m)"
freq_plot.xaxis.axis_label_text_font_size = "14px"
freq_plot.xaxis.axis_label_text_font_style = "bold"
freq_plot.xaxis.major_label_text_font_size = "14px"
freq_plot.xaxis.major_label_text_font_style = "bold"

freq_plot.yaxis.axis_label = "Cn2 (m^-3/5)"
freq_plot.yaxis.axis_label_text_font_size = "14px"
freq_plot.yaxis.axis_label_text_font_style = "bold"
freq_plot.yaxis.major_label_text_font_size = "14px"
freq_plot.yaxis.major_label_text_font_style = "bold"

# -----------------------------------------------------------------------------
# setup the range of variables
wavelength = 0.525e-6
k2 = ((2*np.pi)/wavelength)**2

# distances
range = np.linspace(range_min, range_max, num=41)

# range of Cn2 values
Cn2 = np.linspace(Cn2_min, Cn2_max, num=1000)

# create the mesh
r, cn = np.meshgrid(range, Cn2)

# -----------------------------------------------------------------------------
# calculate the r0 and tg values
r0 = (0.423*k2*cn*r)**(-3/5)
tg = 0.32*r0/wind_speed.value

source = ColumnDataSource(data=dict(tg=[tg]))

# -----------------------------------------------------------------------------
# setup the javascript callback code
cb_dict = dict(source=source, r0=r0, wind_speed=wind_speed, freq_plot=freq_plot,
               min_x_spin=min_x_spin, max_x_spin=max_x_spin, min_y_spin=min_y_spin, max_y_spin=max_y_spin)

update_plot_callback = CustomJS(args=cb_dict, code="""

    var data = source.data;
    var v = wind_speed.value;
    
    data['tg'] = [];

    var x = new Array(1000);

    for(var idx = 0; idx<x.length; idx++)
    {
        x[idx] = new Array(41);
        for(var jdx = 0; jdx<41; jdx++)
        {
            x[idx][jdx] = 0.32*r0[idx][jdx]/v;
        }
    }
    
    data['tg'].push(x);
    
    freq_plot.x_range.start = min_x_spin.value;
    freq_plot.y_range.start = min_y_spin.value   
    freq_plot.x_range.end = max_x_spin.value;
    freq_plot.y_range.end = max_y_spin.value;
    
    source.change.emit();
    
""")

# -----------------------------------------------------------------------------
# create the plot
freq_plot.image(image='tg', x=range_min, y=Cn2_min, dw=(range_max-range_min), dh=(Cn2_max-Cn2_min), palette="Viridis256", level="image", source=source)
freq_plot.grid.grid_line_width = 0.0

# -----------------------------------------------------------------------------
# assign the call backs
for w in [wind_speed, min_x_spin, max_x_spin, min_y_spin, max_y_spin]:
    w.js_on_change('value', update_plot_callback)

# -----------------------------------------------------------------------------
# create the layout for the controls
range_input = column(min_x_spin, max_x_spin)
cn2_input = column(min_y_spin, max_y_spin)

inputs = column(wind_speed, range_input, cn2_input)
layout = column(row(inputs, Spacer(width=20, height=20), freq_plot))

show(layout)

bp = 1

