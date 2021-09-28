function [max_val, index] = find_sharpest_image(image_listing, pt_x, pt_y, img_w, img_h)

    img_vals = zeros(numel(image_listing), 1);
    
    h = pt_y - floor(img_h/2);
    w = pt_x - floor(img_w/2);
    
    for idx=1:numel(image_listing)
        
        fprintf('%s/%s\n', image_listing(idx).folder, image_listing(idx).name);

        img = rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx).name)));

        img = img(h:h+img_h-1, w:w+img_w-1);
        
        image(img);
        colormap(gray(256));
        axis off;
        
        pause(0.2);
        
        Y = abs(fftshift(fft2(img)))/(size(img,1)*size(img,2));
        img_vals(idx) = sum(Y(:));
    end
    
    [max_val, index] = max(img_vals);
    
    fprintf('#-------------------------------------------------------------\n');
    fprintf('%s/%s\n', image_listing(idx).folder, image_listing(index).name);

end