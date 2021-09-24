format long g
format compact
clc
close all
clearvars

% get the location of the script file to save figures
full_path = mfilename('fullpath');
[scriptpath,  filename, ext] = fileparts(full_path);

plot_num = 1;
line_width = 1.0;

commandwindow;

%% setup the range of variables

wavelength = 0.525e-6;
k2 = (4*pi()*pi())/(wavelength * wavelength);


L = 600:10:1000;

Cn2 = 1e-16:1e-16:1e-13;

[l, cn] = meshgrid(L, Cn2);

%% calculate the r0 and tg values

V = 0.1;  % m/s

r0 = (0.423*k2.*cn.*L).^(-3/5);

tg = 0.32 * r0/V;

%% plot the data

figure(plot_num)
set(gcf,'position',([50,50,1500,700]),'color','w')

mesh(L, Cn2, tg)

set(gca,'fontweight','bold','FontSize',12);

xlabel('L (m)', 'fontweight','bold','FontSize',12);
ylabel('Cn^2', 'fontweight','bold','FontSize',12);

zlabel('Time (s)', 'fontweight','bold','FontSize',12);

view(-120,30)
