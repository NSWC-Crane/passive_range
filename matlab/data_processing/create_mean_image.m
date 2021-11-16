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
%% select the zoom folder

image_path = uigetdir(strcat(scriptpath,'../../../../../'), 'Select Folder with Ranges');

if(image_path == 0)
    return;
end

listing = dir(image_path);
listing = listing(3:end);

%%

for idx=1:numel(listing)

    if(~isfolder(fullfile(listing(idx).folder,listing(idx).name)))
        continue;
    end
            
    image_listing = dir(strcat(fullfile(listing(idx).folder, listing(idx).name), '\*.png'));
    
    img = double(imread(fullfile(image_listing(1).folder, image_listing(1).name)));
    
    figure(1)
    set(gcf,'position',([50,50,1400,1000]),'color','w')
    imshow(uint8(img));
    title(num2str(1))
    pause(0.3);
        
    % cycle through the image listings
    for jdx=2:numel(image_listing)
        
        img = img + double(imread(fullfile(image_listing(idx).folder, image_listing(idx).name)));
        
        
        imshow(uint8(img/jdx));
        title(num2str(jdx))
        pause(0.3);

    end
   
    img = uint8(floor(img/numel(image_listing)));

    figure(2)
    imshow(img);
        
    img_filename = fullfile(listing(idx).folder, strcat('mean_img_', listing(idx).folder(end-4:end), '_r', listing(idx).name, '.png'));
    fprintf('saving file: %s\n', img_filename);

    imwrite(img, img_filename);

end

bp = 1;

%%


