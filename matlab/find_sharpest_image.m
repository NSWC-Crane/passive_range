function [max_val, index] = find_sharpest_image(image_listing, pt_x, pt_y, img_w, img_h)

    img_vals = zeros(numel(image_listing), 1);
    yy_max = zeros(numel(image_listing), 1);
    
    h = pt_y - floor(img_h/2);
    w = pt_x - floor(img_w/2);
    
    for idx=1:numel(image_listing)
        

        img = rgb2gray(imread(fullfile(image_listing(idx).folder, image_listing(idx).name)));
        img = img(h:h+img_h-1, w:w+img_w-1);
        
        figure(1);
        image(img);
        colormap(gray(256));
        axis off;
        
        yy = fft(img(floor(img_h/2), :))/img_w;
        yy(1) = 0;
        yy_max(idx) = max(abs(yy));
        
        Y = abs(fftshift(fft2(img)))/(size(img,1)*size(img,2));
        Y(1,1) = 0;
        
        figure(2);
        surf(abs(Y));
        shading('flat');
        colormap(jet(200))
        view(16,0);
        
        figure(3)
        plot(abs(yy(1:floor(img_w/2))))
        
        img_vals(idx) = sum(Y(:));
        fprintf('%s/%s, %6.4f\n', image_listing(idx).folder, image_listing(idx).name, img_vals(idx));
        pause(0.5);
        input(' ');
    end
    
    [max_val, index] = max(img_vals);
    
    fprintf('#-------------------------------------------------------------\n');
    fprintf('%s/%s, %6.4f\n', image_listing(idx).folder, image_listing(index).name, max_val);

end