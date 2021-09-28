format long g
format compact
clc
close all
clearvars

% get the location of the script file to save figures
full_path = mfilename('fullpath');
[scriptpath,  filename, ext] = fileparts(full_path);
plot_count = 1;

commandwindow;

%% select the folder where the images are located

image_path = uigetdir(scriptpath, 'Select Folder with Images');

image_listing = dir(strcat(image_path,'\*.png'));
img = imread(fullfile(image_listing(1).folder, image_listing(1).name));
image(img);
[pt_x, pt_y] = ginput(1);

[max_val, index] = find_sharpest_image(image_listing, floor(pt_x), floor(pt_y), 64, 64);

img = imread(fullfile(image_listing(1).folder, image_listing(index).name));

%% 
image(img);
axis off
