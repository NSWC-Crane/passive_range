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
img_w = 128;
img_h = 128;

num = 20;

image_path = uigetdir(scriptpath, 'Select Folder with Images');
image_listing = dir(strcat(image_path,'\*.png'));

listing = dir(image_path);
listing = listing(3:end);

% img = imread(fullfile(image_listing(1).folder, image_listing(1).name));
% image(img);
% 
% [pt_x, pt_y] = ginput(1);

%% start processing

fft_sum = cell(numel(listing), 1);

for kdx=1:numel(listing)

    image_listing = dir(strcat(fullfile(listing(kdx).folder,listing(kdx).name), '\*.png'));

    for idx=1:num:numel(image_listing)
        tmp_img = double(rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx).name))));

        tl = floor(size(tmp_img)/2 - [img_h, img_w]/2);
        img = tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1);

        for jdx=1:num-1
            tmp_img = double(rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx+jdx).name))));
            tl = floor(size(tmp_img)/2 - [img_h, img_w]/2);
            img = img + tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1);                                
        end

        img = img/num;
        figure(1)
        image(uint8(img))
        colormap(gray(256));

        fft_img = fft2(img)/(img_w * img_h);

        fft_sum{kdx}(end+1) = sum(abs(fft_img(:)));

    end

end


% [max_val, index] = find_sharpest_image(image_listing, floor(pt_x), floor(pt_y), img_w, img_h);
% 
% img = imread(fullfile(image_listing(1).folder, image_listing(index).name));

%% 
image(img);
axis off
