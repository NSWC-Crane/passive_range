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

%% sdet up some constants
img_w = 128;
img_h = 128;

img_start = 10;
img_stop = 19;
num = img_stop - img_start + 1;

inc = 5;

gauss_filter = (1-abs(fft2(create_gauss_kernel(img_w, 10))));

mx = [-1, 0, 1; -2, 0 ,2; -1, 0, 1];
my = [1, 2, 1; 0, 0, 0; -1, -2, -1];

% mx = [-1, 0, 1; -1, 0 ,1; -1, 0, 1];
% my = [1, 1, 1; 0, 0, 0; -1, -1, -1];

% mx = [0, 1; -1, 0];
% my = [1, 0; 0, -1];

myx = my*mx;

%% select the folder where the images are located
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
fprintf('\n');

fft_sum = cell(numel(listing), 1);
max_fft_sum = cell(numel(listing), 1);
focus_vals = cell(numel(listing), 1);
sharpest_img = cell(numel(listing), 1);
% shapest_index = cell(numel(listing), 1);
img_name = cell(numel(listing), 1);

ranges = zeros(numel(listing), 1);

listing_count = 0;

[~, listing_folder, ~] = fileparts(listing(1).folder);

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
        
        [h, w, ~] = size(imgs);      
        img_crop = floor([h/2, w/2] - [img_h/2, img_w/2]) + 1;
        imgs = imgs(img_crop(1):img_crop(1)+img_h-1, img_crop(2):img_crop(2)+img_w-1, :);
        
        imgs = cat(3, imgs, mean(imgs, 3));
        
%         conv_imgs = abs(convn(imgs, mx(end:-1:1, end:-1:1), 'valid'))/(sum(abs(mx(:))));
%         conv_imgs = abs(convn(imgs, mx(end:-1:1, end:-1:1), 'valid'))/(10*sum(abs(mx(:)))) + abs(convn(imgs, my(end:-1:1, end:-1:1), 'valid'))/(10*sum(abs(my(:))));
        
        conv_imgs = (abs(imfilter(imgs, mx, 'replicate', 'same')) + abs(imfilter(imgs, my, 'replicate', 'same')))/1000;
        
        
        fft_imgs = abs(fftn(imgs)/(img_h*img_w));
        fft_imgs = fft_imgs .* gauss_filter;
%         fft_imgs(1,1, :) = 0;
        
        fft_sum(jdx, :) = reshape(sum(sum(fft_imgs)), 1, num+1);
        conv_sum(jdx, :) = reshape(sum(sum(conv_imgs)), 1, num+1);

        img{jdx} = imgs;
        
        fft_stats(jdx, :) = [max(fft_sum(jdx, 1:num)), mean(fft_sum(jdx, 1:num)), fft_sum(jdx, num+1)];
        conv_stats(jdx, :) = [max(conv_sum(jdx, 1:num)), mean(conv_sum(jdx, 1:num)), conv_sum(jdx, num+1)];    
        
        figure(1)
%         image(uint8(img{img_idx}))
%         imagesc(fftshift(real(fft_img)));
        imagesc(conv_imgs(:,:, num+1));
        colormap(jet(256));

    end

    [~, conv_sharpest] = max(conv_stats(:,2));
    [~, fft_sharpest] = max(fft_stats(:,2));
    
    sharpest_img{idx} = img{conv_sharpest};
    
    img_name{idx} = image_listing(fft_sharpest).name;
    
    fprintf('%04d, %s, %s\n', ranges(idx), image_listing(conv_sharpest).name, image_listing(fft_sharpest).name);

    figure(10+idx)
    set(gcf,'position',([50,50,1400,700]),'color','w')
    %subplot(2,1,1)
    box on
    grid on
    hold on
    s1 = plot(focus_vals{idx}, fft_stats(:,2), 'b', 'Linewidth', line_width);
    s2 = plot(focus_vals{idx}, conv_stats(:,2), 'g', 'Linewidth', line_width);    
    
    %     s3 = stem(focus_vals{idx}, fft_stats(:,1), 'b', 'Linewidth', line_width);
    stem(focus_vals{idx}(fft_sharpest), fft_stats(fft_sharpest, 2), 'b', 'Linewidth', line_width);
    stem(focus_vals{idx}(conv_sharpest), conv_stats(conv_sharpest, 2), 'g', 'Linewidth', line_width);
    
    set(gca,'fontweight','bold','FontSize',12);

%     xlim([focus_vals{idx}(1), focus_vals{idx}(end)]);
    %xtickangle(45);    
%     xticks([]);
%     ax = gca;
%     ax.Position = [0.05 0.15 0.92 0.81];
%     ax.XAxis.Exponent = 0;
%     ax.XAxis.TickLabelFormat = '%05d';
%     legend([s1,s2,s3], {'mean1', 'mean2', 'max'}, 'Location', 'northoutside', 'Orientation', 'horizontal');
    legend([s1,s2], {'fft mean', 'conv mean'}, 'Location', 'northoutside', 'Orientation', 'horizontal');

%     subplot(2,1,2)
%     box on
%     grid on
%     hold on
%     plot(focus_vals{idx}, conv_stats(:,2), 'Linewidth', line_width);
%     plot(focus_vals{idx}, conv_stats(:,3), 'Linewidth', line_width);
% %     stem(focus_vals{idx}, conv_stats(:,1), 'b', 'Linewidth', line_width);
%     stem(focus_vals{idx}(conv_sharpest), conv_stats(conv_sharpest, 2), 'r', 'Linewidth', line_width);
%     set(gca,'fontweight','bold','FontSize',12);

    xlim([focus_vals{idx}(1), focus_vals{idx}(end)]);
    xtickangle(45);    
    ax = gca;
%     ax.Position = [0.05 0.15 0.92 0.81];
    ax.XAxis.Exponent = 0;
    ax.XAxis.TickLabelFormat = '%05d';
    pause(0.01);

end

% remove the empty cell
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

%% copy the sharpest fft image to the save folder
fprintf('\n');

% copy_base_location = 'D:/data/turbulence/sharpest';
% 
% % ranges = zeros(numel(fft_sum), 1);
% 
% for idx=1:numel(img_name)
%     
%     img_root = strcat(img_name{idx}(1:end-6), '*');
%     img_folder = fullfile(listing(idx).folder, listing(idx).name);
%     
%     copy_location = fullfile(copy_base_location, listing_folder, num2str(ranges(idx), '%04d'));
%     
%     fprintf('copying: %s to %s\n', strcat(img_folder, '/', img_root), copy_location);
%     copyfile(strcat(img_folder, '/', img_root), copy_location, 'f')
%     
% end


%% create a combined matrix

% get the min and max focus value
% fv = [focus_vals{:}];
% 
% min_fv = min(fv);
% max_fv = max(fv);
% 
% focus_idx = min_fv:inc:max_fv;
% 
% sh_data = zeros(numel(ranges), numel(focus_idx));

% for idx=1:numel(ranges)
%     
%     for jdx=1:numel(focus_idx)
%         
%         for idx=1:numel(focus_vals{idx})
%             
%             if(focus_vals{idx}(idx) == focus_idx(jdx))
%                 
%                 sh_data(idx, jdx) = fft_sum{idx}(idx);
%                 break;
%             end
%             
%         end
%         
%     end
%     
% end


