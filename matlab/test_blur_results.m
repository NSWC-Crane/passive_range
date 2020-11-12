format long g
format compact
clc
close all
clearvars

% get the location of the script file to save figures
full_path = mfilename('fullpath');
[startpath,  filename, ext] = fileparts(full_path);
plot_num = 1;


%% get the directory for the images

img_path = uigetdir(startpath, 'Select Folder with Images');
image_ext = '*.png';

if(img_path == 0)
    return;
end

listing = dir(strcat(img_path, '/', image_ext));

save_dir = img_path;

%% run through each image in the list
commandwindow;

% this defines the expected maximum blur radius
max_blur_radius = 180;

sk = create_1D_gauss_kernel(3, 2.0);

mf = [1 1 1];

dx = [-0.5 0 0.5];

fx = [1 -2 1];

% this is a value to provide a small buffer to exceed before being counted
offset = 1/255;
    
fprintf('File Name\t\t\t\t# of Pixels Blurred\n')
fprintf('-----------------------------------------------------\n')
for idx=1:numel(listing)

    % load in an image and get its size
    img_file = fullfile(listing(idx).folder, listing(idx).name);
    img = imread(img_file);
    [img_h, img_w, img_c] = size(img);
    
    if(img_c > 1)
        img = double(rgb2gray(img));
    else
        img = double(img);
    end

    % find the rough center points assuming that the knife edge is right to left
    img_s = img(floor(img_h/2-5:img_h/2+5),:);
    img_line = mean(img_s, 1);
    img_cw = floor(img_w/2);
    width_range = max(2,img_cw-max_blur_radius):1:min(img_w-1,img_cw+max_blur_radius);
    
    % just use a single line to determine the blur amount
    img_line = conv(img_line, sk, 'same');
    img_line = img_line(width_range);
    
    % get the direction of the line high->low = -1 / low->high = 1
    [min_line, min_idx] = min(img_line);
    [max_line, max_idx] = max(img_line);
    if(max_idx < min_idx)
        direction = -1;
    else
        direction = 1;
    end
    
    % find the areas where limits are met
    mid_limit = (floor(max(img_line(:))) + ceil(min(img_line(:))))/2;
    %[~, mid_idx] = min(abs(img_line - mid_limit));   
    
    if(direction == 1)
        
        mid_idx = find(img_line<mid_limit,1,'last');
        
        %split the ranges in the middle
        x2 = 1:mid_idx;
        x1 = (mid_idx+1):numel(img_line);
    %     m1 = median(img_line(x1));
    %     m2 = median(img_line(x2));


        k1 = unique(convhull(x1, img_line(x1)));
        k2 = unique(convhull(x2, img_line(x2)));

        k1 = k1(end:-1:1);
        k2 = k2(end:-1:1);
        k_line = cat(1, k2(1:end-1), k1);
    else
        mid_idx = find(img_line<mid_limit,1,'first');

        x1 = 1:mid_idx;
        x2 = (mid_idx+1):numel(img_line);
    %     m1 = median(img_line(x1));
    %     m2 = median(img_line(x2));


        k1 = unique(convhull(x1, img_line(x1)));
        k2 = unique(convhull(x2, img_line(x2)));
        
        k_line = cat(1, k1(1:end-1), k2);
    end
    
    
%     low_limit = floor(mean(img_line(x2))) + offset;
%     high_limit = ceil(mean(img_line(x1))) - offset; 
    
    
    % find the lower limit
    count = 0;
    low_limit = floor(img_line(mid_idx));
    while((low_limit>min_line) && (count < 4))

        r = (img_line(x2) >= low_limit-0.25) & (img_line(x2) <= low_limit+0.25);
        count = sum(r);
        
        low_limit = low_limit - 0.25;
    end
    low_limit = low_limit + 0.25;
    
    % find the high limit
    count = 0;
    high_limit = floor(img_line(mid_idx));
    while((high_limit < max_line) && (count < 4))

        r = (img_line(x1) >= high_limit-0.25) & (img_line(x1) <= high_limit+0.25);
        count = sum(r);
        
        high_limit = high_limit + 0.25;
    end
    high_limit = high_limit - 0.25;
    
    
%     low_limit = median(img_line(:) + 1 + offset;
%     high_limit = floor(max(img_line(:))) - 1 - offset;    
    
    % find the point where the slope is changing
    % we assume that there enough distance between the min and max along
    % with noise
    %index = find(

    
    
    
    % get the laplacian to see where the inflection point is located
%     lap = conv(img_line, fx,'same');
%     lap = lap(2:end-1);
%     [min_lap, min_idx] = min(lap);
%     [max_lap, max_idx] = max(lap);
%     
%     dx1 = diff(img_line, 1);
%     dx2 = diff(img_line, 2);

    
%     match = (img_line > low_limit) == (img_line < high_limit));
%     %match = conv(match, mf, 'same');
%     %match = bwareafilt(match,20);
%     
%     % this is where we find the group of matches that contains mid_idx
%     measurements = regionprops(logical(match), 'Extrema');
%     
%     for jdx=1:numel(measurements)
%         
%         min_ex = min(measurements(jdx).Extrema(:,1));
%         max_ex = max(measurements(jdx).Extrema(:,1));
%         
%         if((mid_idx >= min_ex) && (mid_idx <= max_ex))
%             num = max_ex - min_ex;
%             continue;
%         end
%             
%     end
    
    [match, num, min_ex, max_ex] = find_match(img_line, low_limit, high_limit, mid_idx);
    %num = sum(match);
    
    fprintf('%03d: %s, \t%02d\n', (idx-1), listing(idx).name, num-2);

    figure(1)
    plot(img_line, '.-b');
    hold on;
    plot(match*max(img_line(:)), 'r');
    %plot(img_line2, 'g');
    plot(low_limit*ones(size(img_line)), 'g');
    plot(high_limit*ones(size(img_line)), 'g');
    hold off;

end

fprintf('-----------------------------------------------------\n')


%% matching function

function [match, num, min_ex, max_ex] = find_match(data, low_limit, high_limit, mid_idx)

    match = (data > low_limit) == (data < high_limit);
    
    % this is where we find the group of matches that contains mid_idx
    measurements = regionprops(logical(match), 'Extrema');
    
    for jdx=1:numel(measurements)
        
        min_ex = floor(min(measurements(jdx).Extrema(:,1)));
        max_ex = ceil(max(measurements(jdx).Extrema(:,1)));
        
        if((mid_idx >= min_ex) && (mid_idx <= max_ex))
            num = max_ex - min_ex;
            break;
        end
            
    end
    
    match = zeros(size(match));
    match(min_ex:max_ex) = 1;

end
