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
img_w = 128;
img_h = 128;

img_start = 0;
img_stop = 19;
num = img_stop - img_start + 1;

inc = 5;

image_path = uigetdir(strcat(scriptpath,'../../../../'), 'Select Folder with Ranges');

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
close all

fft_sum = cell(numel(listing), 1);
max_fft_sum = cell(numel(listing), 1);
focus_vals = cell(numel(listing), 1);
sharpest_img = cell(numel(listing), 1);
% shapest_index = cell(numel(listing), 1);
img_name = cell(numel(listing), 1);

ranges = zeros(numel(listing), 1);

mx = [-1, 0, 1; -2, 0 ,2; -1, 0, 1];
my = [1, 2, 1; 0, 0, 0; -1, -2, -1];

listing_count = 0;

for idx=1:numel(listing)

    if(~isfolder(fullfile(listing(idx).folder,listing(idx).name)))
        continue;
    end
    
    listing_count = listing_count + 1;
    
    ranges(idx) = str2double(listing(idx).name);
    
    image_listing = dir(strcat(fullfile(listing(idx).folder, listing(idx).name), '\*', num2str(img_start, '%02d'),'.png'));
%     img = cell(ceil(numel(image_listing)/num), 1);
%     img_idx = 1;
%     img_name = cell(ceil(numel(image_listing)/num), 1);

    img = cell(numel(image_listing),1);
    
    conv_stats = zeros(numel(image_listing),3);
%     conv_mean = zeros(numel(image_listing),1);
    conv_sum = zeros(numel(image_listing),num+1);
    
    fft_stats = zeros(numel(image_listing),3);
%     fft_mean = zeros(numel(image_listing),1);
    fft_sum = zeros(numel(image_listing),num+1);
    
    
    % cycle through the image listings
    for jdx=1:numel(image_listing)
        
        % get the parameters of the image to recreate the file name: typical format - image_z01998_f45902_e14987_i00.png
        [z, f, e, n] = parse_image_filename(image_listing(jdx).name);
        base_file_name = strcat('image_z', num2str(z, '%05d'), '_f', num2str(f, '%05d'), '_e', num2str(e, '%05d'), '_i'); 
        
        if(jdx == 1)
%             fn = floor(f/inc + 0.5)*inc;
            fn = ceil(f/inc)*inc;
            focus_vals{idx} = fn:inc:(fn + inc*(numel(image_listing)-1));
        end
        
        imgs = [];
        
        % read in the group of num images
        for kdx=img_start:img_stop
            tmp_img = imread( fullfile(image_listing(idx).folder, strcat(base_file_name, num2str(kdx, '%02d.png'))) );
            
%             imgs = cat( 3, imgs, double(rgb2gray(tmp_img)) );
            
            imgs = cat( 3, imgs, double(tmp_img(:,:,2) ));
        end
        
        img_crop = floor(size(imgs, [1,2])/2 - [img_h/2, img_w/2]) + 1;
        imgs = imgs(img_crop(1):img_crop(1)+img_h-1, img_crop(2):img_crop(2)+img_w-1, :);
        
        imgs = cat(3, imgs, mean(imgs, 3));
        
        conv_imgs = abs(convn(imgs, mx(end:-1:1, end:-1:1), 'valid'))/(sum(abs(mx(:))));
        fft_imgs = abs(fftn(imgs)/(img_h*img_w));
        
        fft_sum(jdx, :) = reshape(sum(sum(fft_imgs)), 1, num+1);
        conv_sum(jdx, :) = reshape(sum(sum(conv_imgs)), 1, num+1);
        

        img{jdx} = imgs;
        
        fft_stats(jdx, :) = [max(fft_sum(jdx, 1:num)), mean(fft_sum(jdx, 1:num)), fft_sum(jdx, num+1)];
        conv_stats(jdx, :) = [max(conv_sum(jdx, 1:num)), mean(conv_sum(jdx, 1:num)), conv_sum(jdx, num+1)];    
        
    end

    [~, conv_sharpest] = max(conv_stats(:,1));
    [~, fft_sharpest] = max(fft_stats(:,1));
    
    
    fprintf('%s, %s\n', image_listing(conv_sharpest).name, image_listing(fft_sharpest).name);

    figure(10+idx)
    set(gcf,'position',([50,50,1400,500]),'color','w')
    box on
    grid on
    hold on
    stairs(focus_vals{idx}, fft_stats(:,2), 'Linewidth', line_width);
    stairs(focus_vals{idx}, fft_stats(:,3), 'Linewidth', line_width);
    stem(focus_vals{idx}, fft_stats(:,1), 'b', 'Linewidth', line_width);
    stem(focus_vals{idx}(fft_sharpest), fft_stats(fft_sharpest, 1), 'r', 'Linewidth', line_width);
    set(gca,'fontweight','bold','FontSize',12);

    xlim([focus_vals{idx}(1), focus_vals{idx}(end)]);
    xtickangle(45);    
    ax = gca;
    ax.Position = [0.05 0.15 0.92 0.81];
    ax.XAxis.Exponent = 0;
    ax.XAxis.TickLabelFormat = '%05d';


    figure(20+idx)
    set(gcf,'position',([50,50,1400,500]),'color','w')
    box on
    grid on
    hold on
    stairs(focus_vals{idx}, conv_stats(:,2), 'Linewidth', line_width);
    stairs(focus_vals{idx}, conv_stats(:,3), 'Linewidth', line_width);
    stem(focus_vals{idx}, conv_stats(:,1), 'b', 'Linewidth', line_width);
    stem(focus_vals{idx}(conv_sharpest), conv_stats(conv_sharpest, 1), 'r', 'Linewidth', line_width);
    set(gca,'fontweight','bold','FontSize',12);

    xlim([focus_vals{idx}(1), focus_vals{idx}(end)]);
    xtickangle(45);    
    ax = gca;
    ax.Position = [0.05 0.15 0.92 0.81];
    ax.XAxis.Exponent = 0;
    ax.XAxis.TickLabelFormat = '%05d';
    pause(0.01);


if(false)

    for idx=1:num:numel(image_listing)

        fft_sum2 = zeros(num, 1);
        tmp_img = cell(num, 1);
%         tmp_img = double(rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx).name))));
%         [img_h, img_w] = size(tmp_img);

        [z, f, e, n] = parse_image_filename(image_listing(idx).name);
        
        
        for jdx=1:num
            tmp_img{jdx} = double(rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx+jdx-1).name))));
            tl = floor(size(tmp_img{jdx})/2 - [img_h, img_w]/2) + 1;
            
            
        end
        
        
        
        
        

%         tl = floor(size(tmp_img)/2 - [img_h, img_w]/2) + 1;
        img{img_idx} = tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1);
        fft_sum2(1) = sum(sum(abs(fft2(img{img_idx})/(img_w * img_h))));

        for jdx=1:num-1
            tmp_img = double(rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx+jdx).name))));
            tl = floor(size(tmp_img)/2 - [img_h, img_w]/2) + 1;
%             fft_sum2(jdx+1) = sum(sum(abs(fft2(tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1))/(img_w * img_h))));
            fft_sum2(jdx+1) = sum(sum(abs(conv2(tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1), my(end:-1:1, end:-1:1), 'valid'))));
            img{img_idx} = img{img_idx} + tmp_img(tl(1):tl(1)+img_h-1, tl(2):tl(2)+img_w-1);                                
        end

        img{img_idx} = img{img_idx}/num;
        fft_img = fft2(img{img_idx})/(img_w * img_h);
        fft_img(1,1) = 0;
        
        max_fft_sum{idx}(end+1) = mean(fft_sum2);
        
%         conv_img = abs(conv2(img{img_idx}, mx(end:-1:1, end:-1:1), 'valid') + conv2(img{img_idx}, my(end:-1:1, end:-1:1), 'valid'));
        conv_img = abs(conv2(img{img_idx}, my(end:-1:1, end:-1:1), 'valid'));
                
        figure(1)
%         image(uint8(img{img_idx}))
%         imagesc(fftshift(real(fft_img)));
        imagesc(conv_img);
        colormap(jet(256));

%         fft_sum{kdx}(end+1) = sum(abs(fft_img(:)));
        fft_sum{idx}(end+1) = mean(fft_sum2);
%         fft_sum{kdx}(end+1) = sum((conv_img(:)));
        
        
        focus_vals{idx}(end+1) = f; %ceil(f/inc)*inc; %floor(f/5+0.5)*5;
        img_idx = img_idx + 1;
    end

    [~, conv_sharpest] = max(fft_sum{idx});
    
    sharpest_img{idx} = img{conv_sharpest};
    img_name{idx} = image_listing((conv_sharpest-1)*num+1).name;
    
    figure(10+idx)
    set(gcf,'position',([50,50,1400,500]),'color','w')
    box on
    grid on
    hold on
%     stem(focus_vals{kdx}, fft_sum{kdx}, 'Linewidth', line_width)
%     stem(focus_vals{kdx}(shapest_index), fft_sum{kdx}(shapest_index), 'r', 'Linewidth', line_width)
    stairs(focus_vals{idx}, fft_sum{idx}, 'Linewidth', line_width)
    stem(focus_vals{idx}, max_fft_sum{idx}, 'b', 'Linewidth', line_width)
    stem(focus_vals{idx}(conv_sharpest), fft_sum{idx}(conv_sharpest), 'r', 'Linewidth', line_width)
    set(gca,'fontweight','bold','FontSize',12);

    xlim([focus_vals{idx}(1), focus_vals{idx}(end)])
    %xticks(focus_vals{kdx}(1:5:end));
    %xticklabels(num2str(focus_vals{kdx}'));
    xtickangle(45);    
    ax = gca;
    ax.Position = [0.05 0.15 0.92 0.81];
    ax.XAxis.Exponent = 0;
    ax.XAxis.TickLabelFormat = '%05d';
    
    pause(0.1);
end
end

%% remove the empty cells

index = [];
for idx=1:numel(focus_vals)
    
    if(~isempty(focus_vals{idx}))
       index = cat(1, index, idx);
    end
end

% fft_sum = {fft_sum{index}}';
focus_vals = {focus_vals{index}}';
sharpest_img = {sharpest_img{index}}';
img_name = {img_name{index}}';
ranges = ranges(index);

bp = 1;

%% show the sharpest images and save them

% ranges = zeros(numel(fft_sum), 1);

for idx=1:numel(fft_sum)

    figure(idx+40)
    set(gcf,'position',([50,50,600,500]),'color','w')
    image(uint8(sharpest_img{idx}))
    colormap(gray(256));    

    fprintf('%s, %s\n', listing(idx).name, img_name{idx});
    
    [z, f, e, n] = parse_image_filename(img_name{idx});
    
%     ranges(idx) = str2double(listing(idx).name);
    
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
        
        for idx=1:numel(focus_vals{idx})
            
            if(focus_vals{idx}(idx) == focus_idx(jdx))
                
                sh_data(idx, jdx) = fft_sum{idx}(idx);
                break;
            end
            
        end
        
    end
    
end

