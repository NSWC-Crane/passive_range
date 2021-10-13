format long g
format compact
clc
close all
clearvars

% get the location of the script file to save figures
full_path = mfilename('fullpath');
[scriptpath,  filename, ext] = fileparts(full_path);
plot_count = 1;
line_width = 1.0;

commandwindow;

%% select the folder where the images are located
img_w = 64;
img_h = 64;

num = 20;

inc = 5;

image_path = uigetdir(strcat(scriptpath,'../../../../'), 'Select Folder with Images');

if(image_path == 0)
    return;
end

image_listing = dir(strcat(image_path,'\*.png'));

listing = dir(image_path);
listing = listing(3:end);

% img = imread(fullfile(image_listing(1).folder, image_listing(1).name));
% image(img);
% [pt_x, pt_y] = ginput(1);

%% start processing

fft_sum = cell(numel(listing), 1);
max_fft_sum = cell(numel(listing), 1);
focus_vals = cell(numel(listing), 1);
sharpest_img = cell(numel(listing), 1);
% shapest_index = cell(numel(listing), 1);
img_name = cell(numel(listing), 1);

mx = [-1, 0, 1; -2, 0 ,2; -1, 0, 1];
my = [1, 2, 1; 0, 0, 0; -1, -2, -1];

for kdx=1:numel(listing)

    if(~isfolder(fullfile(listing(kdx).folder,listing(kdx).name)))
        continue;
    end
    
    image_listing = dir(strcat(fullfile(listing(kdx).folder,listing(kdx).name), '\*.png'));
    img = cell(ceil(numel(image_listing)/num), 1);
    img_idx = 1;
%     img_name = cell(ceil(numel(image_listing)/num), 1);
    
    
    for idx=1:num:numel(image_listing)
%         img_name{img_idx} = image_listing(idx).name;
        fft_sum2 = zeros(num, 1);
        tmp_img = double(rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx).name))));

        [z, f, e, n] = parse_image_filename(image_listing(idx).name);

        tl = floor(size(tmp_img)/2 - [img_h, img_w]/2);
        img{img_idx} = tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1);
        fft_sum2(1) = sum(sum(abs(fft2(img{img_idx})/(img_w * img_h))));

        for jdx=1:num-1
            tmp_img = double(rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx+jdx).name))));
            tl = floor(size(tmp_img)/2 - [img_h, img_w]/2);
            fft_sum2(jdx+1) = sum(sum(abs(fft2(tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1))/(img_w * img_h))));
            img{img_idx} = img{img_idx} + tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1);                                
        end

        img{img_idx} = img{img_idx}/num;
        fft_img = fft2(img{img_idx})/(img_w * img_h);
        fft_img(1,1) = 0;
        
        max_fft_sum{kdx}(end+1) = mean(fft_sum2);
        
        conv_img = abs(conv2(img{img_idx}, mx(end:-1:1, end:-1:1), 'valid') + conv2(img{img_idx}, my(end:-1:1, end:-1:1), 'valid'));
                
        figure(1)
%         image(uint8(img{img_idx}))
%         imagesc(fftshift(real(fft_img)));
        imagesc(conv_img);
        colormap(jet(256));

%         fft_sum{kdx}(end+1) = sum(abs(fft_img(:)));
        fft_sum{kdx}(end+1) = mean(fft_sum2);
        
%         fft_sum{kdx}(end+1) = sum((conv_img(:)));
        
        
        focus_vals{kdx}(end+1) = ceil(f/inc)*inc; %floor(f/5+0.5)*5;
        img_idx = img_idx + 1;
    end
    
    [~, shapest_index] = max(fft_sum{kdx});
    
    sharpest_img{kdx} = img{shapest_index};
    img_name{kdx} = image_listing((shapest_index-1)*num+1).name;
    
    figure(10)
    set(gcf,'position',([50,50,1400,500]),'color','w')
    box on
    grid on
    hold on
%     stem(focus_vals{kdx}, fft_sum{kdx}, 'Linewidth', line_width)
%     stem(focus_vals{kdx}(shapest_index), fft_sum{kdx}(shapest_index), 'r', 'Linewidth', line_width)
    stairs(focus_vals{kdx}, fft_sum{kdx}, 'Linewidth', line_width)
    stem(focus_vals{kdx}, max_fft_sum{kdx}, 'b', 'Linewidth', line_width)
    stem(focus_vals{kdx}(shapest_index), fft_sum{kdx}(shapest_index), 'r', 'Linewidth', line_width)
    set(gca,'fontweight','bold','FontSize',12);

    xlim([focus_vals{kdx}(1), focus_vals{kdx}(end)])
    %xticks(focus_vals{kdx}(1:5:end));
    xticklabels(num2str(focus_vals{kdx}'));
    xtickangle(45);    
    ax = gca;
    ax.Position = [0.05 0.15 0.92 0.81];
    
    pause(0.1);
end

%% remove the empty cells

index = [];
for idx=1:numel(fft_sum)
    
    if(~isempty(fft_sum{idx}))
       index = cat(1, index, idx);
    end
end

fft_sum = {fft_sum{index}}';
focus_vals = {focus_vals{index}}';
sharpest_img = {sharpest_img{index}}';
img_name = {img_name{index}}';

bp = 1;

%% show the sharpest images and save them

ranges = zeros(numel(fft_sum), 1);

for idx=1:numel(fft_sum)

    figure(idx+20)
    set(gcf,'position',([50,50,600,500]),'color','w')
    image(uint8(sharpest_img{idx}))
    colormap(gray(256));    

    fprintf('%s, %s\n', listing(idx).name, img_name{idx});
    
    [z, f, e, n] = parse_image_filename(img_name{idx});
    
    ranges(idx) = str2double(listing(idx).name);
    
    file_name = strcat('image_s', num2str(img_w, '%03d'), '_r', listing(idx).name, '_z', num2str(z, '%04d'), '_f', num2str(f, '%05d'), '_combined.png');    
    imwrite(uint8(sharpest_img{idx}), fullfile(listing(idx).folder, file_name));

end

%% create a combined matrix

% get the min and max focus value
fv = [focus_vals{:}];

min_fv = min(fv);
max_fv = max(fv);

focus_idx = min_fv:inc:max_fv;

sh_data = zeros(numel(ranges), numel(focus_idx));

for idx=1:numel(ranges)
    
    for jdx=1:numel(focus_idx)
        
        for kdx=1:numel(focus_vals{idx})
            
            if(focus_vals{idx}(kdx) == focus_idx(jdx))
                
                sh_data(idx, jdx) = fft_sum{idx}(kdx);
                break;
            end
            
        end
        
    end
    
end


